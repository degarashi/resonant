#pragma once
#include "wrapper.hpp"
#include "handle.hpp"

namespace rs {
	using Priority = uint32_t;
	using IdValue = Wrapper<int>;
	//! Tech:Pass の組み合わせを表す
	using GL16Id = std::array<uint8_t, 2>;
	struct DrawTag {
		using TexAr = std::array<HTex, 4>;
		using VBuffAr = std::array<HVb, 4>;

		//! TechPassIdと事前登録のIdを取り持つ
		struct TPId {
			bool			bId16;		//!< trueならtpId, falseならpreId
			union {
				GL16Id		tpId;
				IdValue		preId;
				uint32_t	value;		//!< ソート時に使用
			};

			TPId();
			TPId& operator = (const GL16Id& id);
			TPId& operator = (const IdValue& id);
			bool valid() const;
		};
		TPId		idTechPass;

		VBuffAr		idVBuffer;
		HIb			idIBuffer;
		TexAr		idTex;
		Priority	priority;
		float		zOffset;

		DrawTag();
	};
}
