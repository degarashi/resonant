#include "glresource.hpp"

namespace rs {
	// ------------------------- GLRBuffer -------------------------
	GLRBuffer::GLRBuffer(int w, int h, GLInRenderFmt fmt):
		_idRbo(0), _behLost(NONE), _restoreInfo(boost::none), _buffFmt(GLFormat::QueryInfo(fmt)->toType), _fmt(fmt), _width(w), _height(h)
	{}
	GLRBuffer::~GLRBuffer() {
		onDeviceLost();
	}
	void GLRBuffer::onDeviceReset() {
		if(_idRbo == 0) {
			GL.glGenRenderbuffers(1, &_idRbo);
			auto u = use();
			allocate();
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
	void GLRBuffer::allocate() {
		GL.glRenderbufferStorage(GL_RENDERBUFFER, _fmt.get(), _width, _height);
	}

	namespace {
		void Nothing(GLFBufferTmp&, GLRBuffer&) {}
	}
	const GLRBuffer::F_LOST GLRBuffer::cs_onLost[] = {
		Nothing,		// NONE
		Nothing,		// CLEAR
#ifndef USE_OPENGLES2
		[](GLFBufferTmp& fb, GLRBuffer& rb) {		// RESTORE
			auto fbi = fb.use();
			fb.attach(GLFBufferTmp::COLOR0, rb._idRbo);
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
				#ifndef USE_OPENGLES2
					case GLFormat::DepthStencil:	// GL_DEPTH24_STENCIL8
						rb._buffFmt = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
						texSize = sizeof(GLuint);
						break;
				#endif
					case GLFormat::Stencil:			// GL_STENCIL_INDEX8
						rb._buffFmt = GL_UNSIGNED_BYTE;
						texSize = sizeof(GLubyte);
						break;
				}
			}
			spn::ByteBuff buff(texSize * rb._width * rb._height);
			GL.glReadPixels(0, 0, rb._width, rb._height, rb._fmt.get(), rb._buffFmt.get(), &buff[0]);
			rb._restoreInfo = std::move(buff);
		}
#else
		// OpenGL ES2ではDepth, StencilフォーマットのRenderBuffer読み取りは出来ない
		// Colorフォーマットだったら一応可能だがRenderBufferに書き戻すのが手間なのであとで対応
		Nothing
#endif
	};

	const GLRBuffer::F_LOST GLRBuffer::cs_onReset[] = {
		Nothing,								// NONE
		[](GLFBufferTmp& fb, GLRBuffer& rb) {		// CLEAR
			const spn::Vec4& c = boost::get<spn::Vec4>(rb._restoreInfo);
			GL.glClearColor(c.x, c.y, c.z, c.w);
			auto fbi = fb.use();
			fb.attach(GLFBuffer::COLOR0, rb._idRbo);
			GL.glClear(GL_COLOR_BUFFER_BIT);
		},
// OpenGL ES2ではglDrawPixelsが使えないので、ひとまず無効化
#ifndef USE_OPENGLES2
		[](GLFBufferTmp& fb, GLRBuffer& rb) {		// RESTORE
			auto& buff = boost::get<spn::ByteBuff>(rb._restoreInfo);
			auto fbi = fb.use();
			fb.attach(GLFBuffer::COLOR0, rb._idRbo);
			GL.glDrawPixels(0,0, rb._fmt.get(), rb._buffFmt, &buff[0]);
			rb._restoreInfo = boost::none;
		}
#else
		Nothing
#endif
	};

