#pragma once
#include "spinner/vector.hpp"
#include <memory>
#include <string>

namespace rs {
	class VDecl;
	using SPVDecl = std::shared_ptr<VDecl>;

	template <class>
	class DrawDecl;
}
//! VDeclメソッド定義
#define DefineVDecl(etag) \
	namespace rs { \
		template <> \
		class DrawDecl<etag> { \
			public: \
				static const SPVDecl& GetVDecl(); \
		}; \
	}

