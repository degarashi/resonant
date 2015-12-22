#include "glresource.hpp"
#include "event.hpp"
#include "systeminfo.hpp"

namespace rs {
	// ------------------------- GLRBuffer -------------------------
	GLRBuffer::GLRBuffer(int w, int h, GLInRenderFmt fmt):
		_idRbo(0), _behLost(NONE), _restoreInfo(boost::none), _buffFmt(GLFormat::QueryInfo(fmt)->elementType), _fmt(fmt), _size(w,h)
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
			// 復帰処理が必要な場合はここでする
			cs_onLost[_behLost](mgr_gl.getTmpFramebuffer(), *this);

			GLW.getDrawHandler().postExecNoWait([buffId=_idRbo](){
				GLuint num;
				GLEC_D(Warn, glGetIntegerv, GL_RENDERBUFFER_BINDING, reinterpret_cast<GLint*>(&num));
				if(num == buffId)
					GLEC_D(Warn, glBindRenderbuffer, GL_RENDERBUFFER, 0);
				GLEC_D(Warn, glDeleteRenderbuffers, 1, &buffId);
			});

			_idRbo = 0;
		}
	}
	void GLRBuffer::allocate() {
		GL.glRenderbufferStorage(GL_RENDERBUFFER, _fmt.get(), _size.width, _size.height);
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
			spn::ByteBuff buff(texSize * rb._size.width * rb._size.height);
			GL.glReadPixels(0, 0, rb._size.width, rb._size.height, rb._fmt.get(), rb._buffFmt.get(), &buff[0]);
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
	const spn::Size& GLRBuffer::getSize() const {
		return _size;
	}
	const std::string& GLRBuffer::getResourceName() const {
		static std::string str("GLRBuffer");
		return str;
	}

	// ------------------------- GLFBufferTmp -------------------------
	GLFBufferTmp::GLFBufferTmp(GLuint idFb, const spn::Size& s):
		GLFBufferCore(idFb),
		_size(s)
	{}
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
	void GLFBufferTmp::getDrawToken(draw::TokenDst& dst) const {
		using UT = draw::FrameBuff;
		new(dst.allocate_memory(sizeof(UT), draw::CalcTokenOffset<UT>())) UT(_idFbo, mgr_info.getScreenSize());
	}

	// ------------------------- GLFBufferCore -------------------------
	GLFBufferCore::GLFBufferCore(GLuint id): _idFbo(id) {}
	RUser<GLFBufferCore> GLFBufferCore::use() const {
		return RUser<GLFBufferCore>(*this);
	}
	void GLFBufferCore::_attachRenderbuffer(Att::Id aId, GLuint rb) {
		GLEC(Trap, glFramebufferRenderbuffer, GL_FRAMEBUFFER, _AttIDtoGL(aId), GL_RENDERBUFFER, rb);
	}
	void GLFBufferCore::_attachCubeTexture(Att::Id aId, GLuint faceFlag, GLuint tb) {
		GL.glFramebufferTexture2D(GL_FRAMEBUFFER, _AttIDtoGL(aId), faceFlag, tb, 0);
	}
	void GLFBufferCore::_attachTexture(Att::Id aId, GLuint tb) {
		_attachCubeTexture(aId, GL_TEXTURE_2D, tb);
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

			void operator()(const RawRb& r) const {
				_dst.bTex = false;
				_dst.idRes = r;
			}
			void operator()(const RawTex& t) const {
				_dst.bTex = true;
				_dst.faceFlag = GL_TEXTURE_2D;
				_dst.idRes = t;
			}
			void operator()(const TexRes& t) const {
				auto* tr = t.first.cref().get();
				_dst.bTex = true;
				_dst.faceFlag = tr->getFaceFlag(t.second);
				_dst.handle = t;
				_dst.idRes = tr->getTextureID();
			}
			void operator()(const HLRb& hlRb) const {
				_dst.bTex = false;
				_dst.handle = hlRb;
				_dst.faceFlag = 0;
				_dst.idRes = hlRb.cref()->getBufferID();
			}
			void operator()(boost::none_t) const {
				_dst.idRes = 0;
			}
		};
		FrameBuff::FrameBuff(GLuint idFb, const spn::Size& s):
			GLFBufferCore(idFb), TokenR(HRes()),
			_size(s)
		{
			for(auto& p : _ent)
				p.idRes = 0;
		}
		FrameBuff::FrameBuff(HRes hRes, GLuint idFb, const Res (&att)[Att::NUM_ATTACHMENT]):
			GLFBufferCore(idFb), TokenR(hRes),
			_size(*GLFBuffer::GetAttachmentSize(att, Att::COLOR0))
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
						GLEC(Trap, glFramebufferTexture2D, GL_FRAMEBUFFER, flag, p.faceFlag, p.idRes, 0);
					else
						GLEC(Trap, glFramebufferRenderbuffer, GL_FRAMEBUFFER, flag, GL_RENDERBUFFER, p.idRes);
				}
			}
			// この時点で有効なフレームバッファになって無ければエラー
			GLenum e = GL.glCheckFramebufferStatus(GL_FRAMEBUFFER);
			Assert(Trap, e==GL_FRAMEBUFFER_COMPLETE, GLFormat::QueryEnumString(e).c_str())

			// 後で参照するためにFramebuff(Color0)のサイズを記録
			GLFBufferCore::_SetCurrentFBSize(_size);
		}

		Viewport::Viewport(bool bPixel, const spn::RectF& r):
			_bPixel(bPixel),
			_rect(r)
		{}
		void Viewport::exec() {
			spn::RectF r = _rect;
			if(!_bPixel) {
				spn::Size s = GLFBufferCore::GetCurrentFBSize();
				r.x0 *= s.width;
				r.x1 *= s.width;
				r.y0 *= s.height;
				r.y1 *= s.height;
			}
			GL.glViewport(r.x0, r.y0, r.width(), r.height());
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
			void operator()(GLFBufferCore::RawTex& t) const {
				// 生のTextureIdは無効になる
				t = 0; }
			void operator()(GLFBufferCore::RawRb& r) const {
				// 生のRenderBufferIdは無効になる
				r = 0; }
			void operator()(GLFBufferCore::TexRes& t) const {
				(*this)(t.first);
			}
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
#ifndef USE_OPENGLES2
			GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3,
#endif
			GL_DEPTH_ATTACHMENT,
			GL_STENCIL_ATTACHMENT
		};
		return c_num[int(att)];
	}
	void GLFBuffer::onDeviceLost() {
		if(_idFbo != 0) {
			GLW.getDrawHandler().postExecNoWait([buffId=getBufferID()](){
				GLEC_D(Warn, glBindFramebuffer, GL_FRAMEBUFFER, buffId);
				for(int i=0 ; i<Att::NUM_ATTACHMENT ; i++) {
					// AttachmentのDetach
					GLEC_D(Warn, glFramebufferRenderbuffer, GL_FRAMEBUFFER, _AttIDtoGL(Att::Id(i)), GL_RENDERBUFFER, 0);
				}
				GLEC_D(Warn, glBindFramebuffer, GL_FRAMEBUFFER, 0);
				GLEC_D(Warn, glDeleteFramebuffers, 1, &buffId);
				GLEC_Chk(Trap)
			});
			_idFbo = 0;
			// Attachmentの解放は各ハンドルに任せる
		}
	}
	template <class T>
	void GLFBuffer::_attachIt(Att::Id att, const T& arg) {
		if(att == Att::DEPTH_STENCIL) {
			// DepthとStencilそれぞれにhTexをセットする
			_attachIt(Att::DEPTH, arg);
			_attachIt(Att::STENCIL, arg);
		} else
			_attachment[att] = arg;
	}
	void GLFBuffer::attachRBuffer(Att::Id att, HRb hRb) {
		_attachIt(att, HLRb(hRb));
	}
	void GLFBuffer::attachTextureFace(Att::Id att, HTex hTex, CubeFace face) {
		_attachIt(att, TexRes(hTex, face));
	}
	void GLFBuffer::attachTexture(Att::Id att, HTex hTex) {
		attachTextureFace(att, hTex, CubeFace::PositiveX);
	}
	void GLFBuffer::attachRawRBuffer(Att::Id att, GLuint idRb) {
		_attachIt(att, RawRb(idRb));
	}
	void GLFBuffer::attachRawTexture(Att::Id att, GLuint idTex) {
		_attachIt(att, RawTex(idTex));
	}
	void GLFBuffer::attachOther(Att::Id attDst, Att::Id attSrc, HFb hFb) {
		_attachment[attDst] = hFb->get()->getAttachment(attSrc);
	}
	void GLFBuffer::detach(Att::Id att) {
		_attachment[att] = boost::none;
	}
	void GLFBuffer::getDrawToken(draw::TokenDst& dst) const {
		using UT = draw::FrameBuff;
		new(dst.allocate_memory(sizeof(UT), draw::CalcTokenOffset<UT>())) UT(handleFromThis(), _idFbo, _attachment);
	}
	const GLFBuffer::Res& GLFBuffer::getAttachment(Att::Id att) const {
		return _attachment[att];
	}
	namespace {
		struct GetSize_Visitor : boost::static_visitor<Size_OP> {
			Size_OP operator()(boost::none_t) const {
				return spn::none;
			}
			Size_OP operator()(const GLFBufferCore::TexRes& t) const {
				return (*this)(t.first);
			}
			template <class T>
			Size_OP operator()(const T& t) const {
				return t->get()->getSize();
			}
			Size_OP operator()(const GLFBufferCore::RawTex& t) const {
				GLint id, w, h;
				GL.glGetIntegerv(GL_TEXTURE_BINDING_2D, &id);
				GL.glBindTexture(GL_TEXTURE_2D, t._value);
				GL.glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WIDTH, &w);
				GL.glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_HEIGHT, &h);
				GL.glBindTexture(GL_TEXTURE_2D, id);
				return spn::Size(w,h);
			}
			Size_OP operator()(const GLFBufferCore::RawRb& t) const {
				GLint id, w, h;
				GL.glGetIntegerv(GL_RENDERBUFFER_BINDING, &id);
				GL.glBindRenderbuffer(GL_RENDERBUFFER, t._value);
				GL.glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &w);
				GL.glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &h);
				GL.glBindRenderbuffer(GL_RENDERBUFFER, id);
				return spn::Size(w,h);
			}
		};
	}
	Size_OP GLFBuffer::GetAttachmentSize(const Res (&att)[Att::NUM_ATTACHMENT], Att::Id id) {
		return boost::apply_visitor(GetSize_Visitor(), att[id]);
	}
	Size_OP GLFBuffer::getAttachmentSize(Att::Id att) const {
		return GetAttachmentSize(_attachment, att);
	}
	const std::string& GLFBuffer::getResourceName() const {
		static std::string str("GLFBuffer");
		return str;
	}

	thread_local spn::Size GLFBufferCore::s_fbSize;
	void GLFBufferCore::_SetCurrentFBSize(const spn::Size& s) {
		s_fbSize = s;
	}
	const spn::Size& GLFBufferCore::GetCurrentFBSize() {
		return s_fbSize;
	}
}

#include "luaw.hpp"
namespace rs {
	void GLFBuffer::LuaExport(LuaState& lsc) {
		auto tbl = std::make_shared<LCTable>();
		(*tbl)["Color0"] = lua_Integer(Att::COLOR0);
		#ifndef USE_OPENGLES2
			(*tbl)["Color1"] = lua_Integer(Att::COLOR1);
			(*tbl)["Color2"] = lua_Integer(Att::COLOR2);
			(*tbl)["Color3"] = lua_Integer(Att::COLOR3);
		#endif
		(*tbl)["Depth"] = lua_Integer(Att::DEPTH);
		(*tbl)["Stencil"] = lua_Integer(Att::STENCIL);
		(*tbl)["DepthStencil"] = lua_Integer(Att::DEPTH_STENCIL);
		lsc.setField(-1, "Attribute", tbl);
	}
}
