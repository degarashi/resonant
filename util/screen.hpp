#pragma once
#include "screenrect.hpp"
#include "../updater.hpp"
#include "../glresource.hpp"

namespace rs {
	namespace util {
		class PostEffect : public DrawableObjT<PostEffect> {
			private:
				using base_t = DrawableObjT<PostEffect>;
				using TPair = std::pair<IdValue, HLTex>;
				using TPairV = std::vector<TPair>;
				IdValue		_idTech;
				TPairV		_texture;
				float		_alpha;
				ScreenRect	_rect;
			public:
				PostEffect(IdValue idTech, Priority dprio);
				void setAlpha(float a);
				void setTexture(IdValue id, HTex hTex);
				void onDraw(IEffect& e) const override;
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
