#include "test.hpp"
#include "engine.hpp"
#include "primitive.hpp"
#include "geometry.hpp"

// ---------------------- Primitive ----------------------
rs::WVb Primitive::s_wVb[Type::Num][2];
rs::WIb Primitive::s_wIb[Type::Num][2];
const rs::IdValue Primitive::T_Prim = GlxId::GenTechId("Primitive", "Default"),
				Primitive::T_PrimDepth = GlxId::GenTechId("Primitive", "Depth"),
				Primitive::T_PrimCube = GlxId::GenTechId("Primitive", "CubeDefault"),
				Primitive::T_PrimCubeDepth = GlxId::GenTechId("Primitive", "CubeDepth");
void Primitive::_initVb(Type::E typ, bool bFlat, bool bFlip) {
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
		std::vector<vertex::prim> tmpV(nV);
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
Primitive::Primitive(float s, rs::HTex hTex, Type::E typ, bool bFlat, bool bFlip):
	_hlTex(hTex)
{
	setScale({s,s,s});
	_initVb(typ, bFlat, bFlip);
}
void Primitive::advance() {
	const spn::AQuat& q = this->getRot();
	auto& self = const_cast<Primitive&>(*this);
	self.setRot(q >> spn::AQuat::Rotation(spn::AVec3(1,1,0).normalization(), spn::RadF(0.01f)));
}
void Primitive::draw(Engine& e) const {
	auto typ = e.getDrawType();
	if(typ == Engine::DrawType::Normal) {
		e.setTechPassId(T_Prim);
		e.setUniform(rs::unif3d::texture::Diffuse, _hlTex);
	} else if(typ == Engine::DrawType::CubeNormal) {
		e.setTechPassId(T_PrimCube);
		e.setUniform(rs::unif3d::texture::Diffuse, _hlTex);
	} else if(typ == Engine::DrawType::CubeDepth) {
		e.setTechPassId(T_PrimCubeDepth);
	} else
		e.setTechPassId(T_PrimDepth);
	e.setVDecl(rs::DrawDecl<vdecl::prim>::GetVDecl());
	e.ref<rs::SystemUniform3D>().setWorld(getToWorld().convertA44());
	e.setVStream(_hlVb, 0);
	e.setIStream(_hlIb);
	e.drawIndexed(GL_TRIANGLES, _hlIb->get()->getNElem());
}
void Primitive::exportDrawTag(rs::DrawTag& d) const {
	d.idTex[0] = _hlTex.get();
}

// ---------------------- Primitive頂点宣言 ----------------------
const rs::SPVDecl& rs::DrawDecl<vdecl::prim>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
		{0,12, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::TEXCOORD0},
		{0,20, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::NORMAL}
	});
	return vd;
}

// ---------------------- PrimitiveObj ----------------------
struct PrimitiveObj::St_Default : StateT<St_Default> {
	void onDraw(const PrimitiveObj& self, rs::IEffect& e) const override {
		self.draw(static_cast<Engine&>(e));
	}
};
PrimitiveObj::PrimitiveObj(float size, rs::HTex hTex, Type::E typ, bool bFlat, bool bFlip):
	Primitive(size, hTex, typ, bFlat, bFlip)
{
	setStateNew<St_Default>();
}
#include "../luaimport.hpp"
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, PrimitiveObj, PrimitiveObj, "DrawableObj", NOTHING,
		(advance)
		(setOffset),
		(float)(rs::HTex)(Primitive::Type::E)(bool)(bool))
