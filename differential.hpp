#pragma once
#include <tuple>
#include "spinner/unituple/defines.hpp"

namespace rs {
	// ---- バッファ切り替えカウント ----
	namespace diff {
		DefUnionTuple(Buffer, ((int, vertex))			// 頂点バッファ切り替え
								((int, index)))			// インデックスバッファ切り替え
		DefUnionTuple(Effect, ((int, drawIndexed))		// インデックスありドローコール
								((int, drawNoIndexed))	// インデックスなしドローコール
								((int, techPass))		// Tech or Pass切り替え
								((Buffer, buffer)))
	}
}
