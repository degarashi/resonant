#include "clipmap.hpp"
#include "../glresource.hpp"
#include "spinner/rectdiff.hpp"
#include "../glx_if.hpp"

// --------------------- Clipmap::Layer::IOfs ---------------------
Clipmap::Layer::IOfs::IOfs(const spn::Vec2& c, const float s) {
	using spn::rect::LoopValue;
	const float s2 = s*2;
	mx = LoopValue(c.x, s2);
	my = LoopValue(c.y, s2);
	ix = static_cast<int>(mx/s) & 1;
	iy = static_cast<int>(my/s) & 1;
}
// --------------------- Clipmap::Layer ---------------------
Clipmap::Layer::Layer(const PowSize s, const float sc):
	_cx(-1e8),
	_cy(-1e8),
	_scale(sc)
{
	_normal = mgr_gl.createTexture(s, GL_RGBA8, false, false);
	_normal->get()->setWrap(rs::WrapState::Repeat);
	_elevation = mgr_gl.createTexture(s, GL_RG16F, false, false);
	_elevation->get()->setWrap(rs::WrapState::Repeat);
	// _diffuse = mgr_gl.createTexture(s, GL_RGB8, false, false);
	// _diffuse->get()->setWrap(rs::WrapState::Repeat);

	// ---- チェックの為の塗りつぶし ----
	std::vector<Vec2> data(s.width * s.height);
	for(auto& d : data)
		d = Vec2(0.1f);
	auto* el = static_cast<rs::Texture_Mem*>(_elevation->get());
	el->writeData({data.data(), data.size()*sizeof(Vec2)}, GL_FLOAT);
	// 間違いが分かりやすいようにFinerLayer(1,0,0), Coarser(0,1,0) とする
	std::vector<Vec4> data2(s.width * s.height);
	for(auto& d : data2)
		d = Vec4(1.f, 0.f, 0.f,1.f);
	el = static_cast<rs::Texture_Mem*>(_normal->get());
	el->writeData({data2.data(), data2.size()*sizeof(Vec4)}, GL_FLOAT);
}
void Clipmap::Layer::setElevSource(const IClipSource_SP& s) {
	_srcElev = s;
}
void Clipmap::Layer::setDiffuseSource(const IClipSource_SP& s) {
	_srcDiffuse = s;
}
float Clipmap::Layer::getScale() const {
	return _scale;
}
spn::RangeF Clipmap::Layer::getRange() const {
	return _srcElev->getRange();
}
IClipSource::Data Clipmap::Layer::getDataRect(const spn::Rect& r) {
	return Data(_elevation, r, getCacheSize());
}

