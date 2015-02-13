#include "test.hpp"

// ---------------------- Cube ----------------------
const rs::IdValue Cube::U_diffuse = GlxId::GenUnifId("tDiffuse"),
					Cube::U_trans = GlxId::GenUnifId("mTrans"),
					Cube::U_litdir = GlxId::GenUnifId("vLitDir");
Cube::Cube(float s, rs::HTex hTex): _hlTex(hTex) {
	setScale({s,s,s});
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
		tmpV[i].pos = tmpPos[tmpI[i]];
		auto& uv = tmpUV[tmpI_uv[i%6]];
		tmpV[i].tex = uv;
		tmpV[i].normal = normal[i/6];
	}

	_hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
	_hlVb.ref()->initData(tmpV, countof(tmpV), sizeof(vertex::cube));
}
void Cube::draw(rs::GLEffect& glx) {
	const spn::AQuat& q = this->getRot();
	setRot(q >> spn::AQuat::Rotation(spn::AVec3(1,1,0).normalization(), spn::RadF(0.01f)));
	glx.setVDecl(rs::DrawDecl<drawtag::cube>::GetVDecl());
	glx.setUniform(U_diffuse, _hlTex);
	auto m = getToWorld();
	auto m2 = m.convertA44() * spn::AMat44::Translation(spn::Vec3(0,0,2));
	auto lk = shared.lock();
	rs::CamData& cd = lk->hlCam.ref();
	m2 *= cd.getViewProjMatrix().convert44();
	glx.setUniform(U_trans, m2, true);
	glx.setVStream(_hlVb, 0);

	glx.setUniform(U_litdir, spn::Vec3(0,1,0), false);
	glx.draw(GL_TRIANGLES, 0, 6*6);
}
// ---------------------- Cube頂点宣言 ----------------------
const rs::SPVDecl& rs::DrawDecl<drawtag::cube>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
		{0,12, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::TEXCOORD0},
		{0,20, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::NORMAL}
	});
	return vd;
}

// ---------------------- CubeObj::MySt ----------------------
void CubeObj::MySt::onUpdate(CubeObj& self) {}
void CubeObj::MySt::onConnected(CubeObj& self, rs::HGroup) {
	self._dtag.zOffset = 1.f;
	auto& d = mgr_scene.getSceneBase().draw;
	auto hl = self.handleFromThis();
	d->get()->addObj(hl);
	PrintLog;
}
void CubeObj::MySt::onDisconnected(CubeObj& self, rs::HGroup) {
	auto& d = mgr_scene.getSceneBase().draw;
	auto hl = self.handleFromThis();
	d->get()->remObj(hl);
	PrintLog;
}
void CubeObj::MySt::onDraw(const CubeObj& self) const {
	auto lk = shared.lock();
	auto& fx = *lk->pFx;
	fx.setTechPassId(self._tpId);
	self._cube.draw(fx);
}
// ---------------------- CubeObj ----------------------
CubeObj::CubeObj(rs::HTex hTex, rs::IdValue tpId):
	_tpId(tpId),
	_cube(1.f, hTex)
{
	PrintLog;
	setStateNew<MySt>();
}
CubeObj::~CubeObj() {
	PrintLog;
}

