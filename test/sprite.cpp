#include "test.hpp"
#include "sprite.hpp"
#include "engine.hpp"
#include "spinner/structure/profiler.hpp"

const rs::IdValue Sprite::T_Sprite = GlxId::GenTechId("Sprite", "Default");
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
	_zRange = {0.f, 1.f};
	_alpha = 1.f;
	_initBuffer();
}
void Sprite::setZOffset(float z) {
	_zOffset = z;
}
void Sprite::setAlpha(float a) {
	_alpha = a;
}
void Sprite::setZRange(const spn::RangeF& r) {
	_zRange = r;
}
void Sprite::draw(Engine& e) const {
	spn::profiler.beginBlockObj("sprite::draw");
	e.setVDecl(rs::DrawDecl<vdecl::sprite>::GetVDecl());
	e.setUniform(rs::unif2d::texture::Diffuse, _hlTex);
	e.setUniform(rs::unif::Alpha, _alpha);
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
const rs::SPVDecl& rs::DrawDecl<vdecl::sprite>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
		{0,12, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::TEXCOORD0}
	});
	return vd;
}