Clipmap::Layer::IOfs Clipmap::Layer::GetDrawOffsetLocal(const spn::Vec2& center, const float ratio) {
	return IOfs(center, ratio);
}
spn::Size Clipmap::Layer::getCacheSize() const {
	return _elevation->get()->getSize();
}
bool Clipmap::Layer::isUpsamp() const {
	return _scale < 1.f;
}
void Clipmap::Layer::refresh(const int tileW, const int ox, const int oy,
							rs::IEffect& e,
							const rs::util::Rect01& rect01)
{
	std::cout << "refresh " << _scale << ": " << ox << ", " << oy << std::endl;
	using spn::rect::DivideRect;
	const auto ts = getCacheSize();
	const Size target(ts.width, ts.height);
	if(isUpsamp()) {
		DivideRect(
			ts,
			Rect(ox-4, ox+tileW+4, oy-4, oy+tileW+4),
			[&e, &rect01, this](const auto& r, const auto& rc){
				_upDrawCache(e, rect01, r, rc, false);
			}
		);
		DivideRect(
			ts,
			Rect(ox-1, ox+tileW+1, oy-1, oy+tileW+1),
			[&e, &rect01, this](const auto& r, const auto& rc){
				_upDrawCache(e, rect01, r, rc, true);
			}
		);
	} else {
		DivideRect(
			ts,
			Rect(ox-1, ox+tileW+1, oy-1, oy+tileW+1),
			[&e, &rect01, this](const auto& r, const auto& rc){
				_drawCache(e, rect01, r, rc);
			}
		);
	}
	_cx = ox;
	_cy = oy;
}
void Clipmap::Layer::incrementalRefresh(const int tileW, const int ox, const int oy,
										rs::IEffect& e,
										const rs::util::Rect01& rect01)
{
	const auto ts = getCacheSize();
	const Size target(ts.width, ts.height);
	using spn::rect::DivideRect;
	using spn::rect::IncrementalRect;
	if(isUpsamp()) {
		IncrementalRect(
			Rect(_cx-4, _cx+tileW+4, _cy-4, _cy+tileW+4),
			ox-_cx,
			oy-_cy,
			[this, &e, &rect01, &target](const Rect& req){
				DivideRect(
					target,
					req,
					[this, &e, &rect01](const Rect& r, const Rect& rc){
						_upDrawCache(e, rect01, r, rc, false);
					}
				);
			}
		);
		IncrementalRect(
			Rect(_cx-1, _cx+tileW+1, _cy-1, _cy+tileW+1),
			ox-_cx,
			oy-_cy,
			[this, &e, &rect01, &target](const Rect& req){
				DivideRect(
					target,
					req,
					[this, &e, &rect01](const Rect& r, const Rect& rc){
						_upDrawCache(e, rect01, r, rc, true);
					}
				);
			}
		);
	} else {
		IncrementalRect(
			Rect(_cx-1, _cx+tileW+1, _cy-1, _cy+tileW+1),
			ox-_cx,
			oy-_cy,
			[this, &e, &rect01, &target](const Rect& req){
				DivideRect(
					target,
					req,
					[this, &e, &rect01](const Rect& r, const Rect& rc){
						_drawCache(e, rect01, r, rc);
					}
				);
			}
		);
	}
	_cx = ox;
	_cy = oy;
}
namespace {
	enum RotationId {
		Rot_0,
		Rot_90,
		Rot_180,
		Rot_270
	};
	const Clipmap::M3 c_mR[] = {
		Clipmap::M3::RotationZ(spn::DegF(0)),
		Clipmap::M3::RotationZ(spn::DegF(90)),
		Clipmap::M3::RotationZ(spn::DegF(180)),
		Clipmap::M3::RotationZ(spn::DegF(270))
	};
}
void Clipmap::Layer::drawBlock12(const int bs, const CBf& cb) const {
	const Vec2 lb_ofs(-bs*2-1);
	const auto scr = _srcElev->getRange();
	auto fnBlock = [scr, lb_ofs, &cb, this](float x, float y){
		const auto bofs = Vec2(x,y)+lb_ofs;
		cb(_elevation, _normal, _srcDiffuse, EBlock, false, M3::Translation(bofs), _scale, scr);
	};
	const int outer_abs = bs*2+2;
	// 左下
	fnBlock(0,0);
	fnBlock(bs,0);
	fnBlock(0,bs);
	// 右下
	fnBlock(outer_abs,  0);
	fnBlock(outer_abs+bs, 0);
	fnBlock(outer_abs+bs, bs);
	// 左上
	fnBlock(0, outer_abs);
	fnBlock(0, outer_abs+bs);
	fnBlock(bs, outer_abs+bs);
	// 右上
	fnBlock(outer_abs, outer_abs+bs);
	fnBlock(outer_abs+bs, outer_abs+bs);
	fnBlock(outer_abs+bs, outer_abs);

	auto fnSide = [scr, lb_ofs, &cb, this](float x, float y, const bool bFlip, const int angId){
		const auto bofs = lb_ofs+Vec2(x,y);
		cb(_elevation, _normal, _srcDiffuse, ESide, bFlip, c_mR[angId] * M3::Translation(bofs), _scale, scr);
	};
	// サイド上
	fnSide(outer_abs, outer_abs+bs, true, Rot_90);
	// サイド左
	fnSide(0, bs*2, false, Rot_0);
	// サイド右
	fnSide(outer_abs+bs, bs*2, false, Rot_0);
	// サイド下
	fnSide(outer_abs, 0, true, Rot_90);
}
void Clipmap::Layer::drawLShape(const int bs, const IOfsP& inner, const CBf& cb) const {
	Assert(Trap, (inner.first==0 || inner.first==1) && (inner.second==0 || inner.second==1))

	const int outer_abs = bs*2+2;
	int rot;
	Vec2 ofs;
	bool bFlip;
	switch(inner.first | (inner.second << 1)) {
		// 左下
		case 0b00:
			rot = Rot_90;
			ofs = Vec2(outer_abs, 0);
			bFlip = true;
			break;
		// 右下
		case 0b01:
			rot = Rot_180;
			ofs = Vec2(outer_abs, outer_abs);
			bFlip = false;
			break;
		// 左上
		case 0b10:
			rot = Rot_0;
			ofs = Vec2(0, 0);
			bFlip = false;
			break;
		// 右上
		case 0b11:
			rot = Rot_270;
			ofs = Vec2(0, outer_abs);
			bFlip = true;
			break;
	}
	const Vec2 bs_ofs(-bs-1);
	const Vec2 bofs(bs_ofs + ofs);
	const auto scr = _srcElev->getRange();
	cb(_elevation, _normal, _srcDiffuse, ELShape, bFlip, c_mR[rot] * M3::Translation(bofs), _scale, scr);
}
// Layer0 = Block*4 + LShape*2 + Block*12 + Side*4
void Clipmap::Layer::drawLayer0(const int bs, const CBf& cb) const {
	// 周囲: Block*12 + Side*4
	drawBlock12(bs, cb);
	// 内側: LShape*2 + Block*4
	// LShape*2 [ AABB ]
	drawLShape(bs, {0,0}, cb);
	drawLShape(bs, {1,1}, cb);

	const auto scr = _srcElev->getRange();
	// Block*4 [ 常に描画 ]
	auto fnBlock = [scr, &cb, this](float x, float y){
		cb(_elevation, _normal, _srcDiffuse, EBlock, false, M3::Translation({x,y}), _scale, scr);
	};
	fnBlock(-bs, -bs);
	fnBlock(-bs, 0);
	fnBlock(0, -bs);
	fnBlock(0, 0);

	// Degeneration [ 常に描画 ]
	const Vec2 diff(-bs*2-1, -bs*2-1);
	cb(_elevation, _normal, _srcDiffuse, EDegeneration, false, M3::Translation(diff), _scale, scr);
}
// Layer = Block*12 + Side*4 + LShape*1 + Inner-Degeneration
void Clipmap::Layer::drawLayerN(const Vec2& center, const int bs, const float scale, const CBf& cb) const {
	auto iofs = GetDrawOffsetLocal(center, _scale * scale);
	// Block*12 + Side*4 [ AABB ]
	drawBlock12(bs, cb);
	// LShape [ AABB ]
	drawLShape(bs, {iofs.ix, iofs.iy}, cb);
	// Degeneration [ 常に描画 ]
	const Vec2 diff(-bs*2-1, -bs*2-1);
	const auto scr = _srcElev->getRange();
	cb(_elevation, _normal, _srcDiffuse, EDegeneration, false, M3::Translation(diff), _scale, scr);
}

