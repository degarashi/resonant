#include "glresource.hpp"
#include "sdlwrap.hpp"
#include <functional>
#include "glhead.hpp"

namespace rs {
	// ------------------------- IGLTexture -------------------------
	DEF_GLRESOURCE_CPP(IGLTexture)
	const GLuint IGLTexture::cs_Filter[3][2] = {
		{GL_NEAREST, GL_LINEAR},
		{GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR},
		{GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR}
	};
	IGLTexture::IGLTexture(OPInCompressedFmt fmt, const spn::Size& sz, bool bCube):
		_idTex(0), _iLinearMag(0), _iLinearMin(0), _iWrapS(GL_CLAMP_TO_EDGE), _iWrapT(GL_CLAMP_TO_EDGE), _size(sz),
		_actID(0), _mipLevel(NoMipmap), _texFlag(bCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D),
		_faceFlag(bCube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_2D), _format(fmt), _coeff(0) {}
	#define FUNC_COPY(z, data, elem)	(elem(data.elem))
	#define SEQ_COPY (_idTex)(_iLinearMag)(_iLinearMin)(_iWrapS)(_iWrapT)(_size)(_actID)(_mipLevel)(_texFlag)(_faceFlag)(_format)(_coeff)
	IGLTexture::IGLTexture(IGLTexture&& t): BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(FUNC_COPY, t, SEQ_COPY)) {
		t._idTex = 0;
	}
	void IGLTexture::Use(IGLTexture& t) {
		GLEC_P(Trap, glActiveTexture, GL_TEXTURE0 + t._actID);
		GLEC_P(Trap, glBindTexture, t._texFlag, t._idTex);
		GLEC_ChkP(Trap)
	}
	void IGLTexture::End(IGLTexture& t) {
		GLEC_ChkP(Trap)
		GLEC_P(Trap, glBindTexture, t._texFlag, 0);
	}

	bool IGLTexture::_onDeviceReset() {
		if(_idTex == 0) {
			GLEC_P(Warn, glGenTextures, 1, &_idTex);
			// フィルタの設定
			use()->setFilter(_mipLevel, static_cast<bool>(_iLinearMag), static_cast<bool>(_iLinearMin))
				->setAnisotropicCoeff(_coeff)
				->setUVWrap(_iWrapS, _iWrapT);
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
		size_t sz = _size.width * _size.height * GLFormat::QueryByteSize(GL_RGBA8, GL_UNSIGNED_BYTE);
		spn::ByteBuff buff(sz);
		#ifndef USE_OPENGLES2
			// OpenGL ES2では無効
			auto u = use();
			glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, &buff[0]);
			u->end();
		#else
			AssertMsg(Trap, false, "not implemented yet");
		#endif
		auto sfc = rs::Surface::Create(buff, sizeof(uint32_t)*_size.width, _size.width, _size.height, SDL_PIXELFORMAT_ARGB8888);
		auto hlRW = mgr_rw.fromFile(path, RWops::Write, true);
		sfc->saveAsPNG(hlRW);
	}
	IGLTexture::Inner1& IGLTexture::setAnisotropicCoeff(float coeff) {
		_coeff = coeff;
		GLfloat aMax;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aMax);
		glTexParameteri(_texFlag, GL_TEXTURE_MAX_ANISOTROPY_EXT, aMax*_coeff);
		return Inner1::Cast(this);
	}
	IGLTexture::Inner1& IGLTexture::setFilter(State miplevel, bool bLinearMag, bool bLinearMin) {
		_iLinearMag = bLinearMag ? 1 : 0;
		_iLinearMin = bLinearMin ? 1 : 0;
		bool b = isMipmap() ^ IsMipmap(miplevel);
		_mipLevel = miplevel;
		if(b) {
			// ミップマップの有りなしを切り替える時はテクスチャを作りなおす
			End(*this);
			onDeviceLost();
			onDeviceReset();
			Use(*this);
		}
		glTexParameteri(_texFlag, GL_TEXTURE_MAG_FILTER, cs_Filter[0][_iLinearMag]);
		glTexParameteri(_texFlag, GL_TEXTURE_MIN_FILTER, cs_Filter[_mipLevel][_iLinearMin]);

		return Inner1::Cast(this);
	}

	void IGLTexture::onDeviceLost() {
		if(_idTex != 0) {
			glDeleteTextures(1, &_idTex);
			_idTex = 0;
			GLEC_ChkP(Warn)
		}
	}
	IGLTexture::Inner1& IGLTexture::setUVWrap(GLuint s, GLuint t) {
		_iWrapS = s;
		_iWrapT = t;

		glTexParameteri(_texFlag, GL_TEXTURE_WRAP_S, _iWrapS);
		glTexParameteri(_texFlag, GL_TEXTURE_WRAP_T, _iWrapT);
		return Inner1::Cast(this);
	}
	bool IGLTexture::operator == (const IGLTexture& t) const {
		return getTextureID() == t.getTextureID();
	}

	// ------------------------- Texture_Mem -------------------------
	Texture_Mem::Texture_Mem(bool bCube, GLInSizedFmt fmt, const spn::Size& sz, bool bStream, bool bRestore):
		IGLTexture(fmt, sz, bCube), _bStream(bStream), _bRestore(bRestore)
	{}
	Texture_Mem::Texture_Mem(bool bCube, GLInSizedFmt fmt, const spn::Size& sz, bool bStream, bool bRestore, GLTypeFmt srcFmt, spn::AB_Byte buff):
		Texture_Mem(bCube, fmt, sz, bStream, bRestore)
	{
		writeData(buff, srcFmt);
	}
	const GLFormatDesc& Texture_Mem::_prepareBuffer() {
		auto& info = *GLFormat::QueryInfo(*_format);
		_typeFormat = info.toType;
		_buff = spn::ByteBuff(_size.width * _size.height * GLFormat::QuerySize(info.toType) * info.numType);
		return info;
	}
	void Texture_Mem::onDeviceLost() {
		if(_idTex != 0) {
			if(!mgr_gl.isInDtor() && _bRestore) {
				auto& info = _prepareBuffer();
	#ifdef USE_OPENGLES2
				//	OpenGL ES2ではglTexImage2Dが実装されていないのでFramebufferにセットしてglReadPixelsで取得
				GLFBufferTmp& tmp = mgr_gl.getTmpFramebuffer();
				auto u = *tmp;
				u->attachColor(0, _idTex);
				GLEC(Trap, glReadPixels, 0, 0, _size.width, _size.height, info.toBase, info.toType, &_buff->operator[](0));
				u->attachColor(0, 0);
	#else
				auto u = use();
				GLEC(Warn, glGetTexImage, GL_TEXTURE_2D, 0, info.toBase, info.toType, &_buff->operator[](0));
				u->end();
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
				GLenum baseFormat = GLFormat::QueryInfo(_format.get())->toBase;
				GLEC(Warn, glTexImage2D, GL_TEXTURE_2D, 0, _format.get(), _size.width, _size.height, 0, baseFormat, _typeFormat.get(), &_buff->operator [](0));
				// DeviceがActiveな時はバッファを空にしておく
				_buff = spn::none;
				_typeFormat = spn::none;
			} else {
				// とりあえず領域だけ確保しておく
				GLEC(Warn, glTexImage2D, GL_TEXTURE_2D, 0, _format.get(), _size.width, _size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			}
			u->end();
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
			glTexImage2D(GL_TEXTURE_2D, 0, _format.get(), _size.width, _size.height, 0, _format.get(), srcFmt.get(), buff.getPtr());
			GLEC_Chk(Trap)
		} else {
			if(_bRestore) {
				// 内部バッファへmove
				_buff = spn::ByteBuff();
				buff.setTo(*_buff);
				_typeFormat = srcFmt;
			}
		}
	}
	void Texture_Mem::writeRect(spn::AB_Byte buff, int width, int ofsX, int ofsY, GLTypeFmt srcFmt, CubeFace face) {
		size_t bs = GLFormat::QueryByteSize(_format.get(), GL_UNSIGNED_BYTE);
		auto sz = buff.getLength();
		int height = sz / (width * bs);
		if(_idTex != 0) {
			auto u = use();
			// GLテクスチャに転送
			GLenum baseFormat = GLFormat::QueryInfo(_format.get())->toBase;
			glTexSubImage2D(GL_TEXTURE_2D, 0, ofsX, ofsY, width, height, baseFormat, srcFmt.get(), buff.getPtr());
			u->end();
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
	// ------------------------- Texture_StaticURI -------------------------
	Texture_StaticURI::Texture_StaticURI(const spn::URI& uri, OPInCompressedFmt fmt):
		IGLTexture(fmt, spn::Size(0,0), false), _uri(uri) {}
	Texture_StaticURI::Texture_StaticURI(Texture_StaticURI&& t):
		IGLTexture(std::move(static_cast<IGLTexture&>(t))), _uri(std::move(t._uri)) {}
	void Texture_StaticURI::onDeviceReset() {
		if(_onDeviceReset())
			_size = LoadTexture(*this, mgr_rw.fromURI(_uri, RWops::Read, true), CubeFace::PositiveX);
	}
	// メンバ関数?
	spn::Size MakeTex(GLenum tflag, const SPSurface& sfc, bool bP2, bool bMip) {
		// SDLフォーマットから適したOpenGLフォーマットへ変換
		SPSurface tsfc = sfc;
		uint32_t fmt = tsfc->getFormat().format;
		if(auto op = GLFormat::QuerySDLtoSDLGL(fmt)) {
			fmt = *op;
			tsfc = tsfc->convert(fmt);
		}
		auto info = GLFormat::QuerySDLtoGL(fmt);
		Assert(Trap, info)

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
				auto buff = tsfc->extractAsContinuous(info->toSDLFormat);
				cb(&buff[0]);
			};
		}
		// ミップマップの場合はサイズを縮小しながらテクスチャを書き込む
		size = tsfc->getSize();
		auto ret = size;
		int layer = 0;
		auto make = [tflag, &layer, &info, &size](const void* data) {
			glTexImage2D(tflag, layer++, info->type, size.width, size.height, 0, info->toBase, info->toType, data);
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
		int pixelsize = info->numType * GLFormat::QuerySize(info->toBase);
		SPSurface sfc = Surface::Create(buff, pixelsize*size.width, size.width, size.height, info->toSDLFormat);
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
				_size = Texture_StaticURI::LoadTexture(*this, mgr_rw.fromURI(_uri->getPacked(i & mask), RWops::Read, true), static_cast<CubeFace>(i));
		}
	}

	// ------------------------- Texture_Debug -------------------------
	Texture_Debug::Texture_Debug(ITDGen* gen, const spn::Size& size, bool bCube): IGLTexture(gen->getFormat(), size, bCube), _gen(gen) {}
	void Texture_Debug::onDeviceReset() {
		if(_onDeviceReset()) {
			GLenum fmt = getFormat();
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
			u->end();
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
	GLenum TDChecker::getFormat() const { return GL_RGBA8; }
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
	TDCChecker::TDCChecker(int nDivW, int nDivH): _nDivW(nDivW), _nDivH(nDivH) {}
	GLenum TDCChecker::getFormat() const { return GL_RGBA8; }
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
	GLenum TDBorder::getFormat() const { return GL_RGBA8; }
	bool TDBorder::isSingle() const { return true; }
	spn::ByteBuff TDBorder::generate(const spn::Size& size, CubeFace face) const {
		Assert(Trap, false, "not implemented yet") throw 0;
	}
}
