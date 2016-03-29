// #include "../glx_if.hpp"
// --------------------- Clipmap::Layer ---------------------
// Clipmap::Layer::Layer(const spn::Size s, const spn::PowInt samp_ratio):
// 	_cx(-1e8),
// 	_cy(-1e8),
// 	_samp_ratio(samp_ratio)
// {
// 	const spn::PowSize ps(s.width+1, s.height+1);
// 	_normal = mgr_gl.createTexture(ps, GL_RGBA8, false, false);
// 	_normal->get()->setWrap(rs::WrapState::Repeat);
// 	_elevation = mgr_gl.createTexture(ps, GL_RG16F, false, false);
// 	_elevation->get()->setWrap(rs::WrapState::Repeat);
//
// // 	std::vector<Vec2> data;
// // 	if(samp_ratio < 0x100) {
// // 		auto data_ = MakeTestHeight(ps/2, samp_ratio);
// // 		data = MakeUpsample(ps/2, data_);
// // 	} else {
// // 		data = MakeTestHeight(ps, samp_ratio);
// // 	}
// // 	auto dataNml = MakeNormal(ps, data, samp_ratio);
// // 	auto* el = static_cast<rs::Texture_Mem*>(_elevation->get());
// // 	el->writeData({data.data(), data.size()*sizeof(Vec2)}, GL_FLOAT);
// // 	el = static_cast<rs::Texture_Mem*>(_normal->get());
// // 	el->writeData({dataNml.data(), dataNml.size()*sizeof(Vec4)}, GL_FLOAT);
//
// 	std::vector<Vec2> data(ps.width*ps.height);
// 	for(auto& d : data)
// 		d = Vec2(0.1f);
// 	auto* el = static_cast<rs::Texture_Mem*>(_elevation->get());
// 	el->writeData({data.data(), data.size()*sizeof(Vec2)}, GL_FLOAT);
// 	std::vector<Vec4> data2(ps.width*ps.height);
// 	for(auto& d : data2)
// 		d = Vec4(0.1f);
// 	el = static_cast<rs::Texture_Mem*>(_normal->get());
// 	el->writeData({data2.data(), data2.size()*sizeof(Vec4)}, GL_FLOAT);
// }
// void Clipmap::Layer::setHeightMap(const IClipSource_SP& src) {
// 	_source = src;
// }
