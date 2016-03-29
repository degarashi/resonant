#include "clipmap.hpp"
#include "../glresource.hpp"
#include "spinner/rectdiff.hpp"

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
ClipTexSource::ClipTexSource(rs::HTex t):
	_texture(t)
{
	t->get()->setWrap(rs::WrapState::Repeat);
}
spn::Size ClipTexSource::getSize() const {
	return _texture->get()->getSize();
}
IClipSource::Data ClipTexSource::getDataRect(const spn::Rect& r) {
	return Data(_texture, r, getSize());
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
spn::Size ClipTestSource::getSize() const {
	return {0,0};
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
					*p++ = Clipmap::Layer::TestElev(
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
