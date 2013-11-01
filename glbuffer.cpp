#include "glresource.hpp"

namespace rs {
	// ------------------------- GLError -------------------------
	std::string GLError::s_tmp;
	const std::pair<GLenum, const char*> GLError::ErrorList[] = {
		{GL_INVALID_VALUE, "Numeric argument out of range"},
		{GL_INVALID_ENUM, "Enum argument out of range"},
		{GL_INVALID_OPERATION, "Operation illegal in current state"},
		{GL_INVALID_FRAMEBUFFER_OPERATION, "Framebuffer is incomplete"},
		{GL_OUT_OF_MEMORY, "Not enough memory left to execute command"}
	};
	const char* GLError::ErrorDesc() {
		GLenum err;
		s_tmp.clear();
		while((err = glGetError()) != GL_NO_ERROR) {
			// OpenGLエラー詳細を取得
			bool bFound = false;
			for(const auto& e : ErrorList) {
				if(e.first == err) {
					s_tmp += e.second;
					s_tmp += '\n';
					bFound = true;
					break;
				}
			}
			if(!bFound)
				s_tmp += "unknown errorID\n";
		}
		if(!s_tmp.empty())
			return s_tmp.c_str();
		return nullptr;
	}
	const char* GLError::GetAPIName() {
		return "OpenGL";
	}
	void GLError::ResetError() {
		while(glGetError() != GL_NO_ERROR);
	}
}
