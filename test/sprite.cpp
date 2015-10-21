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
	e.ref<rs::SystemUniform2D>().setWorld(getToWorld().convertA33());
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

// ----------------------- SpriteObj -----------------------
SpriteObj::SpriteObj(rs::HDGroup hDg, rs::HTex hTex, float depth):
	DWrapper(::MakeCallDraw<Engine>(), Sprite::T_Sprite, hDg, hTex, depth) {}
#include "../luaimport.hpp"
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, SpriteObj, SpriteObj, "Object", NOTHING, (setOffset)(setScale)(setAngle)(setZOffset)(setZRange)(setAlpha)(setPriority), (rs::HDGroup)(rs::HTex)(float))

// ----------------------- U_BoundingSprite -----------------------
U_BoundingSprite::U_BoundingSprite(rs::HDGroup hDg, rs::HTex hTex, const spn::Vec2& pos, const spn::Vec2& svec):
	BoundingSprite(Sprite::T_Sprite, hDg, hTex, pos, svec) {}
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, U_BoundingSprite, U_BoundingSprite, "Object", NOTHING,
		(setScale), (rs::HDGroup)(rs::HTex)(const spn::Vec2&)(const spn::Vec2&))
// ----------------------- BoundingSprite -----------------------
BoundingSprite::BoundingSprite(rs::IdValue tpId, rs::HDGroup hDg, rs::HTex hTex, const spn::Vec2& pos, const spn::Vec2& svec):
	base_t(::MakeCallDraw<Engine>(), tpId, hDg, hTex, 0.f),
	_svec(svec)
{
	setOffset(pos);
	setZRange({-1.f, 1.f});
	setStateNew<St_Default>();
}
struct BoundingSprite::St_Default : base_t::St_Default {
	void onUpdate(base_t& self) override {
		auto& ths = static_cast<BoundingSprite&>(self);
		auto& sc = self.getScale();
		auto& ofs = self.refOffset();
		auto& svec = ths._svec;
		ofs += svec;
		// 境界チェック
		// Left
		if(ofs.x < -1.f) {
			svec.x *= -1.f;
			ofs.x = -1.f;
		}
		// Right
		if(ofs.x + sc.x > 1.f) {
			svec.x *= -1.f;
			ofs.x = 1.f - sc.x;
		}
		// Top
		if(ofs.y + sc.y > 1.f) {
			svec.y *= -1.f;
			ofs.y = 1.f - sc.y;
		}
		// Bottom
		if(ofs.y < -1.f) {
			svec.y *= -1.f;
			ofs.y = -1.f;
		}

		// ZOffset = y
		self.setZOffset(ofs.y);
		self.refreshDrawTag();
	}
};
