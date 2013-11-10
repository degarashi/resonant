#include "glresource.hpp"

// OpenGL関数群の定義
#define GLDEFINE(name,type)		type name;
#include "glfunc.inc"
#undef GLDEFINE

namespace rs {
	namespace {
		bool g_bglfuncInit = false;
	}
	#if defined(_WIN32)
		#include <windows.h>
		namespace {
			using SetSwapInterval_t = bool APIENTRY (*)(int);
			SetSwapInterval_t g_setswapinterval = nullptr;
		}
		#define GLGETPROC(name) wglGetProcAddress((LPCSTR)#name)
		void LoadGLAux() {
			g_setswapinterval = (SetSwapInterval_t)wglGetProcAddress("wglSwapIntervalEXT");
		}
	#else
		namespace {
			using SetSwapInterval_t = void APIENTRY (*)(int);
			SetSwapInterval_t g_setswapinterval = nullptr;
		}
		#define GLGETPROC(name) glXGetProcAddress((const GLubyte*)#name)
		void LoadGLAux() {
			// EXTかSGIのどちらかが存在する事を期待
			try {
				g_setswapinterval = (SetSwapInterval_t)GLGETPROC(glXSwapIntervalSGI);
			} catch(const std::runtime_error& e) {
				g_setswapinterval = (SetSwapInterval_t)GLGETPROC(glXSwapIntervalEXT);
			}
		}
	#endif
	bool IsGLFuncLoaded() {
		return g_bglfuncInit;
	}
	void SetSwapInterval(int n) {
		g_setswapinterval(n);
	}

	// OpenGL関数ロード
	#define GLDEFINE(name,type)		name = (type)GLGETPROC(name); \
			if(!name) throw std::runtime_error(std::string("error on loading GL function \"") + #name + '\"');
		void LoadGLFunc() {
			// 各種API関数
			#include "glfunc.inc"
			// その他OS依存なAPI関数
			LoadGLAux();
			g_bglfuncInit = true;
		}
	#undef GLDEFINE
	// ---------------------- GLShader ----------------------
	void GLShader::_initShader() {
		_idSh = glCreateShader(_flag);

		const auto* pStr = _source.c_str();
		glShaderSource(_idSh, 1, &pStr, nullptr);
		glCompileShader(_idSh);

		// エラーが無かったか確認
		GLint compiled;
		glGetShaderiv(_idSh, GL_COMPILE_STATUS, &compiled);
		AssertT(Trap, compiled==GL_TRUE, (GLE_ShaderError)(GLuint), _idSh)
	}

	GLShader::GLShader() {}
	GLShader::GLShader(GLuint flag, const std::string& src): _flag(flag), _source(src) {
		_initShader();
	}
	GLShader::~GLShader() {
		onDeviceLost();
	}
	bool GLShader::isEmpty() const {
		return _source.empty();
	}
	int GLShader::getShaderID() const {
		return _idSh;
	}
	void GLShader::onDeviceLost() {
		if(_idSh!=0) {
			glDeleteShader(_idSh);
			_idSh = 0;
		}
	}
	void GLShader::onDeviceReset() {
		if(!isEmpty() && _idSh==0)
			_initShader();
	}

	// ---------------------- GLProgram ----------------------
	GLProgram::GLProgram(HSh vsh, HSh psh) {
		_shader[ShType::VERTEX] = vsh;
		_shader[ShType::PIXEL] = psh;
		_initProgram();
	}
	GLProgram::GLProgram(HSh vsh, HSh gsh, HSh psh) {
		_shader[ShType::VERTEX] = vsh;
		_shader[ShType::PIXEL] = psh;
		_shader[ShType::GEOMETRY] = gsh;
		_initProgram();
	}
	void GLProgram::_initProgram() {
		_idProg = glCreateProgram();
		for(int i=0 ; i<static_cast<int>(ShType::NUM_SHTYPE) ; i++) {
			auto& sh = _shader[i];
			// Geometryシェーダー以外は必須
			if(sh.valid()) {
				glAttachShader(_idProg, sh.cref()->getShaderID());
				GLEC_ChkP(Trap)
			} else {
				AssertT(Trap, i==ShType::GEOMETRY, (GLE_Error)(const char*), "missing shader elements (vertex or fragment)")
			}
		}

		glLinkProgram(_idProg);
		// エラーが無いかチェック
		int ib;
		glGetProgramiv(_idProg, GL_LINK_STATUS, &ib);
		AssertT(Trap, ib==GL_TRUE, (GLE_ProgramError)(GLuint), _idProg)
	}
	GLProgram::~GLProgram() {
		onDeviceLost();
	}
	void GLProgram::onDeviceLost() {
		if(_idProg != 0) {
			// ShaderはProgramをDeleteすれば自動的にdetachされる
			glDeleteProgram(_idProg);
			_idProg = 0;
		}
	}
	void GLProgram::onDeviceReset() {
		if(_idProg == 0) {
			// 先にshaderがresetされてないかもしれないので、ここでしておく
			for(auto& s : _shader) {
				if(s)
					s.cref()->onDeviceReset();
			}
			_initProgram();
		}
	}
	const HLSh& GLProgram::getShader(ShType type) const {
		return _shader[(int)type];
	}
	int GLProgram::getUniformID(const std::string& name) const {
		GLint id = getUniformIDNc(name);
		AssertT(Trap, id>=0, (GLE_ParamNotFound)(const std::string&), name)
		return id;
	}
	int GLProgram::getUniformIDNc(const std::string& name) const {
		return glGetUniformLocation(getProgramID(), name.c_str());
	}
	int GLProgram::getAttribID(const std::string& name) const {
		GLint id = getAttribIDNc(name);
		AssertT(Trap, id>=0, (GLE_ParamNotFound)(const std::string&), name)
		return id;
	}
	GLuint GLProgram::getProgramID() const {
		return _idProg;
	}
	int GLProgram::getAttribIDNc(const std::string& name) const {
		return glGetAttribLocation(getProgramID(), name.c_str());
	}
	void GLProgram::use() const {
		glUseProgram(getProgramID());
		GLEC_ChkP(Trap)
	}
}
