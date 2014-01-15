#include "glresource.hpp"

namespace rs {
	// ------------------------- GLRBuffer -------------------------
	DEF_GLRESOURCE_CPP(GLRBuffer)
	GLRBuffer::GLRBuffer(int w, int h, GLInRenderFmt fmt):
		_idRbo(0), _behLost(NONE), _restoreInfo(boost::none), _buffFmt(GLFormat::QueryInfo(fmt)->toType), _fmt(fmt), _width(w), _height(h)
	{}
	GLRBuffer::~GLRBuffer() {
		onDeviceLost();
	}
	void GLRBuffer::onDeviceReset() {
		if(_idRbo == 0) {
			GL.glGenRenderbuffers(1, &_idRbo);
			use()->allocate();
			cs_onReset[_behLost](mgr_gl.getTmpFramebuffer(), *this);
		}
	}
	void GLRBuffer::onDeviceLost() {
		if(_idRbo != 0) {
			GL.glDeleteRenderbuffers(1, &_idRbo);
			cs_onLost[_behLost](mgr_gl.getTmpFramebuffer(), *this);
			_idRbo = 0;
		}
	}
	GLRBuffer::Inner1& GLRBuffer::allocate() {
		GL.glRenderbufferStorage(GL_RENDERBUFFER, _fmt.get(), _width, _height);
		return Inner1::Cast(this);
	}

	namespace {
		void Nothing(GLFBufferTmp&, GLRBuffer&) {}
	}
	const GLRBuffer::F_LOST GLRBuffer::cs_onLost[] = {
		Nothing,		// NONE
		Nothing,		// CLEAR
		[](GLFBufferTmp& fb, GLRBuffer& rb) {		// RESTORE
			auto fbi = fb->attachColor(0, rb._idRbo);
			GLFormat::OPInfo op = GLFormat::QueryInfo(rb._fmt.get());
			int texSize;
			if(op) {
				rb._buffFmt = op->toType;
				texSize = op->numType * GLFormat::QuerySize(op->numType);
			} else {
				// BaseFormatな時は判別をかける
				uint32_t dsc = GLFormat::QueryFormat(rb._fmt.get(), GLFormat::Query_DSC);
				switch(dsc) {
					case GLFormat::Internal:		// GL_RGBA8
						rb._buffFmt = GL_UNSIGNED_BYTE;
						texSize = sizeof(GLubyte)*4;
						break;
					case GLFormat::Depth:			// GL_DEPTH_COMPONENT16
						rb._buffFmt = GL_UNSIGNED_SHORT;
						texSize = sizeof(GLushort);
						break;
					case GLFormat::DepthStencil:	// GL_DEPTH24_STENCIL8
						rb._buffFmt = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
						texSize = sizeof(GLuint);
						break;
					case GLFormat::Stencil:			// GL_STENCIL_INDEX8
						rb._buffFmt = GL_UNSIGNED_BYTE;
						texSize = sizeof(GLubyte);
						break;
				}
			}
			spn::ByteBuff buff(texSize * rb._width * rb._height);
			GL.glReadPixels(0, 0, rb._width, rb._height, rb._fmt.get(), rb._buffFmt.get(), &buff[0]);
			rb._restoreInfo = std::move(buff);
			fbi.end();
		}
	};

	const GLRBuffer::F_LOST GLRBuffer::cs_onReset[] = {
		Nothing,								// NONE
		[](GLFBufferTmp& fb, GLRBuffer& rb) {		// CLEAR
			const spn::Vec4& c = boost::get<spn::Vec4>(rb._restoreInfo);
			GL.glClearColor(c.x, c.y, c.z, c.w);
			auto fbi = fb->attachColor(0, rb._idRbo);
			GL.glClear(GL_COLOR_BUFFER_BIT);
			fbi.end();
		},
		[](GLFBufferTmp& fb, GLRBuffer& rb) {		// RESTORE
			auto& buff = boost::get<spn::ByteBuff>(rb._restoreInfo);
			auto fbi = fb->attachColor(0, rb._idRbo);
			GL.glDrawPixels(0,0, rb._fmt.get(), rb._buffFmt, &buff[0]);
			rb._restoreInfo = boost::none;
			fbi.end();
		}
	};

