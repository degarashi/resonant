#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include "glresource.hpp"
#include "event.hpp"

namespace rs {
	namespace {
		bool g_bglfuncInit = false;
	}
	#define GLGETPROC(name) SDL_GL_GetProcAddress(BOOST_PP_STRINGIZE(name))
	#if defined(_WIN32)
		namespace {
			using SetSwapInterval_t = bool APIENTRY (*)(int);
			SetSwapInterval_t g_setswapinterval = nullptr;
		}
		void LoadGLAux() {
			g_setswapinterval = (SetSwapInterval_t)GLGETPROC(wglSwapIntervalEXT);
		}
	#else
		namespace {
			#ifdef __ANDROID__
				using SetSwapInterval_t = void GL_APIENTRY (*)(int);
			#else
				using SetSwapInterval_t = void APIENTRY (*)(int);
			#endif
			SetSwapInterval_t g_setswapinterval = nullptr;
		}
		void LoadGLAux() {
			// EXTかSGIのどちらかが存在する事を期待
			try {
				g_setswapinterval = (SetSwapInterval_t)GLGETPROC(glXSwapIntervalSGI);
			} catch(const std::runtime_error& e) {
				g_setswapinterval = (SetSwapInterval_t)GLGETPROC(glXSwapIntervalEXT);
			}
		}
	#endif
	bool GLWrap::isGLFuncLoaded() {
		return g_bglfuncInit;
	}
	void IGL_Draw::setSwapInterval(int n) {
		g_setswapinterval(n);
	}
	void IGL_OtherSingle::setSwapInterval(int n) {
		GLW.getDrawHandler().postExec([=](){
			IGL_Draw().setSwapInterval(n);
		});
	}

	// OpenGL関数ロード
	// FuncNameで読み込めなければFuncNameARBとFuncNameEXTで試す
	#define GLDEFINE(...)
 	#define DEF_GLMETHOD(ret_type, num, name, args, argnames) \
		GLWrap::name = nullptr; \
 		GLWrap::name = (typename GLWrap::t_##name) GLGETPROC(name); \
		if(GLWrap::name == nullptr) GLWrap::name = (typename GLWrap::t_##name)GLGETPROC(BOOST_PP_CAT(name,ARB)); \
		if(GLWrap::name == nullptr) GLWrap::name = (typename GLWrap::t_##name)GLGETPROC(BOOST_PP_CAT(name,EXT)); \
		Assert(Warn, GLWrap::name != nullptr, "could not load OpenGL function: %1%", #name)
		void GLWrap::loadGLFunc() {
			// 各種API関数
			#ifndef ANDROID
				#ifdef ANDROID
					#include "android_gl.inc"
				#elif defined(WIN32)
					#include "mingw_gl.inc"
				#else
					#include "linux_gl.inc"
				#endif
			#endif
			// その他OS依存なAPI関数
			LoadGLAux();
			g_bglfuncInit = true;
		}
	#undef DEF_GLMETHOD
	#undef GLDEFINE
	// ---------------------- GLShader ----------------------
	void GLShader::_initShader() {
		_idSh = GL.glCreateShader(_flag);

		std::string ss("#version 110\n");
		ss.append(_source);
		const auto* pStr = ss.c_str();
		GL.glShaderSource(_idSh, 1, &pStr, nullptr);
		GL.glCompileShader(_idSh);

		// エラーが無かったか確認
		GLint compiled;
		GL.glGetShaderiv(_idSh, GL_COMPILE_STATUS, &compiled);
		AssertT(Throw, compiled==GL_TRUE, (GLE_ShaderError)(const std::string&)(GLuint), ss, _idSh)
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
			GL.glDeleteShader(_idSh);
			_idSh = 0;
		}
	}
	void GLShader::onDeviceReset() {
		if(!isEmpty() && _idSh==0)
			_initShader();
	}

	// ---------------------- draw::Program ----------------------
	namespace draw {
		Program::Program(HProg hProg):
			Token(hProg), _idProg(hProg.ref()->getProgramID()) {}
		Program::Program(Program&& p): Token(p), _idProg(p._idProg) {}
		void Program::exec() {
			GL.glUseProgram(_idProg);
		}
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
		_idProg = GL.glCreateProgram();
		for(int i=0 ; i<static_cast<int>(ShType::NUM_SHTYPE) ; i++) {
			auto& sh = _shader[i];
			// Geometryシェーダー以外は必須
			if(sh.valid()) {
				GL.glAttachShader(_idProg, sh.cref()->getShaderID());
				GLEC_ChkP(Trap)
			} else {
				AssertT(Trap, i==ShType::GEOMETRY, (GLE_Error)(const char*), "missing shader elements (vertex or fragment)")
			}
		}

		GL.glLinkProgram(_idProg);
		// エラーが無いかチェック
		int ib;
		GL.glGetProgramiv(_idProg, GL_LINK_STATUS, &ib);
		AssertT(Throw, ib==GL_TRUE, (GLE_ProgramError)(GLuint), _idProg)
	}
	GLProgram::~GLProgram() {
		if(mgr_gl.isInDtor()) {
			for(auto& p : _shader)
				p.setNull();
		}
		onDeviceLost();
	}
	void GLProgram::onDeviceLost() {
		if(_idProg != 0) {
			// ShaderはProgramをDeleteすれば自動的にdetachされる
			GL.glDeleteProgram(_idProg);
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
		return GL.glGetUniformLocation(getProgramID(), name.c_str());
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
		return GL.glGetAttribLocation(getProgramID(), name.c_str());
	}
	void GLProgram::use() const {
		GL.glUseProgram(getProgramID());
		GLEC_ChkP(Trap)
	}
}
