//! リソースハンドルの前方宣言
#pragma once
#include <memory>
#include "spinner/misc.hpp"
#include "spinner/alignedalloc.hpp"
#include "spinner/resmgr.hpp"

namespace rs {
	class GLWrap;
	struct IGLResource;
	class GLRes;
	using UPResource = std::unique_ptr<IGLResource>;
	class GLEffect;
	using UPEffect = std::unique_ptr<GLEffect>;
	class VDecl;
	using SPVDecl = std::shared_ptr<VDecl>;
	class TPStructR;
	class GLBuffer;
	using UPBuffer = std::unique_ptr<GLBuffer>;
	class GLVBuffer;
	using UPVBuffer = std::unique_ptr<GLVBuffer>;
	class GLIBuffer;
	using UPIBuffer = std::unique_ptr<GLIBuffer>;
	class IGLTexture;
	using UPTexture = std::unique_ptr<IGLTexture>;
	class GLProgram;
	using UPProg = std::unique_ptr<GLProgram>;
	class GLShader;
	using UPShader = std::unique_ptr<GLShader>;
	class GLFBuffer;
	using UPFBuffer = std::unique_ptr<GLFBuffer>;
	class GLRBuffer;
	using UPRBuffer = std::unique_ptr<GLRBuffer>;
	DEF_NHANDLE_PROP(GLRes, Res, UPResource, UPResource, std::allocator, std::string)
	DEF_NHANDLE_PROP(GLRes, Tex, UPResource, UPTexture, std::allocator, std::string)
	DEF_NHANDLE_PROP(GLRes, Vb, UPResource, UPVBuffer, std::allocator, std::string)
	DEF_NHANDLE_PROP(GLRes, Ib, UPResource, UPIBuffer, std::allocator, std::string)
	DEF_NHANDLE_PROP(GLRes, Buff, UPResource, UPBuffer, std::allocator, std::string)
	DEF_NHANDLE_PROP(GLRes, Prog, UPResource, UPProg, std::allocator, std::string)
	DEF_NHANDLE_PROP(GLRes, Sh, UPResource, UPShader, std::allocator, std::string)
	DEF_NHANDLE_PROP(GLRes, Fx, UPResource, UPEffect, std::allocator, std::string)
	DEF_NHANDLE_PROP(GLRes, Fb, UPResource, UPFBuffer, std::allocator, std::string)
	DEF_NHANDLE_PROP(GLRes, Rb, UPResource, UPRBuffer, std::allocator, std::string)

	class FontGen;
	class TextObj;
	DEF_NHANDLE_PROP(FontGen, Text, TextObj, TextObj, std::allocator, std::u32string)

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
	class UpdGroup;
	class DrawableObj;
	class DrawGroup;
	using ObjectUP = std::unique_ptr<Object, void (*)(void*)>;	// Alignedメモリ対応の為 デリータ指定
	using GroupUP = std::unique_ptr<UpdGroup, void (*)(void*)>;
	using DrawableUP = std::unique_ptr<DrawableObj, void (*)(void*)>;
	using DrawGroupUP = std::unique_ptr<DrawGroup, void (*)(void*)>;
	class ObjMgr;
	DEF_AHANDLE(ObjMgr, Obj, ObjectUP, ObjectUP)
	DEF_AHANDLE(ObjMgr, Group, ObjectUP, GroupUP)
	DEF_AHANDLE(ObjMgr, DObj, ObjectUP, DrawableUP)
	DEF_AHANDLE(ObjMgr, DGroup, ObjectUP, DrawGroupUP)
}