	RUser<GLRBuffer> GLRBuffer::use() const {
		return RUser<GLRBuffer>(*this);
	}
	void GLRBuffer::use_begin() const {
		GL.glBindRenderbuffer(GL_RENDERBUFFER, _idRbo);
		GLEC_Chk(Trap)
	}
	void GLRBuffer::use_end() const {
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
	GLFBufferTmp::GLFBufferTmp(GLuint idFb): GLFBufferCore(idFb) {}
	RUser<GLFBufferTmp> GLFBufferTmp::use() const {
		return RUser<GLFBufferTmp>(*this);
	}
	void GLFBufferTmp::use_end() const {
		for(int i=0 ; i<NUM_ATTACHMENT ; i++)
			const_cast<GLFBufferTmp*>(this)->_attachRenderbuffer(static_cast<AttID>(i), 0);
	}
	void GLFBufferTmp::attach(AttID att, GLuint rb) {
#ifdef USE_OPENGLES2
		AssertP(Trap, att != AttID::DEPTH_STENCIL)
#endif
		_attachRenderbuffer(att, rb);
	}

	// ------------------------- GLFBufferCore -------------------------
	GLFBufferCore::GLFBufferCore(GLuint id): _idFbo(id) {}
	RUser<GLFBufferCore> GLFBufferCore::use() const {
		return RUser<GLFBufferCore>(*this);
	}
	void GLFBufferCore::_attachRenderbuffer(AttID aId, GLuint rb) {
		GL.glFramebufferRenderbuffer(GL_FRAMEBUFFER, _AttIDtoGL(aId), GL_RENDERBUFFER, rb);
	}
	void GLFBufferCore::_attachTexture(AttID aId, GLuint tb) {
		GL.glFramebufferTexture2D(GL_FRAMEBUFFER, _AttIDtoGL(aId), GL_TEXTURE_2D, tb, 0);
	}
	void GLFBufferCore::use_begin() const {
		GL.glBindFramebuffer(GL_FRAMEBUFFER, _idFbo);
		GLenum e = GL.glCheckFramebufferStatus(GL_FRAMEBUFFER);
		Assert(Trap, e != GL_FRAMEBUFFER_COMPLETE);
	}
	void GLFBufferCore::use_end() const {
		GL.glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	GLuint GLFBufferCore::getBufferID() const { return _idFbo; }

	// ------------------------- draw::FrameBuff -------------------------
	namespace draw {
		struct FrameBuff::Visitor : boost::static_visitor<> {
			draw::FrameBuff::Pair& _dst;
			Visitor(draw::FrameBuff::Pair& dst): _dst(dst) {}

			void operator()(const HLTex& hlTex) const {
				_dst.bTex = true;
				_dst.idRes = hlTex.cref()->getTextureID();
			}
			void operator()(const HLRb& hlRb) const {
				_dst.bTex = false;
				_dst.idRes = hlRb.cref()->getBufferID();
			}
			void operator()(boost::none_t) const {}
		};
		FrameBuff::FrameBuff(HRes hRes, GLuint idFb, const Res (&att)[AttID::NUM_ATTACHMENT]):
			GLFBufferCore(idFb), Token(hRes)
		{
			for(int i=0 ; i<AttID::NUM_ATTACHMENT ; i++)
				boost::apply_visitor(Visitor(_ent[i]), att[i]);
		}
		void FrameBuff::exec() {
			use_begin();
			for(int i=0 ; i<NUM_ATTACHMENT ; i++) {
				auto& p = _ent[i];
				if(p.idRes != 0) {
					auto flag = _AttIDtoGL(static_cast<AttID>(i));
					if(p.bTex)
						GL.glFramebufferTexture2D(GL_FRAMEBUFFER, flag, GL_TEXTURE_2D, p.idRes, 0);
					else
						GL.glFramebufferRenderbuffer(GL_FRAMEBUFFER, flag, GL_RENDERBUFFER, p.idRes);
				}
			}
		}
	}
	// ------------------------- GLFBuffer -------------------------
	namespace {
		const auto fnReset = [](IGLResource* r) { r->onDeviceReset(); };
		// const auto fnLost = [](IGLResource* r) { r->onDeviceLost(); };

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
	GLFBuffer::GLFBuffer(): GLFBufferCore(0) {}
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
	GLenum GLFBufferCore::_AttIDtoGL(AttID att) {
		if(att < AttID::DEPTH)
			return static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + static_cast<int>(att));
		if(att == AttID::DEPTH)
			return GL_DEPTH_ATTACHMENT;
		return GL_STENCIL_ATTACHMENT;
	}

	void GLFBuffer::onDeviceLost() {
		if(_idFbo != 0) {
			{
				auto u = use();
				for(int i=0 ; i<NUM_ATTACHMENT ; i++) {
					// AttachmentのDetach
					GL.glFramebufferRenderbuffer(GL_FRAMEBUFFER, _AttIDtoGL(static_cast<AttID>(i)), GL_RENDERBUFFER, 0);
				}
			}
			GL.glDeleteFramebuffers(1, &_idFbo);
			_idFbo = 0;
			GLEC_Chk(Trap)
			// Attachmentの解放は各ハンドルに任せる
		}
	}
	void GLFBuffer::attach(AttID att, HRb hRb) {
		_attachment[att] = hRb;
	}
	void GLFBuffer::attach(AttID att, HTex hTex) {
		_attachment[att] = hTex;
	}
	void GLFBuffer::detach(AttID att) {
		_attachment[att] = boost::none;
	}
	draw::FrameBuff GLFBuffer::getDrawToken(IPreFunc& pf, HRes hRes) const {
		draw::FrameBuff fb(hRes, _idFbo, _attachment);
		return std::move(fb);
	}
	const GLFBuffer::Res& GLFBuffer::getAttachment(AttID att) const {
		return _attachment[att];
	}
}
