#include "font_base.hpp"

namespace rs {
	// ------------------- CharPlane -------------------
	CharPlane::CharPlane(const spn::PowSize& size, int fh, UPLaneAlloc&& a):
		_sfcSize(size), _fontH(fh), _lalloc(std::move(a)), _nUsed(0)
	{
		_dV = static_cast<float>(fh) / _sfcSize.height;
		_nH = static_cast<int>(spn::Rcp22Bit(_dV));
	}
	void CharPlane::_addCacheTex() {
		// OpenGLテクスチャ確保
		// MEMO: 環境によってはGL_RGBAが32bitでないかもしれないので対策が必要
		HLTex hlTex = mgr_gl.createTexture(_sfcSize, GL_RGBA, true, true);	// DeviceLost時: 内容のリストア有りで初期化
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
