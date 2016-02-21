#include "glresource.hpp"
#include "luaw.hpp"
namespace rs {
	// 先頭のGL_を除く
	#define DEF_GLCONST(name, value) \
		(*tbl)[(#name)+3] = lua_Integer(value);
	#define DEF_GLMETHOD(...)

	#define DEF_GLCONST2(name, value) \
		(*tbl)[#name] = lua_Integer(value)
	void GLRes::LuaExport(LuaState& lsc) {
		auto tbl = std::make_shared<LCTable>();
		#ifdef ANDROID
			#include "opengl_define/android_gl.inc"
		#elif defined(WIN32)
			#include "opengl_define/mingw_gl.inc"
		#else
			#include "opengl_define/linux_gl.inc"
		#endif
		lsc.setField(-1, "Format", tbl);
		tbl->clear();

		DEF_GLCONST2(ClampToEdge, GL_CLAMP_TO_EDGE);
		DEF_GLCONST2(ClampToBorder, GL_CLAMP_TO_BORDER);
		DEF_GLCONST2(MirroredRepeat, GL_MIRRORED_REPEAT);
		DEF_GLCONST2(Repeat, GL_REPEAT);
		DEF_GLCONST2(MirrorClampToEdge, GL_MIRROR_CLAMP_TO_EDGE);
		lsc.setField(-1, "WrapState", tbl);
	}
}
