#include "glresource.hpp"
#include "luaw.hpp"
namespace rs {
	// 先頭のGL_を除く
	#define DEF_GLCONST(name, value) \
		(*tbl)[(#name)+3] = lua_Integer(value);
	#define DEF_GLMETHOD(...)
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
	}
}
