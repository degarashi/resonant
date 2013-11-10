#include "glresource.hpp"

namespace rs {
	// ------------------------- GLError -------------------------
	const std::pair<GLenum, const char*> GLError::ErrorList[] = {
		{GL_INVALID_VALUE, "Numeric argument out of range"},
		{GL_INVALID_ENUM, "Enum argument out of range"},
		{GL_INVALID_OPERATION, "Operation illegal in current state"},
		{GL_INVALID_FRAMEBUFFER_OPERATION, "Framebuffer is incomplete"},
		{GL_OUT_OF_MEMORY, "Not enough memory left to execute command"}
	};
	const char* GLError::errorDesc() const {
		GLenum err;
		tls_errMsgTmp.clear();
		while((err = glGetError()) != GL_NO_ERROR) {
			// OpenGLエラー詳細を取得
			bool bFound = false;
			for(const auto& e : ErrorList) {
				if(e.first == err) {
					tls_errMsgTmp += e.second;
					tls_errMsgTmp += '\n';
					bFound = true;
					break;
				}
			}
			if(!bFound)
				tls_errMsgTmp += "unknown errorID\n";
		}
		if(!tls_errMsgTmp.empty())
			return tls_errMsgTmp.c_str();
		return nullptr;
	}
	void GLError::reset() const {
		while(glGetError() != GL_NO_ERROR);
	}
	const char* GLError::getAPIName() const {
		return "OpenGL";
	}
	void GLError::resetError() const {
		while(glGetError() != GL_NO_ERROR);
	}
	namespace {
		bool CheckGLError(GLGetIV ivF, GLInfoFunc logF, GLenum getflag, GLuint id) {
			GLint ib;
			ivF(id, getflag, &ib);
			return ib == GL_FALSE;
		}
	}
	// ------------------------- GLProgError -------------------------
	GLProgError::GLProgError(GLuint id): _id(id) {}
	const char* GLProgError::errorDesc() const {
		if(CheckGLError(glGetProgramiv, glGetProgramInfoLog, GL_LINK_STATUS, _id))
			return "link program failed";
		return nullptr;
	}
	void GLProgError::reset() const {}
	const char* GLProgError::getAPIName() const {
		return "OpenGL(GLSL program)";
	}
	// ------------------------- GLShError -------------------------
	GLShError::GLShError(GLuint id): _id(id) {}
	const char* GLShError::errorDesc() const {
		if(CheckGLError(glGetShaderiv, glGetShaderInfoLog, GL_COMPILE_STATUS, _id))
			return "compile shader failed";
		return nullptr;
	}
	void GLShError::reset() const {}
	const char* GLShError::getAPIName() const {
		return "OpenGL(GLSL shader)";
	}

	// ------------------------- GLE_ShProgBase -------------------------
	GLE_ShProgBase::GLE_ShProgBase(GLGetIV ivF, GLInfoFunc infoF, const std::string& aux, GLuint id): GLE_Error(""),
		_ivF(ivF), _infoF(infoF), _id(id)
	{
		int logSize, length;
		ivF(id, GL_INFO_LOG_LENGTH, &logSize);
		std::string ret;
		ret.resize(logSize);
		infoF(id, logSize, &length, &ret[0]);
		(GLE_Error&)*this = GLE_Error(aux + ":\n" + std::move(ret));
	}
	// ------------------------- GLE_ShaderError -------------------------
	GLE_ShaderError::GLE_ShaderError(GLuint id): GLE_ShProgBase(glGetShaderiv, glGetShaderInfoLog, "compile shader failed", id) {}
	// ------------------------- GLE_ProgramError -------------------------
	GLE_ProgramError::GLE_ProgramError(GLuint id): GLE_ShProgBase(glGetProgramiv, glGetProgramInfoLog, "link program failed", id) {}
	// ------------------------- GLE_ParamNotFound -------------------------
	GLE_ParamNotFound::GLE_ParamNotFound(const std::string& name): GLE_Error(std::string("parameter not found:\n") + name), _name(name) {}
	// ------------------------- GLE_InvalidArgument -------------------------
	GLE_InvalidArgument::GLE_InvalidArgument(const std::string& shname, const std::string& argname): GLE_Error(shname + ":\n" + argname),
		_shName(shname), _argName(argname) {}
}
