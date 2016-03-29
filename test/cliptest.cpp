#include "clipmap.hpp"
#include "../glresource.hpp"
#include "../glx_if.hpp"
#include "spinner/rectdiff.hpp"

namespace {
	auto pack(float x, float y, float range) {
		return x + y/range;
	}
	auto unpack(float v, float range) {
		return spn::Vec2(
					std::floor(v),
					std::fmod(v, 1.f) * range
				);
	}
	auto packS(float x, float y, float range) {
		return (x+range/2) + (y+range/2)/range;
	}
	auto unpackS(float v, float range) {
		return spn::Vec2(
					std::floor(v) - range/2,
					std::fmod(v, 1.f) * range - range/2
				);
	}
	auto packU(float x, float y, float range, float d_range) {
		float d = std::floor(y - x);
		d = spn::Saturate(d, d_range-1);
		return (x+range/2) + (d+d_range/2)/d_range;
	}
	auto elevLerp(int w, float ratio, float x, float y, int dx, int dy) {
		return (Clipmap::Layer::TestElev(w, ratio, x-dx, y-dy) +
				Clipmap::Layer::TestElev(w, ratio, x+dx, y+dy)) /2;
	};
}
float Clipmap::Layer::TestElev(int w, float ratio, float x, float y) {
	return std::round(std::sin(x * ratio*2 * 2*spn::PI / w) * 63) +
			std::round(std::sin(y * ratio*2 * 2*spn::PI / w) * 63);
}
void Clipmap::save(const spn::PathBlock& pb) const {
	auto pbt = pb;
	int count=0;
	for(auto& l : _layer) {
		pbt <<= (boost::format("layer_%1%.png") % count).str();
		l->save(pbt.plain_utf8());
		pbt.popBack();
		++count;
	}
}
void Clipmap::Layer::save(const std::string& path) const {
	_elevation->get()->save(path);
}
std::vector<spn::Vec4> Clipmap::Layer::MakeNormal(const spn::PowSize s, const std::vector<Vec2>& src, const float ratio) {
	auto elev = [&src, &s](int x, int y){
		// loop
		if(x < 0)
			x += s.width;
		else if(x >= int(s.width))
			x -= s.width;

		if(y < 0)
			y += s.height;
		else if(y >= int(s.height))
			y -= s.height;
		return src[y*s.width + x].x;
	};
	auto nml = [&elev, r=ratio](int x, int y, int diff=1) {
		const auto	eC = elev(x,y),
					eU = elev(x,y+diff),
					eR = elev(x+diff,y);
		spn::Vec3	vH(r*diff,0,eR-eC),
					vV(0,r*diff,eU-eC);
		vH.z /= 64;
		vV.z /= 64;
		vH.normalize();
		vV.normalize();
		return vH.cross(vV).normalization();
	};
	auto nmlLerp = [&nml](int x, int y, int dx, int dy) {
		return (nml(x-dx, y-dy, 2) + nml(x+dx, y+dy, 2)).normalization();
	};

	std::vector<Vec4> dataNml(s.width*s.height*sizeof(Vec4));
	auto* pDstNml = dataNml.data();
	for(int i=0 ; i<int(s.height) ; i++) {
		for(int j=0 ; j<int(s.width) ; j++) {
			const Vec3 n0 = nml(j, i);
			Vec3 n_c;
			switch((j&1) | ((i&1)<<1)) {
				case 0b00:
					// 一段荒い法線
					n_c = nml(j, i, 2);
					break;
				case 0b01:
					// 左右の補間
					n_c = nmlLerp(j, i, 1, 0);
					break;
				case 0b10:
					// 上下の補間
					n_c = nmlLerp(j, i, 0, 1);
					break;
				case 0b11:
					// 斜め(LT -> RB)の補間
					n_c = nmlLerp(j, i, 1, 1);
					break;
			}
			*pDstNml++ = Vec4(n0.x, n0.y, n_c.x, n_c.y);
		}
	}
	return dataNml;
}
std::vector<spn::Vec2> Clipmap::Layer::MakeUpsample(const spn::PowSize ps, const std::vector<Vec2>& src) {
	const auto ps2 = ps*2;
	std::vector<Vec2> data(ps2.width * ps2.height * sizeof(Vec2));
	auto* pDst = data.data();
	auto elev = [ps, &src](int x, int y) {
		using spn::rect::LoopValue;
		return src[LoopValue<int>(y, ps.height) * ps.width + LoopValue<int>(x, ps.width)].x;
	};
	auto elevLerp = [&elev](int x, int y, int dx, int dy) {
		return (elev(x, y) + elev(x+dx, y+dy))/2;
	};
	auto upsamplingA = [&elev](int x, int y, int dx, int dy){
		const float a = -1/16.f,
					b = (8+1)/16.f;
		return elev(x, y) * b +
				elev(x+dx, y+dy) * b +
				elev(x-dx, y-dy) * a +
				elev(x+dx*2, y+dy*2) * a;
	};
	auto upsamplingB = [&elev](int x, int y){
		const float a = -1/16.f,
					b = (8+1)/16.f;
		const float sigma = a*a,
					micro = a*b,
					nu = b*b;
		const float
			v0(elev(x+1, y+1) +
				elev(x, y) +
				elev(x+1, y) +
				elev(x, y+1)),
			v1(elev(x+2, y+2) +
				elev(x-1, y-1) +
				elev(x+2, y-1) +
				elev(x-1, y+2)),
			v2(elev(x, y+2) +
				elev(x+1, y+2) +
				elev(x, y-1) +
				elev(x+1, y-1) +
				elev(x-1, y+1) +
				elev(x+2, y+1) +
				elev(x-1, y) +
				elev(x+2, y));
		return v0*nu + v1*sigma + v2*micro;
	};
	for(int i=0 ; i<int(ps2.height) ; i++) {
		for(int j=0 ; j<int(ps2.width) ; j++) {
			Vec2 e;
			const int y = i/2,
						x = j/2;
			switch((j&1) | ((i&1)<<1)) {
				case 0b00:
					// アップサンプリングなし
					e = Vec2(elev(x,y));
					break;
				case 0b01:
					// 左右の補間
					e = Vec2(upsamplingA(x,y, 1,0),
							elevLerp(x,y, 1,0));
					break;
				case 0b10:
					// 上下の補間
					e = Vec2(upsamplingA(x,y, 0,1),
							elevLerp(x,y, 0,1));
					break;
				case 0b11:
					// 斜め(LT -> RB)の補間
					e = Vec2(upsamplingB(x,y),
							elevLerp(x,y, 1,1));
					break;
			}
			*pDst++ = e;
		}
	}
	return data;
}
std::vector<spn::Vec2> Clipmap::Layer::MakeTestHeight(const spn::PowSize ps, const float ratio) {
	std::vector<Vec2> data(ps.width*ps.height*sizeof(Vec2));
	auto* pDst = data.data();
	for(int i=0 ; i<int(ps.height) ; i++) {
		const int rel_y = i-(ps.height/2);
		for(int j=0 ; j<int(ps.width) ; j++) {
			const int rel_x = j-(ps.width/2);
			const float e0 = TestElev(ps.width, ratio, rel_x, rel_y);
			float d_c;
			switch((j&1) | ((i&1)<<1)) {
				case 0b00:
					// 補間なし
					d_c = e0;
					break;
				case 0b01:
					// 左右の補間
					d_c = elevLerp(ps.width, ratio, rel_x, rel_y, 1, 0);
					break;
				case 0b10:
					// 上下の補間
					d_c = elevLerp(ps.width, ratio, rel_x, rel_y, 0, 1);
					break;
				case 0b11:
					// 斜め(LT -> RB)の補間
					d_c = elevLerp(ps.width, ratio, rel_x, rel_y, 1, 1);
					break;
			}
			*pDst++ = Vec2(e0, d_c);
		}
	}
	return data;
}
// void Clipmap::Layer::Test(const spn::PowSize s, const spn::PowInt samp_ratio, rs::IEffect& e) {
// 	static auto source = std::make_shared<ClipTestSource>(2.f, 2.f, s*2, 0);
// 	static auto hlTgt = mgr_gl.createTexture(s, GL_RGBA16F, false, false);
// 	static int count = 0;
// 	if(count == 0) {
// 		rs::HLFb fb = mgr_gl.makeFBuffer();
// 		fb->get()->attachTexture(rs::GLFBuffer::Att::COLOR0, hlTgt);
// 		auto fb0 = e.getFramebuffer();
// 		e.setFramebuffer(fb);
// 		e.setTechPassId(T_Sampling);
// 		auto src = source->getDataRect({s.width,s.width*2, 0,s.height*2});
// 		e.setUniform(U_Elevation, src.tex);
// 		e.setUniform(U_SrcUVRect, src.uvrect);
// 		e.setUniform(U_SrcUVUnit, src.unit*2);
// 		e.setUniform(U_DestTexelRect, Vec4(s.width/2, s.height, s.width/2, 0));
// 		e.setViewport(false, {0.5f,1, 0,1});
// 		rs::util::Rect01().draw(e);
// 		e.setFramebuffer(fb0);
// 	} else if(count == 3) {
// 		auto height = MakeTestHeight(s, samp_ratio*2);
// 		auto data = hlTgt->get()->readData(GL_RG, GL_FLOAT, 0);
// 		auto* result = (const Vec2*)data.data();
// 		for(int i=0 ; i<int(s.height) ; i++) {
// 			for(int j=0 ; j<int(s.width) ; j++) {
// 				auto& gpu = result[i*s.width+j];
// 				auto& cpu = height[i*s.width+j];
// 				std::cout << gpu.distance(cpu) << std::endl;
// 			}
// 		}
// 	}
// 	++count;

