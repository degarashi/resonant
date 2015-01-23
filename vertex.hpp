#pragma once
#include "spinner/vector.hpp"
#include <memory>
#include <string>

namespace rs {
	class VDecl;
	using SPVDecl = std::shared_ptr<VDecl>;

	struct DrawParam {
		SPVDecl			spVDecl;
		uint32_t		primitiveType;
		std::string		techName,
						passName;
	};
	using DrawParamSP = std::shared_ptr<DrawParam>;

	template <class>
	class DrawDecl;
}
//! VDeclとDrawParamの2つを定義
#define DefineDrawParam(etag) \
	namespace rs { \
		template <> \
		class DrawDecl<etag> { \
			public: \
				static const SPVDecl& GetVDecl(); \
				static const DrawParamSP& GetDParam(); \
		}; \
	}
//! VDeclのみ定義
#define DefineVDecl(etag) \
	namespace rs { \
		template <> \
		class DrawDecl<etag> { \
			public: \
				static const SPVDecl& GetVDecl(); \
		}; \
	}

