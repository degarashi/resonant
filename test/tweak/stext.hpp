#pragma once
#include "handle.hpp"
#include "font.hpp"

namespace tweak {
	class StaticText {
		private:
			using TextV = std::vector<rs::HLText>;
			TextV	_text;
		public:
			StaticText(rs::CCoreID cid, int n, const std::string* str);
			rs::HText getText(int t) const;
	};
	struct STextPack : StaticText {
		enum EType {
			Type,
			Step,
			Base,
			Initial,
			Current,
			_Num
		};
		STextPack(rs::CCoreID cid);
	};
}