	void GLRBuffer::Use(GLRBuffer& rb) {
		GL.glBindRenderbuffer(GL_RENDERBUFFER, rb._idRbo);
		GLEC_Chk(Trap)
	}
	void GLRBuffer::End(GLRBuffer&) {
		GLEC_Chk(Trap)
		GL.glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
	GLuint GLRBuffer::getBufferID() const {
		return _idRbo;
	}
	void GLRBuffer::setOnLost(OnLost beh, const spn::Vec4* color) {
		_behLost = beh;
		if(beh == CLEAR)
			_restoreInfo = *color;
	}

	// ------------------------- GLFBufferTmp -------------------------
	DEF_GLRESOURCE_CPP(GLFBufferTmp)
	void GLFBufferTmp::Use(GLFBufferTmp& tmp) {
		GL.glBindFramebuffer(GL_FRAMEBUFFER, tmp._idFb);
	}
	void GLFBufferTmp::End(GLFBufferTmp&) {
		constexpr GLenum ids[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
									GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
		for(auto id : ids)
			GL.glFramebufferRenderbuffer(GL_FRAMEBUFFER, id, GL_RENDERBUFFER, 0);
	}
	GLFBufferTmp::GLFBufferTmp(GLuint idFb): _idFb(idFb) {}
	GLFBufferTmp::Inner1& GLFBufferTmp::attachColor(int n, GLuint rb) {
		return _attach(GL_COLOR_ATTACHMENT0+n, rb);
	}
	GLFBufferTmp::Inner1& GLFBufferTmp::attachDepth(GLuint rb) {
		return _attach(GL_DEPTH_ATTACHMENT, rb);
	}
	GLFBufferTmp::Inner1& GLFBufferTmp::attachStencil(GLuint rb) {
		return _attach(GL_STENCIL_ATTACHMENT, rb);
	}
	GLFBufferTmp::Inner1& GLFBufferTmp::_attach(GLenum flag, GLuint rb) {
		GL.glFramebufferRenderbuffer(GL_FRAMEBUFFER, flag, GL_RENDERBUFFER, rb);
		return Inner1::Cast(this);
	}

	// ------------------------- GLFBuffer -------------------------
	DEF_GLRESOURCE_CPP(GLFBuffer)
	namespace {
		const auto fnReset = [](IGLResource* r) { r->onDeviceReset(); };
		const auto fnLost = [](IGLResource* r) { r->onDeviceLost(); };

		template <class F>
		struct HdlVisitor : boost::static_visitor<> {
			F _f;
			HdlVisitor(F f): _f(f) {}
			void operator()(boost::none_t) const {}
			template <class HDL>
			void operator()(HDL& hdl) const {
				_f(hdl.ref().get());
			}
		};
		template <class F>
		HdlVisitor<F> MakeVisitor(F f) {
			return HdlVisitor<F>(f);
		}
	}
	GLFBuffer::GLFBuffer(): _idFbo(0) {}
	GLFBuffer::~GLFBuffer() {
		onDeviceLost();
	}
	void GLFBuffer::onDeviceReset() {
		if(_idFbo == 0) {
			GL.glGenFramebuffers(1, &_idFbo);
			GLEC_Chk(Trap)

			auto v = MakeVisitor(fnReset);
			for(auto& att : _attachment) {
				// Attachmentの復元は今行う (各ハンドルでの処理はif文で弾かれる)
				boost::apply_visitor(v, att);
			}
		}
	}
	GLenum GLFBuffer::_AttIDtoGL(AttID att) {
		constexpr GLenum c_fbAtt[GLFBuffer::NUM_ATTACHMENT] = {
			GL_COLOR_ATTACHMENT0,
			GL_DEPTH_ATTACHMENT,
			GL_STENCIL_ATTACHMENT
		};
		return c_fbAtt[att];
	}

	void GLFBuffer::onDeviceLost() {
		if(_idFbo != 0) {
			{
				auto u = use();
				for(int i=0 ; i<NUM_ATTACHMENT ; i++) {
					// AttachmentのDetach
					GL.glFramebufferRenderbuffer(GL_FRAMEBUFFER, _AttIDtoGL(static_cast<AttID>(i)), GL_RENDERBUFFER, 0);
				}
				u->end();
			}
			GL.glDeleteFramebuffers(1, &_idFbo);
			_idFbo = 0;
			GLEC_Chk(Trap)
			// Attachmentの解放は各ハンドルに任せる
		}
	}
	void GLFBuffer::Use(GLFBuffer& fb) {
		GL.glBindFramebuffer(GL_FRAMEBUFFER, fb._idFbo);
		GLenum e = GL.glCheckFramebufferStatus(GL_FRAMEBUFFER);
		Assert(Trap, e != GL_FRAMEBUFFER_COMPLETE);
		GLEC_Chk(Trap)
	}
	void GLFBuffer::End(GLFBuffer&) {
		GL.glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	GLFBuffer::Inner1& GLFBuffer::attach(AttID att, HRb hRb) {
		_attachment[att] = hRb;
		GL.glFramebufferRenderbuffer(GL_FRAMEBUFFER, _AttIDtoGL(att), GL_RENDERBUFFER, hRb.ref()->getBufferID());
		return Inner1::Cast(this);
	}
	GLFBuffer::Inner1& GLFBuffer::attach(AttID att, HTex hTex) {
		_attachment[att] = hTex;
		GL.glFramebufferTexture2D(GL_FRAMEBUFFER, _AttIDtoGL(att), GL_TEXTURE_2D, hTex.ref()->getTextureID(), 0);
		return Inner1::Cast(this);
	}
	GLFBuffer::Inner1& GLFBuffer::detach(AttID att) {
		GL.glFramebufferRenderbuffer(GL_FRAMEBUFFER, _AttIDtoGL(att), GL_RENDERBUFFER, 0);
		return Inner1::Cast(this);
	}
	GLuint GLFBuffer::getBufferID() const { return _idFbo; }
	const GLFBuffer::Res& GLFBuffer::getAttachment(AttID att) const {
		return _attachment[att];
	}
}
