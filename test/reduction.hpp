#pragma once
#include "../util/screen.hpp"

class Reduction : public rs::DrawableObjT<Reduction> {
	private:
		static const rs::IdValue T_Reduct2,
								T_Reduct4;
		rs::HLTex				_hlSource;
		bool					_bFour;
		mutable rs::HLTex		_hlResult;
		mutable rs::HLFb		_hlFb;
		rs::util::ScreenRect	_rect;

		rs::IdValue _getTechId() const;
		int _getRatio() const;
	public:
		Reduction(rs::Priority p, bool bFour);
		void setSource(rs::HLTex h);
		rs::HTex getResult() const;
		void onDraw(rs::IEffect& e) const override;
};
DEF_LUAIMPORT(Reduction)
