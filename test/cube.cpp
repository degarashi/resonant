#include "test.hpp"

// ---------------------- Cube ----------------------
Cube::Cube(float s, rs::HTex hTex): _hlTex(hTex) {
	// 大きさ1の立方体を定義しておいて後で必要に応じてスケーリングする
	struct TmpV {
		spn::Vec3 pos;
		spn::Vec4 tex;
	};

	TmpV tmpV[6*6];
	spn::Vec3 tmpPos[8];
	spn::Vec2 tmpUV[4];
	for(int i=0 ; i<0x8 ; i++) {
		// 座標生成
		int cur0 = (i & 0x01) ? 1 : -1,
			cur1 = (i & 0x02) ? 1 : -1,
			cur2 = (i & 0x04) ? 1 : -1;
		tmpPos[i] = spn::Vec3(cur0, cur1, cur2);
		// UV座標生成
		if(i<4)
			tmpUV[i] = spn::Vec2(std::max(cur0, 0), std::max(cur1, 0));
	}
	const int tmpI[6*6] = {
		0,2,1,
		2,3,1,

		1,3,7,
		7,5,1,

		4,6,2,
		2,0,4,

		6,7,2,
		7,3,2,

		0,1,5,
		5,4,0,

		7,6,4,
		4,5,7
	};
	const int tmpI_uv[6] = {
		0,1,2,
		2,1,3
	};
	for(int i=0 ; i<6*6 ; i++) {
		tmpV[i].pos = tmpPos[tmpI[i]];
		auto& uv = tmpUV[tmpI_uv[i%6]];
		tmpV[i].tex = spn::Vec4(uv.x, uv.y, 0, 0);
	}

	_hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
	_hlVb.ref()->initData(tmpV, countof(tmpV), sizeof(TmpV));
}
void Cube::draw(rs::GLEffect& glx) {
	if(_techID < 0) {
		_techID = glx.getTechID("TheCube");
		_passID = glx.getPassID("P0");
	}
	glx.setTechnique(_techID, true);
	glx.setPass(_passID);
	// 頂点フォーマット定義
	rs::SPVDecl decl(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
		{0,12, GL_FLOAT, GL_FALSE, 4, (GLuint)rs::VSem::TEXCOORD0}
	});
	glx.setVDecl(std::move(decl));
	glx.setUniform(glx.getUniformID("tDiffuse"), _hlTex);
	auto m = getToWorld();
	glx.setUniform(glx.getUniformID("mCube"), m);
	glx.setVStream(_hlVb, 0);
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
