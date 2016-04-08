#include "clipsource.hpp"
#include "../glresource.hpp"
#include "spinner/rectdiff.hpp"
#include "spinner/random.hpp"

// -------- IClipSource --------
IClipSource::Data::Data(rs::HTex t, const spn::RectF& r, const spn::SizeF& s):
	tex(t),
	uvrect(r.width() / s.width,
			r.height() / s.height,
			r.x0 / s.width,
			r.y0 / s.height),
	unit(1 / s.width,
		1 / s.height)
{}
IClipSource::Data::Data(rs::HTex t, const spn::Rect& r, const spn::Size& s):
	Data(t, r.toRect<float>(), s.toSize<float>())
{}

// -------- ClipTexSource --------
IClipSource_SP ClipTexSource::Create(rs::HTex t) {
	return std::make_shared<ClipTexSource>(t);
}
ClipTexSource::ClipTexSource(rs::HTex t):
	_texture(t)
{
	t->get()->setWrap(rs::WrapState::Repeat);
}
spn::RangeF ClipTexSource::getRange() const {
	return {0, 1.f};
}
IClipSource::Data ClipTexSource::getDataRect(const spn::Rect& r) {
	return Data(_texture, r, _texture->get()->getSize());
}

// -------- ClipPNSource --------
IClipSource_SP ClipPNSource::Create(const HashVec_SP& sp,
							const int tsize,
							const int freq)
{
	return std::make_shared<ClipPNSource>(sp, tsize, freq);
}
ClipPNSource::ClipPNSource(const HashVec_SP& sp,
							const spn::PowInt tsize,
							const int freq):
	_hash(sp),
	_freq(freq)
{
	_hTex = mgr_gl.createTexture({tsize, tsize}, GL_R16F, false, false);
	_hTex->get()->setWrap(rs::WrapState::Repeat);
}
spn::RangeF ClipPNSource::getRange() const {
	return _hash->getRange();
}
IClipSource::Data ClipPNSource::getDataRect(const spn::Rect& r) {
	// 書き込むデータ範囲の分割
	auto* ptex = static_cast<rs::Texture_Mem*>(_hTex->get());
	const auto sz = _hTex->get()->getSize();
	spn::rect::DivideRect(
		spn::Size(sz.width, sz.height),
		r,
		[ptex, this](const auto& r, const auto& rc) {
			// 書き込むデータの生成
			std::vector<float>	data(r.width() * r.height());
			auto* p = data.data();
			for(int i=r.y0 ; i<r.y1 ; i++) {
				for(int j=r.x0 ; j<r.x1 ; j++) {
					const float val = _hash->getElev((j+1)*_freq, (i+1)*_freq);
					*p++ = val;
				}
			}
			AssertP(Trap, p==data.data()+data.size())
			ptex->writeRect({data.data(), data.size()*sizeof(float)}, rc, GL_FLOAT);
		}
	);
	return Data(_hTex, r, sz);
}

// -------- ClipTestSource --------
ClipTestSource::ClipTestSource(const float fH, const float fV, const spn::PowSize size, const float aux):
	_freqH(fH),
	_freqV(fV),
	_aux(aux)
{
	_hTex = mgr_gl.createTexture(size, GL_R16F, false, false);
	_hTex->get()->setWrap(rs::WrapState::Repeat);
	std::vector<float> data(size.width*size.height);
	for(auto& h : data)
		h = 0;
	((rs::Texture_Mem*)_hTex->get())->writeData({data.data(), data.size()*sizeof(float)}, GL_FLOAT);
}
spn::RangeF ClipTestSource::getRange() const {
	return {-1.f, 1.f};
}
void ClipTestSource::save(const std::string& path) const {
	_hTex->get()->save(path);
}
IClipSource::Data ClipTestSource::getDataRect(const spn::Rect& r) {
	// 書き込むデータ範囲の分割
	auto* ptex = static_cast<rs::Texture_Mem*>(_hTex->get());
	const auto sz = _hTex->get()->getSize();
	spn::rect::DivideRect(
		spn::Size(sz.width,sz.height),
		r,
		[ptex, this](const auto& r, const auto& rc) {
			// 書き込むデータの生成
			std::vector<float>	data(r.width() * r.height());
			auto* p = data.data();
			for(int i=r.y0 ; i<r.y1 ; i++) {
				for(int j=r.x0 ; j<r.x1 ; j++) {
					*p++ = TestElev(
								1, 1,
								(j+_aux) * _freqH,
								(i+_aux) * _freqV
							);
				}
			}
			AssertP(Trap, p==data.data()+data.size())
			ptex->writeRect({data.data(), data.size()*sizeof(float)}, rc, GL_FLOAT);
		}
	);
	return Data(_hTex, r, sz);
}
float ClipTestSource::TestElev(const int w, const float ratio, const float x, const float y) {
	return std::sin(x * ratio*2 * 2*spn::PI / w)/2 +
			std::sin(y * ratio*2 * 2*spn::PI / w)/2;
}
