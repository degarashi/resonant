#include "test.hpp"
#include "engine.hpp"
#include "cube.hpp"
#include "geometry.hpp"

// ---------------------- Cube ----------------------
rs::WVb Cube::s_wVb[Type::Num][2];
rs::WIb Cube::s_wIb[Type::Num][2];
const rs::IdValue Cube::T_Cube = GlxId::GenTechId("Cube", "Default"),
				Cube::T_CubeDepth = GlxId::GenTechId("Cube", "Depth"),
				Cube::T_CubeCube = GlxId::GenTechId("Cube", "CubeDefault"),
				Cube::T_CubeCubeDepth = GlxId::GenTechId("Cube", "CubeDepth");
void Cube::_initVb(Type::E typ, bool bFlat, bool bFlip) {
	int indexI = bFlip ? 1 : 0;

	if(!(_hlVb = s_wVb[typ][indexI].lock())) {
		boom::Vec3V tmpPos;
		boom::IndexV tmpIndex;
		boom::Vec2V tmpUv;
		spn::Pose3D pose;
		switch(typ) {
			case Type::Cone:
				boom::geo3d::Geometry::MakeCone(tmpPos, tmpIndex, 16);
				break;
			case Type::Cube:
				boom::geo3d::Geometry::MakeCube(tmpPos, tmpIndex);
				break;
			case Type::Sphere:
				boom::geo3d::Geometry::MakeSphere(tmpPos, tmpIndex, 32, 16);
				break;
			case Type::Torus:
				boom::geo3d::Geometry::MakeTorus(tmpPos, tmpIndex, 0.5f, 16, 12);
				break;
			case Type::Capsule:
				boom::geo3d::Geometry::MakeCapsule(tmpPos, tmpIndex, 1, 8);
				break;
			default:
				AssertF(Trap, "invalid type")
		};
		if(bFlip)
			boom::FlipFace(tmpIndex.begin(), tmpIndex.end(), tmpIndex.begin(), 0);

		if(typ == Type::Cube)
			boom::geo3d::Geometry::UVUnwrapCylinder(tmpUv, pose, tmpPos);
		else
			boom::geo3d::Geometry::UVUnwrapSphere(tmpUv, 2,1, pose, tmpPos);

		boom::Vec3V posv, normalv;
		boom::Vec2V uvv;
		boom::IndexV indexv;
		if(bFlat) {
			boom::geo3d::Geometry::MakeVertexNormalFlat(posv, indexv, normalv, uvv,
														tmpPos, tmpIndex, tmpUv);
		} else {
			boom::geo3d::Geometry::MakeVertexNormal(normalv, tmpPos, tmpIndex);
			posv = tmpPos;
			uvv = tmpUv;
			indexv = tmpIndex;
		}
		// 大きさ1の立方体を定義しておいて後で必要に応じてスケーリングする
		const int nV = posv.size();
		std::vector<vertex::cube> tmpV(nV);
		for(int i=0 ; i<nV ; i++) {
			tmpV[i].pos = posv[i];
			tmpV[i].tex = uvv[i];
			tmpV[i].normal = normalv[i];
		}
		_hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
		_hlVb.ref()->initData(std::move(tmpV));
		s_wVb[typ][indexI] = _hlVb.weak();
		_hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
		_hlIb.ref()->initData(std::move(indexv));
		s_wIb[typ][indexI] = _hlIb.weak();
	} else
		_hlIb = s_wIb[typ][indexI].lock();
}
Cube::Cube(float s, rs::HTex hTex, Type::E typ, bool bFlat, bool bFlip):
	_hlTex(hTex)
{
	setScale({s,s,s});
	_initVb(typ, bFlat, bFlip);
}
void Cube::advance() {
	const spn::AQuat& q = this->getRot();
	auto& self = const_cast<Cube&>(*this);
	self.setRot(q >> spn::AQuat::Rotation(spn::AVec3(1,1,0).normalization(), spn::RadF(0.01f)));
}
void Cube::draw(Engine& e) const {
	auto typ = e.getDrawType();
	if(typ == Engine::DrawType::Normal) {
		e.setTechPassId(T_Cube);
		e.setUniform(rs::unif3d::texture::Diffuse, _hlTex);
	} else if(typ == Engine::DrawType::CubeNormal) {
		e.setTechPassId(T_CubeCube);
		e.setUniform(rs::unif3d::texture::Diffuse, _hlTex);
	} else if(typ == Engine::DrawType::CubeDepth) {
		e.setTechPassId(T_CubeCubeDepth);
	} else
		e.setTechPassId(T_CubeDepth);
	e.setVDecl(rs::DrawDecl<vdecl::cube>::GetVDecl());
	e.ref<rs::SystemUniform3D>().setWorld(getToWorld().convertA44());
	e.setVStream(_hlVb, 0);
	e.setIStream(_hlIb);
	e.drawIndexed(GL_TRIANGLES, _hlIb->get()->getNElem());
}
void Cube::exportDrawTag(rs::DrawTag& d) const {
	d.idTex[0] = _hlTex.get();
}

// ---------------------- Cube頂点宣言 ----------------------
const rs::SPVDecl& rs::DrawDecl<vdecl::cube>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
		{0,12, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::TEXCOORD0},
		{0,20, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::NORMAL}
	});
	return vd;
}

// ---------------------- CubeObj ----------------------
struct CubeObj::St_Default : StateT<St_Default> {
	void onDraw(const CubeObj& self, rs::IEffect& e) const override {
		self.draw(static_cast<Engine&>(e));
	}
};
CubeObj::CubeObj(float size, rs::HTex hTex, Type::E typ, bool bFlat, bool bFlip):
	Cube(size, hTex, typ, bFlat, bFlip)
{
	setStateNew<St_Default>();
}
#include "../luaimport.hpp"
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, CubeObj, CubeObj, "DrawableObj", NOTHING,
		(advance)
		(setOffset),
		(float)(rs::HTex)(Cube::Type::E)(bool)(bool))
