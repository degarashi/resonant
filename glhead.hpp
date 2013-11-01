#pragma once

#include <GL/gl.h>
#if !defined(WIN32)
	#include <GL/glx.h>
	#undef Convex
	#include "glext.h"
	#include "glxext.h"
#else
	#include "glext.h"
#endif

#define GLDEFINE(name,type)		extern type name;
#include "glfunc.inc"
#undef GLDEFINE

#include <memory>
#include <vector>
#include <assert.h>

namespace rs {
	extern void LoadGLFunc();
	extern bool IsGLFuncLoaded();
	extern void SetSwapInterval(int n);

	struct IGLResource;
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

	enum ShType : unsigned int {
		VERTEX, GEOMETRY, PIXEL,
		NUM_SHTYPE
	};
	//! シェーダーIDに対するOpenGL定数
	const static GLuint c_glShFlag[ShType::NUM_SHTYPE] = {
		GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER
	};
}
