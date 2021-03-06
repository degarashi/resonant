#pragma once
#include "screenrect.hpp"
#include "../updater.hpp"
#include "../glresource.hpp"

namespace rs {
	namespace util {
		class PostEffect : public DrawableObjT<PostEffect> {
			private:
				using base_t = DrawableObjT<PostEffect>;
				using ParamF = std::function<void (IEffect&)>;
				using ParamFV = std::vector<std::pair<IdValue, ParamF>>;
				IdValue			_idTech;
				ParamFV			_param;
				Rect11			_rect11;
				spn::RectF		_drawRect;

				void _applyParam(IEffect& e) const;
			public:
				const static IdValue U_RectScale;
				PostEffect(IdValue idTech, Priority dprio);
				void setTechPassId(IdValue idTech);
				template <class T>
				void setParam(IdValue id, const T& t) {
					setParamFunc(id, [id,tv=t](auto& e){ e.setUniform(id, tv); });
				}
				void setParamFunc(IdValue id, const ParamF& f);
				void setRect(const spn::RectF& r);
				void clearParam();
				void onDraw(IEffect& e) const override;
		};

		class Viewport : public DrawableObjT<Viewport> {
			private:
				//! 矩形サイズ指定
				/*! true:	ピクセル
					false:	画面比率 */
				bool		_bPixel;
				spn::RectF	_rect;
			public:
				Viewport(Priority dprio);
				void onDraw(IEffect& e) const override;
				void setByRatio(const spn::RectF& r);
				void setByPixel(const spn::RectF& r);
		};
		class FBSwitch : public DrawableObjT<FBSwitch> {
			private:
				HLFb			_hlFb;
				using ClearParam_OP = spn::Optional<draw::ClearParam>;
				ClearParam_OP	_cparam;
			public:
				FBSwitch(Priority dprio, HFb hFb, const ClearParam_OP& p=spn::none);
				void onDraw(IEffect& e) const override;
				void setClearParam(const ClearParam_OP& p);
		};
		class FBClear : public DrawableObjT<FBClear> {
			private:
				draw::ClearParam _param;
			public:
				FBClear(Priority dprio,
						const draw::ClearParam& p);
				void onDraw(IEffect& e) const override;
		};
	}
}
#include "../luaimport.hpp"
DEF_LUAIMPORT(rs::util::FBSwitch)
DEF_LUAIMPORT(rs::util::FBClear)
