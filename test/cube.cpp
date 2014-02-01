#include "test.hpp"

// ---------------------- Cube ----------------------
Cube::Cube(float s, rs::HTex hTex): _hlTex(hTex), _techID(-1), _passID(-1) {
	setScale(s,s,s);
	using spn::Vec2;
	using spn::Vec3;
	// 大きさ1の立方体を定義しておいて後で必要に応じてスケーリングする
	struct TmpV {
		Vec3 pos;
		Vec2 tex;
		Vec3 normal;
	};

	TmpV tmpV[6*6];
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
	_hlVb.ref()->initData(tmpV, countof(tmpV), sizeof(TmpV));
}
void Cube::draw(rs::GLEffect& glx) {
	const spn::AQuat& q = this->getRot();
	setRot(q >> spn::AQuat::Rotation(spn::AVec3(1,1,0).normalization(), 0.01f));
	if(_techID < 0) {
		_techID = glx.getTechID("TheCube");
		glx.setTechnique(_techID, true);
		_passID = glx.getPassID("P0");
	}
	glx.setTechnique(_techID, true);
	glx.setPass(_passID);
	// 頂点フォーマット定義
	rs::SPVDecl decl(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
		{0,12, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::TEXCOORD0},
		{0,20, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::NORMAL}
	});
	glx.setVDecl(std::move(decl));
	glx.setUniform(glx.getUniformID("tDiffuse"), _hlTex);
	auto m = getToWorld();
	auto m2 = m.convertA44() * spn::AMat44::Translation(spn::Vec3(0,0,2));
	auto lk = shared.lock();
	rs::CamData& cd = lk->hlCam.ref();
	m2 *= cd.getViewProjMatrix().convert44();
	glx.setUniform(glx.getUniformID("mTrans"), m2);
	glx.setVStream(_hlVb, 0);

	glx.setUniform(glx.getUniformID("vLitDir"), spn::Vec3(0,1,0));
	glx.draw(GL_TRIANGLES, 0, 6*6);
}

// ---------------------- CubeObj ----------------------
void CubeObj::MySt::onUpdate(CubeObj& self) {}
CubeObj::CubeObj(rs::HTex hTex): _cube(1.f, hTex) {
	LogOutput("TestObj::ctor");
	setStateNew<MySt>();
}
CubeObj::~CubeObj() {
	LogOutput("TestObj::dtor");
}
void CubeObj::onDestroy() {
	LogOutput("TestObj::onDestroy");
}
