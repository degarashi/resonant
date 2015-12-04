#include "test.hpp"
#include "engine.hpp"
#include "cube.hpp"
#include "geometry.hpp"

// ---------------------- Cube ----------------------
rs::WVb Cube::s_wVb[2];
const rs::IdValue Cube::T_Cube = GlxId::GenTechId("Cube", "Default"),
				Cube::T_CubeDepth = GlxId::GenTechId("Cube", "Depth"),
				Cube::T_CubeCube = GlxId::GenTechId("Cube", "CubeDefault"),
				Cube::T_CubeCubeDepth = GlxId::GenTechId("Cube", "CubeDepth");
void Cube::_initVb(bool bFlip) {
	int indexI = bFlip ? 1 : 0;

	if(!(_hlVb = s_wVb[indexI].lock())) {
		boom::Vec3V tmpPos;
		boom::IndexV tmpIndex;
		boom::geo3d::Geometry::MakeCube(tmpPos, tmpIndex);
		if(bFlip)
			boom::FlipFace(tmpIndex.begin(), tmpIndex.end(), tmpIndex.begin(), 0);

		boom::Vec2V tmpUv;
		spn::Pose3D pose;
		boom::geo3d::Geometry::UVUnwrapCylinder(tmpUv, pose, tmpPos);

		boom::Vec3V posv, normalv;
		boom::Vec2V uvv;
		boom::IndexV indexv;
		boom::geo3d::Geometry::MakeVertexNormalFlat(posv, indexv, normalv, uvv,
													tmpPos, tmpIndex, tmpUv);
		// 大きさ1の立方体を定義しておいて後で必要に応じてスケーリングする
		vertex::cube tmpV[6*6];
		for(int i=0 ; i<6*6 ; i++) {
			const auto idx = indexv[i];
			tmpV[i].pos = posv[idx];
			tmpV[i].tex = uvv[idx];
			tmpV[i].normal = normalv[idx];
		}

		_hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
		_hlVb.ref()->initData(tmpV, countof(tmpV), sizeof(vertex::cube));
		s_wVb[indexI] = _hlVb.weak();
	}
}
Cube::Cube(float s, rs::HTex hTex, bool bFlip):
	_hlTex(hTex)
{
	setScale({s,s,s});
	_initVb(bFlip);
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
	e.draw(GL_TRIANGLES, 0, 6*6);
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
CubeObj::CubeObj(float size, rs::HTex hTex, bool bFlip):
	Cube(size, hTex, bFlip)
{
	setStateNew<St_Default>();
}
#include "../luaimport.hpp"
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, CubeObj, CubeObj, "DrawableObj", NOTHING,
		(advance)
		(setOffset),
		(float)(rs::HTex)(bool))
