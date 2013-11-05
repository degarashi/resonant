#include "font_base.hpp"

namespace rs {
	// ------------------- CharPlane -------------------
	CharPlane::CharPlane(const spn::PowSize& size, int fh, UPLaneAlloc&& a):
		_sfcSize(size), _fontH(fh), _lalloc(std::move(a)), _nUsed(0)
	{
		_dV = static_cast<float>(fh) / _sfcSize.height;
		_nH = static_cast<int>(spn::Rcp22Bit(_dV));
	}
	CharPlane::CharPlane(CharPlane&& cp): _plane(std::move(cp._plane)), _sfcSize(cp._sfcSize), _fontH(cp._fontH),
		_lalloc(std::move(cp._lalloc)), _nUsed(cp._nUsed), _dV(cp._dV)
	{
		cp._lalloc.reset(nullptr);
	}
	void CharPlane::_addCacheTex() {
		// OpenGLテクスチャ確保
		HLTex hlTex = mgr_gl.createTexture(_sfcSize, GL_RGBA8, true);	// DeviceLost時: 内容のリストア有りで初期化
		HTex hTex = hlTex.get();
		_plane.push_back(std::move(hlTex));
		// Lane登録
		spn::Rect rect(0,_sfcSize.width, 0,_fontH);
		for(int i=0 ; i<_nH ; i++) {
			_lalloc->addFreeLane(hTex, rect);
			rect.addOffset(0, _fontH);
		}
	}
	void CharPlane::rectAlloc(LaneRaw& dst, int width) {
		++_nUsed;
		if(!_lalloc->alloc(dst, width)) {
			// 新しく領域を確保して再トライ
			_addCacheTex();
			Assert(Trap, _lalloc->alloc(dst, width))
		}
	}
	const spn::PowSize& CharPlane::getSurfaceSize() const {
		return _sfcSize;
	}
}
