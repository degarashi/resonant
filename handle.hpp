//! リソースハンドルの前方宣言
#pragma once
#include <memory>
#include "spinner/misc.hpp"
#include "spinner/alignedalloc.hpp"

namespace rs {
	class CameraMgr;
	class CamData;
	DEF_AHANDLE_PROP(CameraMgr, Cam, CamData, CamData, spn::Alloc16)

	class PointerMgr;
	struct TPos2D;
	DEF_AHANDLE(PointerMgr, Ptr, TPos2D, TPos2D)

	class InputMgrBase;
	class Input;
	class IInput;
	using UPInput = std::unique_ptr<IInput>;
	DEF_AHANDLE(InputMgrBase, Input, UPInput, UPInput)

	class FTLibrary;
	class FTFace;
	DEF_AHANDLE(FTLibrary, FT, FTFace, FTFace)

	class RWops;
	class RWMgr;
	DEF_AHANDLE(RWMgr, RW, RWops, RWops)

	class ABufMgr;
	class ABuffer;
	using UPABuff = std::unique_ptr<ABuffer>;
	DEF_AHANDLE(ABufMgr, Ab, UPABuff, UPABuff)

	class ASource;
	class SSrcMgr;
	DEF_AHANDLE(SSrcMgr, Ss, ASource, ASource)

	class SGroupMgr;
	class AGroup;
	DEF_AHANDLE(SGroupMgr, Sg, AGroup, AGroup)
	
	class Object;
	using UPObject = std::unique_ptr<Object, void (*)(void*)>;	// Alignedメモリ対応の為 デリータ指定
	class ObjMgr;
	DEF_AHANDLE(ObjMgr, Gbj, UPObject, UPObject)

	class UpdMgr;
	class UpdChild;
	using UPUpdCh = std::unique_ptr<UpdChild>;
	DEF_AHANDLE(UpdMgr, Upd, UPUpdCh, UPUpdCh)
}