// 	static auto height = MakeTestHeight(s, samp_ratio);
// 	static rs::HLTex hlSrc = mgr_gl.createTextureInit(s, GL_RG16F, false, false, GL_FLOAT, {height.data(), height.size()*sizeof(Vec2)});
// 	hlSrc->get()->setWrap(rs::WrapState::Repeat);
// 	hlSrc->get()->setLinear(false);
// 	static rs::HLTex hlTgt = mgr_gl.createTexture(s, GL_RGBA16F, false, false);
// 	static int count = 0;
// 	if(count == 0) {
// 		rs::HLFb fb = mgr_gl.makeFBuffer();
// 		fb->get()->attachTexture(rs::GLFBuffer::Att::COLOR0, hlTgt);
//
// 		auto fb0 = e.getFramebuffer();
// 		e.setFramebuffer(fb);
// 		e.setTechPassId(T_MakeNormal);
// 		const Vec2 unit(1.f/s.width, 1.f/s.height);
// 		e.setUniform(U_SrcUVUnit, unit);
// 		e.setUniform(U_SrcUVRect, Vec4(1, 1, 0, 0));
// 		e.setUniform(U_DestTexelRect, Vec4(s.width,s.height,0,0));
// 		e.setUniform(U_Elevation, hlSrc);
//
// 		rs::util::Rect01().draw(e);
// 		e.setFramebuffer(fb0);
// 	} else if(count == 3) {
// 		auto restoreZ = [](float x, float y){
// 			Vec3 res(x, y, 0);
// 			res.z = std::sqrt(1.f - x*x - y*y);
// 			return res;
// 		};
// 		auto dataNml = MakeNormal(s, height, samp_ratio);
// 		auto data = hlTgt->get()->readData(GL_RGBA, GL_FLOAT, 0);
// 		auto* result = (const Vec4*)data.data();
// 		for(int i=0 ; i<s.height ; i++) {
// 			for(int j=0 ; j<s.width ; j++) {
// 				auto& gpu = result[i*s.width+j];
// 				auto gpuv = restoreZ(gpu.x, gpu.y);
// 				auto gpuc = restoreZ(gpu.z, gpu.w);
// 				auto& cpu = dataNml[i*s.width+j];
// 				auto cpuv = restoreZ(cpu.x, cpu.y);
// 				auto cpuc = restoreZ(cpu.z, cpu.w);
// 				std::cout << gpuv.distance(cpuv) << std::endl;
// 			}
// 		}
// 	}
// 	++count;
// }
