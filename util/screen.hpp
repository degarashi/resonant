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
				TPairV		_texture;
				float		_alpha;
				ScreenRect	_rect;
			public:
				PostEffect(Priority dprio);
				void setAlpha(float a);
				void setTexture(IdValue id, HTex hTex);
				void onDraw(GLEffect& e) const override;
		};
		class FBSwitch : public DrawableObjT<FBSwitch> {
			private:
				HLFb		_hlFb;
			public:
				FBSwitch(Priority dprio, HFb hFb);
				void onDraw(GLEffect& e) const override;
		};
		class FBClear : public DrawableObjT<FBClear> {
			private:
				draw::ClearParam _param;
			public:
				FBClear(Priority dprio,
						const draw::ClearParam& p);
				void onDraw(GLEffect& e) const override;
		};
	}
}
