#include "glresource.hpp"
#include "sdlwrap.hpp"
#include <functional>
#include "glhead.hpp"
#include "event.hpp"

namespace rs {
	// ------------------------- IGLTexture -------------------------
	const GLuint IGLTexture::cs_Filter[3][2] = {
		{GL_NEAREST, GL_LINEAR},
		{GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST},
		{GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR}
	};
	IGLTexture::IGLTexture(MipState miplevel, OPInCompressedFmt fmt, const spn::Size& sz, bool bCube):
		_idTex(0), _iLinearMag(0), _iLinearMin(0), _wrapS(ClampToEdge), _wrapT(ClampToEdge),
		_actID(0), _mipLevel(miplevel), _texFlag(bCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D),
		_faceFlag(bCube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_2D), _coeff(0), _size(sz), _format(fmt)
	{
		Assert(Trap, !bCube || (_size.width==_size.height))
	}
	#define FUNC_COPY(z, data, elem)	(elem(data.elem))
	#define FUNC_MOVE(z, data, elem)	(elem(std::move(data.elem)))
	#define SEQ_TEXTURE (_idTex)(_iLinearMag)(_iLinearMin)(_wrapS)(_wrapT)(_actID)\
						(_mipLevel)(_texFlag)(_faceFlag)(_coeff)(_size)(_format)
	IGLTexture::IGLTexture(IGLTexture&& t): BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(FUNC_MOVE, t, SEQ_TEXTURE)) {
		t._idTex = 0;
	}
	IGLTexture::IGLTexture(const IGLTexture& t): BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(FUNC_COPY, t, SEQ_TEXTURE)) {}

	RUser<IGLTexture> IGLTexture::use() const {
		return RUser<IGLTexture>(*this);
	}
	void IGLTexture::use_begin() const {
		GLEC_D(Trap, glActiveTexture, GL_TEXTURE0 + _actID);
		GLEC_D(Trap, glBindTexture, getTexFlag(), _idTex);
		GLEC_Chk_D(Trap);
	}
	void IGLTexture::use_end() const {
		GLEC_Chk_D(Trap);
		GLEC_D(Trap, glBindTexture, _texFlag, 0);
	}

	bool IGLTexture::_onDeviceReset() {
		if(_idTex == 0) {
			GL.glGenTextures(1, &_idTex);
			return true;
		}
		return false;
	}
	GLenum IGLTexture::getTexFlag() const {
		return _texFlag;
	}
	GLenum IGLTexture::getFaceFlag(CubeFace face) const {
		if(isCubemap())
			return _faceFlag + static_cast<int>(face) - static_cast<int>(CubeFace::PositiveX);
		return _faceFlag;
	}
	const OPInCompressedFmt& IGLTexture::getFormat() const {
		return _format;
	}
	IGLTexture::~IGLTexture() { onDeviceLost(); }
	const spn::Size& IGLTexture::getSize() const { return _size; }
	GLint IGLTexture::getTextureID() const { return _idTex; }
	void IGLTexture::setActiveID(GLuint n) { _actID = n; }
	bool IGLTexture::isMipmap() const { return  IsMipmap(_mipLevel); }
	bool IGLTexture::isCubemap() const { return _texFlag != GL_TEXTURE_2D; }
	bool IGLTexture::IsMipmap(MipState level) {
		return level >= MipmapNear;
	}
	void IGLTexture::save(const std::string& path, CubeFace face) {
		auto buff = readData(GL_BGRA, GL_UNSIGNED_BYTE, 0, face);
		auto sfc = rs::Surface::Create(buff, sizeof(uint32_t)*_size.width, _size.width, _size.height, SDL_PIXELFORMAT_ARGB8888);
		// OpenGLテクスチャは左下が原点なので…
		auto sfcVf = sfc->flipVertical();
		auto hlRW = mgr_rw.fromFile(path, RWops::Write);
		sfcVf->saveAsPNG(hlRW);
	}
	spn::ByteBuff IGLTexture::readData(GLInFmt internalFmt, GLTypeFmt elem, int level, CubeFace face) const {
		auto size = getSize();
		const size_t sz = size.width * size.height * GLFormat::QueryByteSize(internalFmt, elem);
		spn::ByteBuff buff(sz);
		#ifndef USE_OPENGLES2
		{
			auto u = use();
			GL.glGetTexImage(getFaceFlag(face), level, internalFmt, elem, buff.data());
		}
		#elif
		{
			//	OpenGL ES2ではglGetTexImageが実装されていないのでFramebufferにセットしてglReadPixelsで取得
			auto lcgl = mgr_gl.lockGL();
			GLint id;
			GL.glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &id);
			GLFBufferTmp& tmp = mgr_gl.getTmpFramebuffer();
			auto u = tmp.use();
			if(isCubemap())
				tmp.attachCubeTexture(GLFBuffer::Att::COLOR0, getTextureID(), getFaceFlag(face));
			else
				tmp.attachTexture(GLFBuffer::Att::COLOR0, getTextureID());
			GL.glReadPixels(0, 0, size.width, size.height, internalFmt, elem, buff.data());
			tmp.attachTexture(GLFBuffer::Att::COLOR0, 0);
			GL.glBindFramebuffer(GL_READ_FRAMEBUFFER_BINDING, id);
		}
		#endif
		return buff;
	}
	spn::ByteBuff IGLTexture::readRect(GLInFmt internalFmt, GLTypeFmt elem, const spn::Rect& rect, CubeFace face) const {
		const size_t sz = rect.width() * rect.height() * GLFormat::QueryByteSize(internalFmt, elem);

		auto lcgl = mgr_gl.lockGL();
		GLint id;
		GLenum flag;
		#ifndef USE_OPENGLES2
			flag = GL_READ_FRAMEBUFFER_BINDING;
		#elif
			flag = GL_FRAMEBUFFER_BINDING;
		#endif
		GL.glGetIntegerv(flag, &id);

		GLFBufferTmp& tmp = mgr_gl.getTmpFramebuffer();
		spn::ByteBuff buff(sz);
		auto u = tmp.use();
		if(isCubemap())
			tmp.attachCubeTexture(GLFBuffer::Att::COLOR0, getTextureID(), getFaceFlag(face));
		else
			tmp.attachTexture(GLFBuffer::Att::COLOR0, getTextureID());
		GL.glReadPixels(rect.x0, rect.y0, rect.width(), rect.height(), internalFmt, elem, buff.data());
		tmp.attachTexture(GLFBuffer::Att::COLOR0, 0);

		GL.glBindFramebuffer(flag, id);
		return buff;
	}
	void IGLTexture::setAnisotropicCoeff(float coeff) {
		_coeff = coeff;
	}
	void IGLTexture::setFilter(bool bLinearMag, bool bLinearMin) {
		_iLinearMag = bLinearMag ? 1 : 0;
		_iLinearMin = bLinearMin ? 1 : 0;
	}
	void IGLTexture::setLinear(bool bLinear) {
		setFilter(bLinear, bLinear);
	}
	void IGLTexture::onDeviceLost() {
		if(_idTex != 0) {
			GLW.getDrawHandler().postExecNoWait([buffId=getTextureID()](){
				GLuint id = buffId;
				GL.glDeleteTextures(1, &id);
			});
			_idTex = 0;
			GLEC_Chk_D(Warn);
		}
	}
	void IGLTexture::setUVWrap(WrapState s, WrapState t) {
		_wrapS = s;
		_wrapT = t;
	}
	void IGLTexture::setWrap(WrapState st) {
		setUVWrap(st, st);
	}
	bool IGLTexture::operator == (const IGLTexture& t) const {
		return getTextureID() == t.getTextureID();
	}
	void IGLTexture::getDrawToken(draw::TokenDst& dst, GLint id, int index, int actID) {
		using UT = draw::Texture;
		auto* ptr = dst.allocate_memory(sizeof(UT), draw::CalcTokenOffset<UT>());
		new(ptr) UT(handleFromThis(), id, index, actID, *this);
	}

	// ------------------------- Texture_Mem -------------------------
	Texture_Mem::Texture_Mem(bool bCube, GLInSizedFmt fmt, const spn::Size& sz, bool /*bStream*/, bool bRestore):
		IGLTexture(NoMipmap, fmt, sz, bCube), /*_bStream(bStream),*/ _bRestore(bRestore)
	{}
	const GLFormatDesc& Texture_Mem::_prepareBuffer() {
		auto& info = *GLFormat::QueryInfo(*_format);
		_typeFormat = info.elementType;
		_buff = spn::ByteBuff(_size.width * _size.height * GLFormat::QuerySize(info.elementType) * info.numElem);
		return info;
	}
	void Texture_Mem::onDeviceLost() {
		if(_idTex != 0) {
			if(!mgr_gl.isInDtor() && _bRestore) {
				auto& info = _prepareBuffer();
				_buff = readData(info.baseType, info.elementType, 0);
			}
			IGLTexture::onDeviceLost();
		}
	}
	void Texture_Mem::onDeviceReset() {
		if(_onDeviceReset()) {
			auto u = use();
			GL.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			if(_bRestore && _buff) {
				// バッファの内容から復元
				GLenum baseFormat = GLFormat::QueryInfo(_format.get())->baseType;
				GLEC(Warn, glTexImage2D, getFaceFlag(), 0, _format.get(), _size.width, _size.height, 0, baseFormat, _typeFormat.get(), &_buff->operator [](0));
				// DeviceがActiveな時はバッファを空にしておく
				_buff = spn::none;
				_typeFormat = spn::none;
			} else {
				// とりあえず領域だけ確保しておく
				if(isCubemap()) {
					for(int i=0 ; i<=static_cast<int>(CubeFace::NegativeZ) ; i++)
						GLEC(Warn, glTexImage2D, getFaceFlag(static_cast<CubeFace>(i)), 0, _format.get(), _size.width, _size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
				} else
					GLEC(Warn, glTexImage2D, getFaceFlag(), 0, _format.get(), _size.width, _size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			}
		}
	}
	// DeviceLostの時にこのメソッドを読んでも無意味
	void Texture_Mem::writeData(spn::AB_Byte buff, GLTypeFmt srcFmt, CubeFace face) {
		// バッファ容量がサイズ以上かチェック
		auto szInput = GLFormat::QuerySize(srcFmt);
		Assert(Trap, buff.getLength() >= _size.width*_size.height*szInput)
		// DeviceLost中でなければすぐにテクスチャを作成するが、そうでなければ内部バッファにコピーするのみ
		if(_idTex != 0) {
			// テクスチャに転送
			auto& tfm = getFormat();
			auto& sz = getSize();
			auto u = use();
			GL.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			GL.glTexImage2D(getFaceFlag(face), 0, tfm.get(), sz.width, sz.height,
							0, GLFormat::QueryInfo(*tfm)->baseType, srcFmt.get(), buff.getPtr());
			GLEC_Chk_D(Trap);
		} else {
			if(_bRestore) {
				// 内部バッファへmove
				_buff = buff.moveTo();
				_typeFormat = srcFmt;
			}
		}
	}
	void Texture_Mem::writeRect(spn::AB_Byte buff, const spn::Rect& rect, const GLTypeFmt srcFmt, const CubeFace face) {
		const size_t bs = GLFormat::QueryByteSize(_format.get(), srcFmt);
		const auto sz = buff.getLength();
		AssertP(Trap, sz >= bs*rect.width()*rect.height())
		if(_idTex != 0) {
			auto& fmt = getFormat();
			auto u = use();
			// GLテクスチャに転送
			const GLenum baseFormat = GLFormat::QueryInfo(fmt.get())->baseType;
			GL.glTexSubImage2D(getFaceFlag(face), 0, rect.x0, rect.y0, rect.width(), rect.height(), baseFormat, srcFmt.get(), buff.getPtr());
		} else {
			// 内部バッファが存在すればそこに書き込んでおく
			if(_buff) {
				// でもフォーマットが違う時は警告だけ出して何もしない
				if(*_typeFormat != srcFmt)
					Assert(Warn, false, "テクスチャのフォーマットが違うので部分的に書き込めない")
				else {
					auto& b = *_buff;
					auto* dst = &b[_size.width * rect.y0 + rect.x0];
					auto* src = buff.getPtr();
					// 1画素のバイト数
					const size_t sz = GLFormat::QueryByteSize(_format.get(), _typeFormat.get());
					for(int i=0 ; i<rect.height() ; i++) {
						std::memcpy(dst, src, rect.width());
						dst += _size.width;
						src += sz;
					}
				}
			}
		}
	}

	namespace {
		const GLenum cs_wrap[WrapState::_Num] = {
			GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_BORDER,
			GL_MIRRORED_REPEAT,
			GL_REPEAT,
			GL_MIRROR_CLAMP_TO_EDGE
		};
	}
	// ------------------------- draw::Texture -------------------------
	namespace draw {
		// --------------- Texture ---------------
		Texture::Texture(HRes hRes, GLint uid, int index, int baseActID, const IGLTexture& t): IGLTexture(t), Uniform(hRes, uid + index)
		{
			setActiveID(baseActID);
		}
		Texture::~Texture() {
			// IGLTextureのdtorでリソースを開放されないように0にセットしておく
			_idTex = 0;
		}
		void Texture::exec() {
			// 最後にBindは解除しない
			use_begin();
			{
				// setAnisotropic
				GLfloat aMax;
				GL.glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aMax);
				GL.glTexParameteri(_texFlag, GL_TEXTURE_MAX_ANISOTROPY_EXT, aMax*_coeff);
				// setUVWrap
				GL.glTexParameteri(_texFlag, GL_TEXTURE_WRAP_S, cs_wrap[_wrapS]);
				GL.glTexParameteri(_texFlag, GL_TEXTURE_WRAP_T, cs_wrap[_wrapT]);
				// setFilter
				GL.glTexParameteri(_texFlag, GL_TEXTURE_MAG_FILTER, cs_Filter[0][_iLinearMag]);
				GL.glTexParameteri(_texFlag, GL_TEXTURE_MIN_FILTER, cs_Filter[_mipLevel][_iLinearMin]);
			}
			GL.glUniform1i(idUnif, _actID);
		}

		// --------------- TextureA ---------------
		TextureA::TextureA(GLint id, const HRes* hRes, const IGLTexture** tp, int baseActID, int nTex):
			Uniform(HRes(), -1)
		{
			for(int i=0 ; i<nTex ; i++)
				_texA.emplace_back(hRes[i], id, i, baseActID+i, *tp[i]);
		}
		void TextureA::exec() {
			for(auto& p : _texA)
				p.exec();
		}
	}

	// ------------------------- Texture_StaticURI -------------------------
	Texture_StaticURI::Texture_StaticURI(const spn::URI& uri, MipState miplevel, OPInCompressedFmt fmt):
		IGLTexture(miplevel, fmt, spn::Size(0,0), false), _uri(uri) {}
	Texture_StaticURI::Texture_StaticURI(Texture_StaticURI&& t):
		IGLTexture(std::move(static_cast<IGLTexture&>(t))), _uri(std::move(t._uri)) {}
	void Texture_StaticURI::onDeviceReset() {
		if(_onDeviceReset())
			std::tie(_size, _format) = LoadTexture(*this, mgr_rw.fromURI(_uri, RWops::Read), CubeFace::PositiveX);
	}
	// メンバ関数?
	std::pair<spn::Size,GLInCompressedFmt> MakeTex(GLenum tflag, const SPSurface& sfc, OPInCompressedFmt fmt, bool bP2, bool bMip) {
		// SDLフォーマットから適したOpenGLフォーマットへ変換
		SPSurface tsfc = sfc;
		GLFormatDesc desc;
		if(fmt) {
			desc = *GLFormat::QueryInfo(*fmt);
		} else {
			// 希望するフォーマットが無ければSurfaceから決定
			auto info = GLFormat::QuerySDLtoGL(tsfc->getFormat().format);
			if(!info) {
				// INDEXEDなフォーマット等は該当が無いのでRGB24として扱う
				info = GLFormat::QuerySDLtoGL(SDL_PIXELFORMAT_RGB24);
				AssertP(Trap, info)
			}
			desc = *info;
		}
		auto sdlFmt = desc.sdlFormat!=SDL_PIXELFORMAT_UNKNOWN ? desc.sdlFormat : desc.sdlLossFormat;
		tsfc->convert(sdlFmt);
		// テクスチャ用のサイズ調整
		auto size = tsfc->getSize();
		spn::PowSize n2size{size.width, size.height};
		using CB = std::function<void (const void*)>;
		std::function<void (CB)>	func;
		// 2乗サイズ合わせではなくpitchが詰めてある場合は変換しなくていい
		if(!bP2 && tsfc->isContinuous()) {
			func = [&tsfc](CB cb) {
				auto lk = tsfc->lock();
				cb(lk.getBits());
			};
		} else {
			// 2乗サイズ合わせ
			if(bP2 && size != n2size)
				tsfc = tsfc->resize(n2size);
			func = [&tsfc, sdlFmt](CB cb) {
				auto buff = tsfc->extractAsContinuous(sdlFmt);
				cb(&buff[0]);
			};
		}
		// ミップマップの場合はサイズを縮小しながらテクスチャを書き込む
		const auto tsize = tsfc->getSize();
		size = tsize;
		int layer = 0;
		auto make = [tflag, &layer, &desc, &size](const void* data) {
			GL.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			GL.glTexImage2D(tflag, layer++, desc.format, size.width, size.height, 0, desc.baseType, desc.elementType, data);
		};
		if(!bMip)
			func(make);
		else {
			for(;;) {
				func(make);
				if(size.width==1 && size.height==1)
					break;
				size.shiftR_one(1);
				tsfc = tsfc->resize(size);
			}
		}
		return std::make_pair(tsize, desc.format);
	}
	auto MakeMip(GLenum tflag, GLenum format, const spn::Size& size, const spn::ByteBuff& buff, bool bP2, bool bMip) {
		// 簡単の為に一旦SDL_Surfaceに変換
		auto info = GLFormat::QueryInfo(format);
		int pixelsize = info->numElem* GLFormat::QuerySize(info->baseType);
		SPSurface sfc = Surface::Create(buff, pixelsize*size.width, size.width, size.height, info->sdlFormat);
		return MakeTex(tflag, sfc, spn::none, bP2, bMip);
	}
	std::pair<spn::Size, GLInCompressedFmt> Texture_StaticURI::LoadTexture(IGLTexture& tex, HRW hRW, CubeFace face) {
		SPSurface sfc = Surface::Load(hRW);
		auto tbd = tex.use();
		GLenum tflag = tex.getFaceFlag(face);
		return MakeTex(tflag, sfc, tex.getFormat(), true, tex.isMipmap());
	}

	// ------------------------- Texture_StaticCubeURI -------------------------
	Texture_StaticCubeURI::Texture_StaticCubeURI(Texture_StaticCubeURI&& t): IGLTexture(std::move(static_cast<IGLTexture&>(t))), _uri(std::move(t._uri)) {}
	Texture_StaticCubeURI::Texture_StaticCubeURI(const spn::URI& uri, MipState miplevel, OPInCompressedFmt fmt): IGLTexture(miplevel, fmt, spn::Size(0,0), true), _uri(new OPArray<spn::URI, 1>(uri)) {}
	Texture_StaticCubeURI::Texture_StaticCubeURI(const spn::URI& uri0, const spn::URI& uri1, const spn::URI& uri2,
			const spn::URI& uri3, const spn::URI& uri4, const spn::URI& uri5, MipState miplevel, OPInCompressedFmt fmt):
			IGLTexture(miplevel, fmt, spn::Size(0,0), true), _uri(new OPArray<spn::URI, 6>(uri0, uri1, uri2, uri3, uri4, uri5)) {}
	void Texture_StaticCubeURI::onDeviceReset() {
		if(_onDeviceReset()) {
			int mask = _uri->getNPacked()==1 ? 0x00 : 0xff;
			for(int i=0 ; i<6 ; i++) {
				auto ret = Texture_StaticURI::LoadTexture(*this, mgr_rw.fromURI(_uri->getPacked(i & mask), RWops::Read), static_cast<CubeFace>(i));
				if(i==0)
					std::tie(_size, _format) = ret;
			}
		}
	}

	// ------------------------- Texture_Debug -------------------------
	Texture_Debug::Texture_Debug(ITDGen* gen, const spn::Size& size, bool bCube, MipState miplevel): IGLTexture(miplevel, GLFormat::QuerySDLtoGL(gen->getFormat())->format, size, bCube), _gen(gen) {}
	void Texture_Debug::onDeviceReset() {
		if(_onDeviceReset()) {
			GLenum fmt = GLFormat::QuerySDLtoGL(this->_gen->getFormat())->format;
			auto size = getSize();
			auto u = use();
			if(isCubemap()) {
				if(_gen->isSingle()) {
					auto buff = _gen->generate(getSize());
					for(int i=0 ; i<6 ; i++)
						MakeMip(getFaceFlag(static_cast<CubeFace>(i)), fmt, size, buff, true, isMipmap());
				}
				for(int i=0 ; i<6 ; i++)
					MakeMip(getFaceFlag(static_cast<CubeFace>(i)), fmt, size, _gen->generate(getSize(), static_cast<CubeFace>(i)), true, isMipmap());
			} else
				MakeMip(getFaceFlag(), fmt, size, _gen->generate(getSize(), CubeFace::PositiveX), true, isMipmap());
		}
	}

	namespace {
		// ブレゼンハムアルゴリズム
		struct Bresen {
			int _width, _tgt;
			int _cur, _error;

			Bresen(int width, int tgt): _width(width), _tgt(tgt), _cur(0), _error(0/*width/2*/) {}
			int current() const { return _cur; }
			void advance() {
				_error += _tgt;
				if(_error >= _width) {
					_error -= _width;
					++_cur;
				}
			}
		};
	}
	// ------------------------- TDChecker -------------------------
	TDChecker::TDChecker(const spn::Vec4& col0, const spn::Vec4& col1, int nDivW, int nDivH):
		_col{col0,col1}, _nDivW(nDivW), _nDivH(nDivH)
	{}
	uint32_t TDChecker::getFormat() const { return SDL_PIXELFORMAT_RGBA8888; }
	bool TDChecker::isSingle() const { return true; }
	spn::ByteBuff TDChecker::generate(const spn::Size& size, CubeFace /*face*/) const {
		GLubyte pack[2][4];
		for(int k=0 ; k<2 ; k++) {
			for(int i=0 ; i<4 ; i++)
				pack[k][i] = static_cast<GLubyte>(_col[k].m[i] * 255 + 0.5f);
		}

		int w = size.width,
			h = size.height;
		// データの生成
		Bresen brY(h, _nDivH);
		spn::ByteBuff buff(w*h*sizeof(GLuint));
		auto* ptr = reinterpret_cast<GLuint*>(&buff[0]);
		for(int i=0 ; i<h ; i++) {
			int y = brY.current();
			Bresen brX(w, _nDivW);
			for(int j=0 ; j<w ; j++) {
				int x = brX.current();
				*ptr++ = *reinterpret_cast<GLuint*>(pack[(x^y) & 1]);
				brX.advance();
			}
			brY.advance();
		}
		return buff;
	}
	// ------------------------- TDCChecker -------------------------
	TDCChecker::TDCChecker(int /*nDivW*/, int /*nDivH*/)/*: _nDivW(nDivW), _nDivH(nDivH)*/ {}
	uint32_t TDCChecker::getFormat() const { return SDL_PIXELFORMAT_RGBA8888; }
	bool TDCChecker::isSingle() const { return true; }
	spn::ByteBuff TDCChecker::generate(const spn::Size& /*size*/, CubeFace /*face*/) const {
		const static GLubyte tex[] = {
			255, 255, 255, 255,     0,   0,   0, 255,   255, 255, 255 ,255,     0,   0,   0, 255,
			255,   0,   0, 255,     0, 255,   0, 255,     0,   0, 255 ,255,   255, 255, 255, 255,
			128,   0,   0, 255,     0, 128,   0, 255,     0,   0, 128 ,255,   128, 128, 128, 255,
			255, 255,   0, 255,   255,   0, 255, 255,     0, 255, 255 ,255,   255, 255, 255, 255,
		};
		spn::ByteBuff buff(sizeof(tex));
		memcpy(&buff[0], tex, sizeof(tex));
		return buff;
	}

	// ------------------------- TDBorder -------------------------
	TDBorder::TDBorder(const spn::Vec4&, const spn::Vec4&) {
		Assert(Trap, false, "not implemented yet")
	}
	uint32_t TDBorder::getFormat() const { return SDL_PIXELFORMAT_RGBA8888; }
	bool TDBorder::isSingle() const { return true; }
	spn::ByteBuff TDBorder::generate(const spn::Size& /*size*/, CubeFace /*face*/) const {
		Assert(Trap, false, "not implemented yet") throw 0;
	}

	const std::string& IGLTexture::getResourceName() const {
		static std::string str("IGLTexture");
		return str;
	}
}
