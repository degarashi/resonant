#include "test.hpp"
#include "engine.hpp"
#include "cube.hpp"

// ---------------------- Cube ----------------------
rs::WVb Cube::s_wVb[2];
const rs::IdValue Cube::T_Cube = GlxId::GenTechId("Cube", "Default"),
				Cube::T_CubeDepth = GlxId::GenTechId("Cube", "Depth"),
				Cube::T_CubeCube = GlxId::GenTechId("Cube", "CubeDefault"),
				Cube::T_CubeCubeDepth = GlxId::GenTechId("Cube", "CubeDepth");
void Cube::_initVb(bool bFlip) {
	int index = bFlip ? 1 : 0;

	if(!(_hlVb = s_wVb[index].lock())) {
		using spn::Vec2;
		using spn::Vec3;
		// 大きさ1の立方体を定義しておいて後で必要に応じてスケーリングする
		vertex::cube tmpV[6*6];
		Vec3 tmpPos[8];
		Vec2 tmpUV[4];
		for(int i=0 ; i<0x8 ; i++) {
			// 座標生成
			int cur0 = (i & 0x01) ? 1 : -1,
				cur1 = (i & 0x02) ? 1 : -1,
				cur2 = (i & 0x04) ? 1 : -1;
			tmpPos[i] = Vec3(cur0, cur1, cur2);
			// UV座標生成
			if(i<4)
				tmpUV[i] = Vec2(std::max(cur0, 0), std::max(cur1, 0));
		}
		const int tmpI[6*6] = {
			// Front
			0,2,1,
			2,3,1,
			// Right
			1,3,7,
			7,5,1,
			// Left
			4,6,2,
			2,0,4,
			// Top
			6,7,2,
			7,3,2,
			// Bottom
			0,1,5,
			5,4,0,
			// Back
			7,6,4,
			4,5,7
		};
		const int tmpI_CW[3] = {0,1,2},
				tmpI_CCW[3] = {2,1,0};
		const int (&tmpI_Index)[3] = (bFlip) ? tmpI_CCW : tmpI_CW;
		const int tmpI_uv[6] = {
			0,1,2,
			2,1,3
		};
		const Vec3 normal[6] = {
			Vec3(0,0,-1),
			Vec3(1,0,0),
			Vec3(-1,0,0),
			Vec3(0,1,0),
			Vec3(0,-1,0),
			Vec3(0,0,1)
		};
		for(int i=0 ; i<6*6 ; i++) {
			tmpV[i].pos = tmpPos[tmpI[(i/3*3)+tmpI_Index[i%3]]];
			auto& uv = tmpUV[tmpI_uv[i%6]];
			tmpV[i].tex = uv;
			tmpV[i].normal = normal[i/6];
			if(bFlip)
				tmpV[i].normal *= -1;
		}

		_hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
		_hlVb.ref()->initData(tmpV, countof(tmpV), sizeof(vertex::cube));
		s_wVb[index] = _hlVb.weak();
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
