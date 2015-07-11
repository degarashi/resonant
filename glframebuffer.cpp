#include "glresource.hpp"

namespace rs {
	// ------------------------- GLRBuffer -------------------------
	GLRBuffer::GLRBuffer(int w, int h, GLInRenderFmt fmt):
		_idRbo(0), _behLost(NONE), _restoreInfo(boost::none), _buffFmt(GLFormat::QueryInfo(fmt)->elementType), _fmt(fmt), _width(w), _height(h)
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
			fb.attach(GLFBufferTmp::Att::Id::COLOR0, rb._idRbo);
			GLFormat::OPInfo op = GLFormat::QueryInfo(rb._fmt.get());
			int texSize;
			if(op) {
				rb._buffFmt = op->elementType;
				texSize = op->numElem * GLFormat::QuerySize(op->numElem);
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
			fb.attach(GLFBuffer::Att::Id::COLOR0, rb._idRbo);
			GL.glClear(GL_COLOR_BUFFER_BIT);
		},
// OpenGL ES2ではglDrawPixelsが使えないので、ひとまず無効化
#ifndef USE_OPENGLES2
		[](GLFBufferTmp& fb, GLRBuffer& rb) {		// RESTORE
			auto& buff = boost::get<spn::ByteBuff>(rb._restoreInfo);
			auto fbi = fb.use();
			fb.attach(GLFBuffer::Att::Id::COLOR0, rb._idRbo);
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
		for(int i=0 ; i<Att::NUM_ATTACHMENT ; i++)
			const_cast<GLFBufferTmp*>(this)->_attachRenderbuffer(Att::Id(i), 0);
	}
	void GLFBufferTmp::attach(Att::Id att, GLuint rb) {
#ifdef USE_OPENGLES2
		AssertP(Trap, att != Att::DEPTH_STENCIL)
#endif
		_attachRenderbuffer(att, rb);
	}

	// ------------------------- GLFBufferCore -------------------------
	GLFBufferCore::GLFBufferCore(GLuint id): _idFbo(id) {}
	RUser<GLFBufferCore> GLFBufferCore::use() const {
		return RUser<GLFBufferCore>(*this);
	}
	void GLFBufferCore::_attachRenderbuffer(Att::Id aId, GLuint rb) {
		GLEC(Trap, glFramebufferRenderbuffer, GL_FRAMEBUFFER, _AttIDtoGL(aId), GL_RENDERBUFFER, rb);
	}
	void GLFBufferCore::_attachTexture(Att::Id aId, GLuint tb) {
		GL.glFramebufferTexture2D(GL_FRAMEBUFFER, _AttIDtoGL(aId), GL_TEXTURE_2D, tb, 0);
	}
	void GLFBufferCore::use_begin() const {
		GL.glBindFramebuffer(GL_FRAMEBUFFER, _idFbo);
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
			void operator()(boost::none_t) const {
				_dst.idRes = 0;
			}
		};
		FrameBuff::FrameBuff(GLuint idFb):
			GLFBufferCore(idFb), Token(HRes())
		{
			for(auto& p : _ent)
				p.idRes = 0;
		}
		FrameBuff::FrameBuff(HRes hRes, GLuint idFb, const Res (&att)[Att::NUM_ATTACHMENT]):
			GLFBufferCore(idFb), Token(hRes)
		{
			for(int i=0 ; i<Att::NUM_ATTACHMENT ; i++)
				boost::apply_visitor(Visitor(_ent[i]), att[i]);
		}
		void FrameBuff::exec() {
			use_begin();
			for(int i=0 ; i<Att::NUM_ATTACHMENT ; i++) {
				auto& p = _ent[i];
				if(p.idRes != 0) {
					auto flag = _AttIDtoGL(Att::Id(i));
					if(p.bTex)
						GLEC(Trap, glFramebufferTexture2D, GL_FRAMEBUFFER, flag, GL_TEXTURE_2D, p.idRes, 0);
					else
						GLEC(Trap, glFramebufferRenderbuffer, GL_FRAMEBUFFER, flag, GL_RENDERBUFFER, p.idRes);
				}
			}
			// この時点で有効なフレームバッファになって無ければエラー
			GLenum e = GL.glCheckFramebufferStatus(GL_FRAMEBUFFER);
			Assert(Trap, e == GL_FRAMEBUFFER_COMPLETE)
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
	GLenum GLFBufferCore::_AttIDtoGL(Att::Id att) {
		const GLenum c_num[Att::NUM_ATTACHMENT] = {
			GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3,
			GL_DEPTH_ATTACHMENT,
			GL_STENCIL_ATTACHMENT
		};
		return c_num[int(att)];
	}
	void GLFBuffer::onDeviceLost() {
		if(_idFbo != 0) {
			{
				auto u = use();
				for(int i=0 ; i<Att::NUM_ATTACHMENT ; i++) {
					// AttachmentのDetach
					GLEC(Trap, glFramebufferRenderbuffer, GL_FRAMEBUFFER, _AttIDtoGL(Att::Id(i)), GL_RENDERBUFFER, 0);
				}
			}
			GL.glDeleteFramebuffers(1, &_idFbo);
			_idFbo = 0;
			GLEC_Chk(Trap)
			// Attachmentの解放は各ハンドルに任せる
		}
	}
	void GLFBuffer::attach(Att::Id att, HRb hRb) {
		if(att == Att::DEPTH_STENCIL) {
			// DepthとStencilそれぞれにhRbをセットする
			attach(Att::DEPTH, hRb);
			attach(Att::STENCIL, hRb);
		} else
			_attachment[att] = HLRb(hRb);
	}
	void GLFBuffer::attach(Att::Id att, HTex hTex) {
		if(att == Att::DEPTH_STENCIL) {
			// DepthとStencilそれぞれにhTexをセットする
			attach(Att::DEPTH, hTex);
			attach(Att::STENCIL, hTex);
		} else
			_attachment[att] = HLTex(hTex);
	}
	void GLFBuffer::detach(Att::Id att) {
		_attachment[att] = boost::none;
	}
	draw::SPFb_Token GLFBuffer::getDrawToken() const {
		return std::make_shared<draw::FrameBuff>(handleFromThis(), _idFbo, _attachment);
	}
	const GLFBuffer::Res& GLFBuffer::getAttachment(Att::Id att) const {
		return _attachment[att];
	}
}
