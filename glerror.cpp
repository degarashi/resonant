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
	const char* GLError::errorDesc() {
		GLenum err;
		_errMsg.clear();
		while((err = GL.glGetError()) != GL_NO_ERROR) {
			// OpenGLエラー詳細を取得
			bool bFound = false;
			for(const auto& e : ErrorList) {
				if(e.first == err) {
					_errMsg += e.second;
					_errMsg += '\n';
					bFound = true;
					break;
				}
			}
			if(!bFound)
				_errMsg += "unknown errorID\n";
		}
		if(!_errMsg.empty())
			return _errMsg.c_str();
		return nullptr;
	}
	void GLError::reset() const {
		while(GL.glGetError() != GL_NO_ERROR);
	}
	const char* GLError::getAPIName() const {
		return "OpenGL";
	}
	void GLError::resetError() const {
		while(GL.glGetError() != GL_NO_ERROR);
	}
	// ------------------------- GLProgError -------------------------
	GLProgError::GLProgError(GLuint id): _id(id) {}
	const char* GLProgError::errorDesc() const {
		GLint ib;
		GL.glGetProgramiv(_id, GL_LINK_STATUS, &ib);
		if(ib == GL_FALSE)
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
		GLint ib;
		GL.glGetShaderiv(_id, GL_COMPILE_STATUS, &ib);
		if(ib == GL_FALSE)
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
		(GL.*ivF)(id, GL_INFO_LOG_LENGTH, &logSize);
		std::string ret;
		ret.resize(logSize);
		(GL.*infoF)(id, logSize, &length, &ret[0]);
		(GLE_Error&)*this = GLE_Error(aux + ":\n" + std::move(ret));
	}
	// ------------------------- GLE_ShaderError -------------------------
	const char* GLE_ShaderError::GetErrorName() { return "compile shader failed"; }
	GLE_ShaderError::GLE_ShaderError(const std::string& src, GLuint id):
		GLE_ShProgBase(&IGL::glGetShaderiv, &IGL::glGetShaderInfoLog, spn::AddLineNumber(src, 1, 1, true, true) + GetErrorName(), id)
	{}
	// ------------------------- GLE_ProgramError -------------------------
	const char* GLE_ProgramError::GetErrorName() { return "link program failed"; }
	GLE_ProgramError::GLE_ProgramError(GLuint id): GLE_ShProgBase(&IGL::glGetProgramiv, &IGL::glGetProgramInfoLog, GetErrorName(), id) {}
	// ------------------------- GLE_ParamNotFound -------------------------
	const char* GLE_ParamNotFound::GetErrorName() { return "parameter not found"; }
	GLE_ParamNotFound::GLE_ParamNotFound(const std::string& name): GLE_Error(std::string(GetErrorName()) + '\n' + name), _name(name) {}
	// ------------------------- GLE_InvalidArgument -------------------------
	const char* GLE_InvalidArgument::GetErrorName() { return "invalid argument"; }
	GLE_InvalidArgument::GLE_InvalidArgument(const std::string& shname, const std::string& argname): GLE_Error(std::string(GetErrorName()) + shname + ":\n" + argname),
		_shName(shname), _argName(argname) {}
	// ------------------------- GLE_LogicalError -------------------------
	const char* GLE_LogicalError::GetErrorName() { return "logical error"; }
}
