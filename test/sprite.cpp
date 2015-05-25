#include "test.hpp"
#include "sprite.hpp"
#include "engine.hpp"

const rs::IdValue Sprite::T_Sprite = GlxId::GenTechId("TheSprite", "P0");
// ----------------------- Sprite -----------------------
rs::WVb Sprite::s_wVb;
rs::WIb Sprite::s_wIb;
void Sprite::_initBuffer() {
	if(!(_hlVb = s_wVb.lock())) {
		// 大きさ1の矩形を定義して後でスケーリング
		vertex::sprite tmpV[] = {
			{{0,1}, {0,0}},
			{{1,1}, {1,0}},
			{{1,0}, {1,1}},
			{{0,0}, {0,1}}
		};
		_hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
		_hlVb->get()->initData(tmpV, countof(tmpV), sizeof(vertex::sprite));

		GLushort idx[] = {0,1,2, 2,3,0};
		_hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
		_hlIb->get()->initData(idx, countof(idx));

		s_wVb = _hlVb.weak();
		s_wIb = _hlIb.weak();
	} else
		_hlIb = s_wIb.lock();
}
Sprite::Sprite(rs::HTex hTex, float z) {
	_hlTex = hTex;
	_zOffset = z;
	_initBuffer();
}
void Sprite::draw(Engine& e) const {
	e.setVDecl(rs::DrawDecl<drawtag::sprite>::GetVDecl());
	e.setUniform(rs::unif2d::texture::Diffuse, _hlTex);
	e.ref2d().setWorld(getToWorld().convertA33());
	e.setVStream(_hlVb, 0);
	e.setIStream(_hlIb);
	e.drawIndexed(GL_TRIANGLES, 6);
}
void Sprite::exportDrawTag(rs::DrawTag& d) const {
	d.idTex[0] = _hlTex;
	d.idVBuffer[0] = _hlVb;
	d.idIBuffer = _hlIb;
	d.zOffset = _zOffset;
}
// ---------------------- Sprite頂点宣言 ----------------------
const rs::SPVDecl& rs::DrawDecl<drawtag::sprite>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
		{0,12, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::TEXCOORD0}
	});
	return vd;
}
