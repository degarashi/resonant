#include "stext.hpp"

namespace tweak {
	// ---------------- StaticText ----------------
	StaticText::StaticText(rs::CCoreID cid, const int n, const std::string* str) {
		_text.resize(n);
		for(int i=0 ; i<n ; i++)
			_text[i] = mgr_text.createText(cid, str[i]);
	}
	rs::HText StaticText::getText(const int t) const {
		return _text[t];
	}
	// ---------------- STextPack ----------------
	namespace {
		const std::string cs_typename[] = {
			"Type",
			"Step",
			"Base",
			"Initial",
			"Current"
		};
	}
	STextPack::STextPack(const rs::CCoreID cid):
		StaticText(cid, countof(cs_typename), cs_typename)
	{}
}
