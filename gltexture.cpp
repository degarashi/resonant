#include "glresource.hpp"
#include "sdlwrap.hpp"
#include <functional>
#include "glhead.hpp"

namespace rs {
	// ------------------------- IGLTexture -------------------------
	const GLuint IGLTexture::cs_Filter[3][2] = {
		{GL_NEAREST, GL_LINEAR},
		{GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR},
		{GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR}
	};
	IGLTexture::IGLTexture(OPInCompressedFmt fmt, const spn::Size& sz, bool bCube):
		_idTex(0), _iLinearMag(0), _iLinearMin(0), _iWrapS(GL_CLAMP_TO_EDGE), _iWrapT(GL_CLAMP_TO_EDGE),
		_actID(0), _mipLevel(NoMipmap), _texFlag(bCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D),
		_faceFlag(bCube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_2D), _coeff(0), _size(sz), _format(fmt) {}
	#define FUNC_COPY(z, data, elem)	(elem(data.elem))
	#define FUNC_MOVE(z, data, elem)	(elem(std::move(data.elem)))
	#define SEQ_TEXTURE (_idTex)(_iLinearMag)(_iLinearMin)(_iWrapS)(_iWrapT)(_actID)\
						(_mipLevel)(_texFlag)(_faceFlag)(_coeff)(_size)(_format)(_bReset)
	#define SEQ_TEXTURE_M	SEQ_TEXTURE(_preFunc)
	IGLTexture::IGLTexture(IGLTexture&& t): BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(FUNC_MOVE, t, SEQ_TEXTURE_M)) {
		t._idTex = 0;
		t._bReset = false;
		t._preFunc = nullptr;
	}
	IGLTexture::IGLTexture(const IGLTexture& t): BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(FUNC_COPY, t, SEQ_TEXTURE)) {
		// PreFuncを持っている状態ではコピー禁止
		Assert(Trap, !static_cast<bool>(t._preFunc))
	}

	RUser<IGLTexture> IGLTexture::use() const {
		return RUser<IGLTexture>(*this);
	}
	void IGLTexture::use_begin() const {
		GLEC_D(Trap, glActiveTexture, GL_TEXTURE0 + _actID);
		GLEC_D(Trap, glBindTexture, _texFlag, _idTex);
		GLEC_Chk_D(Trap);
	}
	void IGLTexture::use_end() const {
		GLEC_Chk_D(Trap);
		GLEC_D(Trap, glBindTexture, _texFlag, 0);
	}

	bool IGLTexture::_onDeviceReset() {
		if(_idTex == 0) {
			GLEC_D(Warn, glGenTextures, 1, &_idTex);
			return true;
		}
		return false;
	}
	GLenum IGLTexture::getTexFlag() const {
		return _texFlag;
	}
	GLenum IGLTexture::getFaceFlag() const {
		return _faceFlag;
	}
	const GLInCompressedFmt& IGLTexture::getFormat() const {
		return *_format;
	}
	IGLTexture::~IGLTexture() { onDeviceLost(); }
	const spn::Size& IGLTexture::getSize() const { return _size; }
	GLint IGLTexture::getTextureID() const { return _idTex; }
	void IGLTexture::setActiveID(GLuint n) { _actID = n; }
	bool IGLTexture::isMipmap() const { return  IsMipmap(_mipLevel); }
	bool IGLTexture::isCubemap() const { return _texFlag != GL_TEXTURE_2D; }
	bool IGLTexture::IsMipmap(State level) {
		return level >= MipmapNear;
	}
	void IGLTexture::save(const std::string& path) {
		auto saveFmt = GL_RGBA;
		size_t sz = _size.width * _size.height * GLFormat::QueryByteSize(saveFmt, GL_UNSIGNED_BYTE);
		spn::ByteBuff buff(sz);
		#ifndef USE_OPENGLES2
			{
				// OpenGL ES2では無効
				auto u = use();
				GL.glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, &buff[0]);
			}
		#else
			Assert(Trap, false, "not implemented yet");
		#endif
		auto sfc = rs::Surface::Create(buff, sizeof(uint32_t)*_size.width, _size.width, _size.height, SDL_PIXELFORMAT_ARGB8888);
		auto hlRW = mgr_rw.fromFile(path, RWops::Write);
		sfc->saveAsPNG(hlRW);
	}
	void IGLTexture::setAnisotropicCoeff(float coeff) {
		_coeff = coeff;
	}
	void IGLTexture::_reallocate() {
		UniLock lk(_mutex);
		onDeviceLost();
		onDeviceReset();
	}
	void IGLTexture::setFilter(State miplevel, bool bLinearMag, bool bLinearMin) {
		_iLinearMag = bLinearMag ? 1 : 0;
		_iLinearMin = bLinearMin ? 1 : 0;
		bool b = isMipmap() ^ IsMipmap(miplevel);
		_mipLevel = miplevel;
		if(b) {
			// ミップマップの有りなしを切り替える時はテクスチャを作りなおす
			_bReset = true;
		}
	}

	void IGLTexture::onDeviceLost() {
		if(_idTex != 0) {
			GL.glDeleteTextures(1, &_idTex);
			_idTex = 0;
			_bReset = false;
			GLEC_Chk_D(Warn);
		}
	}
	void IGLTexture::setUVWrap(GLuint s, GLuint t) {
		_iWrapS = s;
		_iWrapT = t;
	}
	bool IGLTexture::operator == (const IGLTexture& t) const {
		return getTextureID() == t.getTextureID();
	}
	draw::SPToken IGLTexture::getDrawToken(IPreFunc& pf, GLint id, int index, int actID, HRes hRes) {
		if(_preFunc)
			pf.addPreFunc(std::move(_preFunc));
		if(_bReset) {
			_bReset = false;
			_reallocate();
		}
		draw::SPToken ret = std::make_shared<draw::Texture>(hRes, id, index, actID, *this);
		return std::move(ret);
	}

	// ------------------------- Texture_Mem -------------------------
	Texture_Mem::Texture_Mem(bool bCube, GLInSizedFmt fmt, const spn::Size& sz, bool bStream, bool bRestore):
		IGLTexture(fmt, sz, bCube), /*_bStream(bStream),*/ _bRestore(bRestore)
	{}
	Texture_Mem::Texture_Mem(bool bCube, GLInSizedFmt fmt, const spn::Size& sz, bool bStream, bool bRestore, GLTypeFmt srcFmt, spn::AB_Byte buff):
		Texture_Mem(bCube, fmt, sz, bStream, bRestore)
	{
		writeData(buff, srcFmt);
	}
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
	#ifdef USE_OPENGLES2
				//	OpenGL ES2ではglTexImage2Dが実装されていないのでFramebufferにセットしてglReadPixelsで取得
				GLFBufferTmp& tmp = mgr_gl.getTmpFramebuffer();
				auto u = tmp.use();
				tmp.attach(GLFBuffer::COLOR0, _idTex);
				GLEC(Trap, glReadPixels, 0, 0, _size.width, _size.height, info.baseType, info.elementType, &_buff->operator[](0));
				tmp.attach(GLFBuffer::COLOR0, 0);
	#else
				auto u = use();
				GLEC(Warn, glGetTexImage, GL_TEXTURE_2D, 0, info.baseType, info.elementType, &_buff->operator[](0));
	#endif
			}
			IGLTexture::onDeviceLost();
		}
	}
	void Texture_Mem::onDeviceReset() {
		if(_onDeviceReset()) {
			auto u = use();
			if(_bRestore && _buff) {
				// バッファの内容から復元
				GLenum baseFormat = GLFormat::QueryInfo(_format.get())->baseType;
				GLEC(Warn, glTexImage2D, GL_TEXTURE_2D, 0, _format.get(), _size.width, _size.height, 0, baseFormat, _typeFormat.get(), &_buff->operator [](0));
				// DeviceがActiveな時はバッファを空にしておく
				_buff = spn::none;
				_typeFormat = spn::none;
			} else {
				// とりあえず領域だけ確保しておく
				GLEC(Warn, glTexImage2D, GL_TEXTURE_2D, 0, _format.get(), _size.width, _size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
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
			std::shared_ptr<spn::ByteBuff> pbuff(new spn::ByteBuff(buff.moveTo()));
			_preFunc = [this, srcFmt, pbuff]() {
				auto& tfm = getFormat();
				auto& sz = getSize();
				use_begin();
				GL.glTexImage2D(GL_TEXTURE_2D, 0, tfm.get(), sz.width, sz.height,
								0, tfm.get(), srcFmt.get(), pbuff->data());
				GLEC_Chk_D(Trap);
			};
		} else {
			if(_bRestore) {
				// 内部バッファへmove
				_buff = buff.moveTo();
				_typeFormat = srcFmt;
			}
		}
	}
	void Texture_Mem::writeRect(spn::AB_Byte buff, int width, int ofsX, int ofsY, GLTypeFmt srcFmt, CubeFace face) {
		size_t bs = GLFormat::QueryByteSize(_format.get(), GL_UNSIGNED_BYTE);
		auto sz = buff.getLength();
		int height = sz / (width * bs);
		if(_idTex != 0) {
			std::shared_ptr<spn::ByteBuff> pbuff(new spn::ByteBuff(buff.moveTo()));
			std::shared_ptr<PreFunc> pf(new PreFunc(std::move(_preFunc)));
			auto& fmt = getFormat();
			_preFunc = [=]() {
				if(*pf)
					(*pf)();
				auto u = use();
				// GLテクスチャに転送
				GLenum baseFormat = GLFormat::QueryInfo(fmt.get())->baseType;
				GL.glTexSubImage2D(GL_TEXTURE_2D, 0, ofsX, ofsY, width, height, baseFormat, srcFmt.get(), pbuff->data());
			};
		} else {
			// 内部バッファが存在すればそこに書き込んでおく
			if(_buff) {
				// でもフォーマットが違う時は警告だけ出して何もしない
				if(*_typeFormat != srcFmt)
					Assert(Warn, false, "テクスチャのフォーマットが違うので部分的に書き込めない")
				else {
					auto& b = *_buff;
					auto* dst = &b[_size.width * ofsY + ofsX];
					auto* src = buff.getPtr();
					// 1画素のバイト数
					size_t sz = GLFormat::QueryByteSize(_format.get(), _typeFormat.get());
					for(int i=0 ; i<height ; i++) {
						std::memcpy(dst, src, width);
						dst += _size.width;
						src += sz;
					}
				}
			}
		}
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
				GL.glTexParameteri(_texFlag, GL_TEXTURE_WRAP_S, _iWrapS);
				GL.glTexParameteri(_texFlag, GL_TEXTURE_WRAP_T, _iWrapT);
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
	Texture_StaticURI::Texture_StaticURI(const spn::URI& uri, OPInCompressedFmt fmt):
		IGLTexture(fmt, spn::Size(0,0), false), _uri(uri) {}
	Texture_StaticURI::Texture_StaticURI(Texture_StaticURI&& t):
		IGLTexture(std::move(static_cast<IGLTexture&>(t))), _uri(std::move(t._uri)) {}
	void Texture_StaticURI::onDeviceReset() {
		if(_onDeviceReset())
			_size = LoadTexture(*this, mgr_rw.fromURI(_uri, RWops::Read), CubeFace::PositiveX);
	}
	// メンバ関数?
	spn::Size MakeTex(GLenum tflag, const SPSurface& sfc, bool bP2, bool bMip) {
		// SDLフォーマットから適したOpenGLフォーマットへ変換
		SPSurface tsfc = sfc;
		uint32_t fmt = tsfc->getFormat().format;
		auto info = GLFormat::QuerySDLtoGL(fmt);
		if(!info) {
			// INDEXEDなフォーマット等は該当が無いのでRGB24として扱う
			info = GLFormat::QuerySDLtoGL(SDL_PIXELFORMAT_RGB24);
			AssertP(Trap, info)
		}
		fmt = info->sdlFormat;
		tsfc = tsfc->convert(fmt);

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
			func = [&tsfc, &info](CB cb) {
				auto buff = tsfc->extractAsContinuous(info->sdlFormat);
				cb(&buff[0]);
			};
		}
		// ミップマップの場合はサイズを縮小しながらテクスチャを書き込む
		size = tsfc->getSize();
		auto ret = size;
		int layer = 0;
		auto make = [tflag, &layer, &info, &size](const void* data) {
			GL.glTexImage2D(tflag, layer++, info->format, size.width, size.height, 0, info->baseType, info->elementType, data);
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
		return ret;
	}
	spn::Size MakeMip(GLenum tflag, GLenum format, const spn::Size& size, const spn::ByteBuff& buff, bool bP2, bool bMip) {
		// 簡単の為に一旦SDL_Surfaceに変換
		auto info = GLFormat::QueryInfo(format);
		int pixelsize = info->numElem* GLFormat::QuerySize(info->baseType);
		SPSurface sfc = Surface::Create(buff, pixelsize*size.width, size.width, size.height, info->sdlFormat);
		return MakeTex(tflag, sfc, bP2, bMip);
	}
	spn::Size Texture_StaticURI::LoadTexture(IGLTexture& tex, HRW hRW, CubeFace face) {
		SPSurface sfc = Surface::Load(hRW);
		auto tbd = tex.use();
		GLenum tflag = tex.getFaceFlag() + static_cast<int>(face) - static_cast<int>(CubeFace::PositiveX);
		return MakeTex(tflag, sfc, true, tex.isMipmap());
	}

	// ------------------------- Texture_StaticCubeURI -------------------------
	Texture_StaticCubeURI::Texture_StaticCubeURI(Texture_StaticCubeURI&& t): IGLTexture(std::move(static_cast<IGLTexture&>(t))), _uri(std::move(t._uri)) {}
	Texture_StaticCubeURI::Texture_StaticCubeURI(const spn::URI& uri, OPInCompressedFmt fmt): IGLTexture(fmt, spn::Size(0,0), true), _uri(new OPArray<spn::URI, 1>(uri)) {}
	Texture_StaticCubeURI::Texture_StaticCubeURI(const spn::URI& uri0, const spn::URI& uri1, const spn::URI& uri2,
			const spn::URI& uri3, const spn::URI& uri4, const spn::URI& uri5, OPInCompressedFmt fmt):
			IGLTexture(fmt, spn::Size(0,0), true), _uri(new OPArray<spn::URI, 6>(uri0, uri1, uri2, uri3, uri4, uri5)) {}
	void Texture_StaticCubeURI::onDeviceReset() {
		if(_onDeviceReset()) {
			int mask = _uri->getNPacked()==1 ? 0x00 : 0xff;
			for(int i=0 ; i<6 ; i++)
				_size = Texture_StaticURI::LoadTexture(*this, mgr_rw.fromURI(_uri->getPacked(i & mask), RWops::Read), static_cast<CubeFace>(i));
		}
	}

	// ------------------------- Texture_Debug -------------------------
	Texture_Debug::Texture_Debug(ITDGen* gen, const spn::Size& size, bool bCube): IGLTexture(GLFormat::QuerySDLtoGL(gen->getFormat())->format, size, bCube), _gen(gen) {}
	void Texture_Debug::onDeviceReset() {
		if(_onDeviceReset()) {
			GLenum fmt = GLFormat::QuerySDLtoGL(this->_gen->getFormat())->format;
			auto size = getSize();
			auto u = use();
			if(isCubemap()) {
				if(_gen->isSingle()) {
					auto buff = _gen->generate(getSize());
					for(int i=0 ; i<6 ; i++)
						MakeMip(getFaceFlag()+i, fmt, size, buff, true, isMipmap());
				}
				for(int i=0 ; i<6 ; i++)
					MakeMip(getFaceFlag()+i, fmt, size, _gen->generate(getSize(), static_cast<CubeFace>(i)), true, isMipmap());
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
	spn::ByteBuff TDChecker::generate(const spn::Size& size, CubeFace face) const {
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
		return std::move(buff);
	}
	// ------------------------- TDCChecker -------------------------
	TDCChecker::TDCChecker(int nDivW, int nDivH)/*: _nDivW(nDivW), _nDivH(nDivH)*/ {}
	uint32_t TDCChecker::getFormat() const { return SDL_PIXELFORMAT_RGBA8888; }
	bool TDCChecker::isSingle() const { return true; }
	spn::ByteBuff TDCChecker::generate(const spn::Size& size, CubeFace face) const {
		const static GLubyte tex[] = {
			255, 255, 255, 255,     0,   0,   0, 255,   255, 255, 255 ,255,     0,   0,   0, 255,
			255,   0,   0, 255,     0, 255,   0, 255,     0,   0, 255 ,255,   255, 255, 255, 255,
			128,   0,   0, 255,     0, 128,   0, 255,     0,   0, 128 ,255,   128, 128, 128, 255,
			255, 255,   0, 255,   255,   0, 255, 255,     0, 255, 255 ,255,   255, 255, 255, 255,
		};
		spn::ByteBuff buff(sizeof(tex));
		memcpy(&buff[0], tex, sizeof(tex));
		return std::move(buff);
	}

	// ------------------------- TDBorder -------------------------
	TDBorder::TDBorder(const spn::Vec4&, const spn::Vec4&) {
		Assert(Trap, false, "not implemented yet")
	}
	uint32_t TDBorder::getFormat() const { return SDL_PIXELFORMAT_RGBA8888; }
	bool TDBorder::isSingle() const { return true; }
	spn::ByteBuff TDBorder::generate(const spn::Size& size, CubeFace face) const {
		Assert(Trap, false, "not implemented yet") throw 0;
	}
}