namespace {
	//! Rectで示される矩形をVec4に変換
	template <class T>
	auto RectVec4(const T& r) {
		return spn::Vec4(r.width(), r.height(), r.x0, r.y0);
	}
	//! UV矩形の拡大縮小
	/*!
		\param[in]		uv			基準となるUV矩形(XScale, YScale, XOffset, YOffset)
		\param[in]		unit		1テクセルの幅
		\param[in]		w			矩形スケールの変異幅(1=1テクセル) Width
		\param[in]		h			矩形スケールの変異幅(1=1テクセル) Height
		\param[in]		mv_w		矩形オフセットの移動幅(1=1テクセル) X
		\param[in]		mv_h		矩形オフセットの移動幅(1=1テクセル) Y
	*/
	auto UVShrink(const spn::Vec4& uv, const spn::Vec2& unit, float w, float h, float mv_w, float mv_h) {
		return uv + spn::Vec4(-unit.x*w,
								-unit.y*h,
								unit.x*mv_w,
								unit.y*mv_h);
	}
	//! UV矩形の拡大縮小(簡易バージョン)
	/*!
		\param[in]		uv			基準となるUV矩形(XScale, YScale, XOffset, YOffset)
		\param[in]		unit		1テクセルの幅
		\param[in]		w			矩形スケールの変異幅(1=1テクセル) Width
		\param[in]		h			矩形スケールの変異幅(1=1テクセル) Height
	*/
	auto UVShrink(const spn::Vec4& uv, const spn::Vec2& unit, float w, float h) {
		return UVShrink(uv, unit, w*2, h*2, w, h);
	}
}
void Clipmap::Layer::_upDrawCache(
		rs::IEffect& e,
		const rs::util::Rect01& rect01,
		const Rect& rectG,
		const Rect& rectL,
		const bool bNormal
) {
	const auto rectLF = rectL.toRect<float>();
	// あくまでテクセル%2をずらすだけ
	auto lf = RectVec4(rectLF);
	lf += Vec4(0,0,1,1);
	rs::HLFb fb0 = e.getFramebuffer();
	auto fb = mgr_gl.makeFBuffer();

	if(!bNormal) {
		auto tr = rectG;
		using spn::rect::LoopValueD;
		// シェーダーに渡すのはかさ乗せしてない範囲なので補正
		tr.x0 = LoopValueD(tr.x0, 2);
		tr.x1 = LoopValueD(tr.x1, 2);
		tr.y0 = LoopValueD(tr.y0, 2);
		tr.y1 = LoopValueD(tr.y1, 2);

		tr.expand(1);
		const auto data = _srcElev->getDataRect(tr);
		auto uvr = UVShrink(data.uvrect, data.unit, 1,1);
		uvr.z -= data.unit.x/4;
		uvr.w -= data.unit.y/4;
		e.setTechPassId(T_Upsampling);
		e.setUniform(U_Elevation, data.tex);
		e.setUniform(U_SrcUVUnit, data.unit);
		e.setUniform(U_SrcUVRect, uvr);
		e.setUniform(U_DestTexelRect, lf);
		fb->get()->attachTexture(rs::GLFBufferCore::Att::COLOR0, _elevation);
		e.setFramebuffer(fb);
		// Normalの関係上、周囲3マス分計算
		e.setViewport(true, rectLF);
		rect01.draw(e);
	}
	else {
		auto tr = rectG;
		tr.expand(3);
		const auto data = getDataRect(tr);
		auto uvr = UVShrink(data.uvrect, data.unit, 3,3);
		uvr.z -= data.unit.x/4;
		uvr.w -= data.unit.y/4;
		e.setTechPassId(T_MakeNormal);
		e.setUniform(U_Elevation, data.tex);
		e.setUniform(U_SrcUVUnit, data.unit);
		e.setUniform(U_SrcUVRect, uvr);
		e.setUniform(U_DestTexelRect, lf);
		e.setUniform(U_Ratio, _scale);
		fb->get()->attachTexture(rs::GLFBufferCore::Att::COLOR0, _normal);
		e.setFramebuffer(fb);
		e.setViewport(true, rectLF);
		rect01.draw(e);
	}
	e.setFramebuffer(fb0);
}
void Clipmap::Layer::_drawCache(
		rs::IEffect& e,
		const rs::util::Rect01& rect01,
		const Rect& rectG,
		const Rect& rectL
) {
	const auto rectLF = rectL.toRect<float>();
	// あくまでテクセル%2をずらすだけ
	auto lf = RectVec4(rectLF);
	lf += Vec4(0,0,1,1);
	rs::HLFb fb0 = e.getFramebuffer();
	auto fb = mgr_gl.makeFBuffer();

	// e.setTechPassId(T_SamplingD);
	// {
	// 	const IClipSource::Data data = _srcDiffuse->getDataRect(rectG);
	// 	e.setUniform(U_Elevation, data.tex);
	// 	e.setUniform(U_SrcUVRect, data.uvrect);
	// 	fb->get()->attachTexture(rs::GLFBufferCore::Att::COLOR0, _diffuse);
	// 	e.setFramebuffer(fb);
	// 	e.setViewport(true, rectLF);
	// 	rect01.draw(e);
	// }

	// Normal(or shrink) sampling
	e.setTechPassId(T_Sampling);
	auto tr = rectG;
	tr.expand(4);
	const IClipSource::Data data = _srcElev->getDataRect(tr);
	// シェーダーに渡すのはかさ乗せしてない範囲なので補正
	auto uvr = UVShrink(data.uvrect, data.unit, 4.f, 4.f);
	// Compute Elevation
	{
		// coarserの計算するのに周囲+3のソースが必要
		e.setUniform(U_Elevation, data.tex);
		e.setUniform(U_SrcUVUnit, data.unit);
		e.setUniform(U_SrcUVRect, uvr);
		e.setUniform(U_DestTexelRect, lf);
		fb->get()->attachTexture(rs::GLFBufferCore::Att::COLOR0, _elevation);
		e.setFramebuffer(fb);
		e.setViewport(true, rectLF);
		rect01.draw(e);
	}
	if(rectL.width() <= 1) {
		e.setFramebuffer(fb0);
		return;
	}

	// 法線やcoarserをGPUで計算する為に周囲の2頂点分多くソースを確保
	// Compute normal
	e.setTechPassId(T_MakeNormal);
	{
		e.setUniform(U_Elevation, data.tex);
		e.setUniform(U_SrcUVUnit, data.unit);
		e.setUniform(U_SrcUVRect, uvr);
		e.setUniform(U_DestTexelRect, lf);
		e.setUniform(U_Ratio, _scale);
		fb->get()->attachTexture(rs::GLFBufferCore::Att::COLOR0, _normal);
		e.setFramebuffer(fb);
		e.setViewport(true, rectLF);
		rect01.draw(e);
	}
	e.setFramebuffer(fb0);
}
rs::HTex Clipmap::Layer::getCache() const { return _elevation; }
rs::HTex Clipmap::Layer::getNormalCache() const { return _normal; }
