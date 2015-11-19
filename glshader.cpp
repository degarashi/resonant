#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include "glresource.hpp"
#include "event.hpp"

#if !defined(WIN32) && !defined(ANDROID)
	#include <GL/glx.h>
	#include "glxext.h"
#endif

namespace rs {
	namespace {
		bool g_bglfuncInit = false;
	}
	#define GLGETPROC(name) SDL_GL_GetProcAddress(BOOST_PP_STRINGIZE(name))
	bool GLWrap::isGLFuncLoaded() {
		return g_bglfuncInit;
	}
	void IGL_Draw::setSwapInterval(int n) {
		SDL_GL_SetSwapInterval(n);
	}
	void IGL_OtherSingle::setSwapInterval(int n) {
		GLW.getDrawHandler().postExec([=](){
			IGL_Draw().setSwapInterval(n);
		});
	}

	// OpenGL関数ロード
	// FuncNameで読み込めなければFuncNameARBとFuncNameEXTで試す
	#define GLDEFINE(...)
	#define DEF_GLCONST(...)
	#define DEF_GLMETHOD(ret_type, num, name, args, argnames) \
		GLWrap::name = nullptr; \
		GLWrap::name = (typename GLWrap::t_##name) GLGETPROC(name); \
		if(GLWrap::name == nullptr) GLWrap::name = (typename GLWrap::t_##name)GLGETPROC(BOOST_PP_CAT(name,ARB)); \
		if(GLWrap::name == nullptr) GLWrap::name = (typename GLWrap::t_##name)GLGETPROC(BOOST_PP_CAT(name,EXT)); \
		Assert(Warn, GLWrap::name != nullptr, "could not load OpenGL function: %1%", #name)
		void GLWrap::loadGLFunc() {
			// 各種API関数
			#ifdef ANDROID
				#include "opengl_define/android_gl.inc"
			#elif defined(WIN32)
				#include "opengl_define/mingw_gl.inc"
			#else
				#include "opengl_define/linux_gl.inc"
			#endif
			// その他OS依存なAPI関数
			g_bglfuncInit = true;
		}
	#undef DEF_GLMETHOD
	#undef DEF_GLCONST
	#undef GLDEFINE
	// ---------------------- GLShader ----------------------
	void GLShader::_initShader() {
		_idSh = GL.glCreateShader(c_glShFlag[_flag]);

		const auto* pStr = _source.c_str();
		GL.glShaderSource(_idSh, 1, &pStr, nullptr);
		GL.glCompileShader(_idSh);

		// エラーが無かったか確認
		GLint compiled;
		GL.glGetShaderiv(_idSh, GL_COMPILE_STATUS, &compiled);
		AssertTArg(Throw, compiled==GL_TRUE, (GLE_ShaderError)(const std::string&)(GLuint), _source, _idSh)
	}

	GLShader::GLShader() {}
	GLShader::GLShader(ShType flag, const std::string& src): _idSh(0), _flag(flag), _source(src) {
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
			GLW.getDrawHandler().postExecNoWait([buffId=getShaderID()](){
				GLEC_D(Warn, glDeleteShader, buffId);
			});
			_idSh = 0;
		}
	}
	void GLShader::onDeviceReset() {
		if(!isEmpty() && _idSh==0)
			_initShader();
	}
	ShType GLShader::getShaderType() const {
		return _flag;
	}

	// ---------------------- draw::Program ----------------------
	namespace draw {
		Program::Program(HRes hRes, GLuint idProg):
			TokenR(hRes), _idProg(idProg) {}
		void Program::exec() {
			GL.glUseProgram(_idProg);
		}
	}

	// ---------------------- GLProgram ----------------------
	void GLProgram::_setShader(HSh hSh) {
		if(hSh)
			_shader[hSh.ref()->getShaderType()] = hSh;
	}
	void GLProgram::_initProgram() {
		_idProg = GL.glCreateProgram();
		for(int i=0 ; i<static_cast<int>(ShType::NUM_SHTYPE) ; i++) {
			auto& sh = _shader[i];
			// Geometryシェーダー以外は必須
			if(sh.valid()) {
				GL.glAttachShader(_idProg, sh.cref()->getShaderID());
				GLEC_Chk_D(Trap)
			} else {
				AssertT(Trap, i==ShType::GEOMETRY, GLE_Error, "missing shader elements (vertex or fragment)")
			}
		}

		GL.glLinkProgram(_idProg);
		// エラーが無いかチェック
		int ib;
		GL.glGetProgramiv(_idProg, GL_LINK_STATUS, &ib);
		AssertTArg(Throw, ib==GL_TRUE, (GLE_ProgramError)(GLuint), _idProg)
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
			GLW.getDrawHandler().postExecNoWait([buffId=getProgramID()](){
				GLuint num;
				GLEC_D(Warn, glGetIntegerv, GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&num));
				if(num == buffId)
					GLEC_D(Warn, glUseProgram, 0);
				// ShaderはProgramをDeleteすれば自動的にdetachされる
				GLEC_D(Warn, glDeleteProgram, buffId);
			});
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
	OPGLint GLProgram::getUniformID(const std::string& name) const {
		GLint id = GL.glGetUniformLocation_NC(getProgramID(), name.c_str());
		return id>=0 ? OPGLint(id) : OPGLint(spn::none);
	}
	OPGLint GLProgram::getAttribID(const std::string& name) const {
		GLint id = GL.glGetAttribLocation_NC(getProgramID(), name.c_str());
		return id>=0 ? OPGLint(id) : OPGLint(spn::none);
	}
	GLuint GLProgram::getProgramID() const {
		return _idProg;
	}
	int GLProgram::_getNumParam(GLenum flag) const {
		int iv;
		GL.glGetProgramiv(getProgramID(), flag, &iv);
		return iv;
	}
	int GLProgram::getNActiveAttribute() const {
		return _getNumParam(GL_ACTIVE_ATTRIBUTES);
	}
	int GLProgram::getNActiveUniform() const {
		return _getNumParam(GL_ACTIVE_UNIFORMS);
	}
	GLParamInfo GLProgram::_getActiveParam(int n, InfoF infoF) const {
		GLchar buff[128];
		GLsizei len;
		GLint sz;
		GLenum typ;
		(GL.*infoF)(getProgramID(), n, countof(buff), &len, &sz, &typ, buff);
		GLParamInfo ret = *GLFormat::QueryGLSLInfo(typ);
		ret.name = buff;
		return ret;
	}
	GLParamInfo GLProgram::getActiveAttribute(int n) const {
		return _getActiveParam(n, &IGL::glGetActiveAttrib);
	}
	GLParamInfo GLProgram::getActiveUniform(int n) const {
		return _getActiveParam(n, &IGL::glGetActiveUniform);
	}
	void GLProgram::use() const {
		GL.glUseProgram(getProgramID());
	}
	void GLProgram::getDrawToken(draw::TokenDst& dst) const {
		using UT = draw::Program;
		new(dst.allocate_memory(sizeof(UT), draw::CalcTokenOffset<UT>())) UT(handleFromThis(), getProgramID());
	}
	// ------------------ GLParamInfo ------------------
	GLParamInfo::GLParamInfo(const GLSLFormatDesc& desc): GLSLFormatDesc(desc) {}
}

