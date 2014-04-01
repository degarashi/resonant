#include "glresource.hpp"
#include "glx.hpp"
#include "adaptsdl.hpp"

namespace rs {
	GLRes::GLRes() {
		_bInit = false;
		_bInDtor = false;
		_upFb.reset(new GLFBuffer());

		// EmptyTexture = 1x1の単色テクスチャ
		uint32_t buff1 = 0xffffffff;
		_hlEmptyTex.reset(new HLTex(createTexture(spn::Size(1,1), GL_RGBA, false, true, GL_UNSIGNED_BYTE, spn::AB_Byte(&buff1, 1))));
	}
	GLRes::~GLRes() {
		// 破棄フラグを立ててからOnDeviceLost関数を呼ぶ
		_bInDtor = true;
		onDeviceLost();
	}
	GLRes::LHdl GLRes::_common(const std::string& key, std::function<UPResource ()> cb) {
		LHdl lh = getFromKey(key);
		if(!lh.valid())
			lh = base_type::acquire(key, cb()).first;
		initHandle(lh);
		return std::move(lh);
	}
	HLTex GLRes::loadTexture(const spn::URI& uri, OPInCompressedFmt fmt) {
		LHdl lh = _common(uri.plainUri_utf8(), [&](){return UPResource(new Texture_StaticURI(uri, fmt));});
		return Cast<UPTexture>(std::move(lh));
	}
	HLTex GLRes::loadCubeTexture(const spn::URI& uri, OPInCompressedFmt fmt) {
		LHdl lh = _common(uri.plainUri_utf8(), [&](){return UPResource(new Texture_StaticCubeURI(uri, fmt));});
		return Cast<UPTexture>(std::move(lh));
	}
	HLTex GLRes::loadCubeTexture(const spn::URI& uri0, const spn::URI& uri1, const spn::URI& uri2,
								  const spn::URI& uri3, const spn::URI& uri4, const spn::URI& uri5, OPInCompressedFmt fmt)
	{
		std::string tmp(uri0.plainUri_utf8());
		auto fn = [&tmp](const spn::URI& u) { tmp.append(u.plainUri_utf8()); };
		fn(uri1); fn(uri2); fn(uri3); fn(uri4); fn(uri5);

		LHdl lh = _common(tmp, [&](){return UPResource(new Texture_StaticCubeURI(uri0,uri1,uri2,uri3,uri4,uri5,fmt));});
		return Cast<UPTexture>(std::move(lh));
	}
	HLTex GLRes::createTexture(const spn::Size& size, GLInSizedFmt fmt, bool bStream, bool bRestore) {
		LHdl lh = base_type::acquire(UPResource(new Texture_Mem(false, fmt, size, bStream, bRestore)));
		initHandle(lh);
		return Cast<UPTexture>(std::move(lh));
	}
	HLTex GLRes::createTexture(const spn::Size& size, GLInSizedFmt fmt, bool bStream, bool bRestore, GLTypeFmt srcFmt, spn::AB_Byte data) {
		LHdl lh = base_type::acquire(UPResource(new Texture_Mem(false, fmt, size, bStream, bRestore, srcFmt, data)));
		initHandle(lh);
		return Cast<UPTexture>(std::move(lh));
	}
	HLSh GLRes::makeShader(GLuint flag, const std::string& src) {
		LHdl lh = base_type::acquire(UPResource(new GLShader(flag, src)));
		initHandle(lh);
		return Cast<UPShader>(std::move(lh));
	}
	HLFx GLRes::loadEffect(const spn::URI& uri) {
		HLRW hlRW = mgr_rw.fromURI(uri, RWops::Read);
		AdaptSDL as(hlRW.get());
		LHdl lh = _common(uri.plainUri_utf8(), [&](){ return UPResource(new GLEffect(as)); });
		return Cast<UPEffect>(std::move(lh));
	}
	HLVb GLRes::makeVBuffer(GLuint dtype) {
		LHdl lh = base_type::acquire(UPResource(new GLVBuffer(dtype)));
		initHandle(lh);
		return Cast<UPVBuffer>(std::move(lh));
	}
	HLIb GLRes::makeIBuffer(GLuint dtype) {
		LHdl lh = base_type::acquire(UPResource(new GLIBuffer(dtype)));
		initHandle(lh);
		return Cast<UPIBuffer>(std::move(lh));
	}

	HLProg GLRes::makeProgram(HSh vsh, HSh psh) {
		LHdl lh = base_type::acquire(UPResource(new GLProgram(vsh,psh)));
		initHandle(lh);
		return Cast<UPProg>(std::move(lh));
	}
	HLProg GLRes::makeProgram(HSh vsh, HSh gsh, HSh psh) {
		LHdl lh = base_type::acquire(UPResource(new GLProgram(vsh,gsh,psh)));
		initHandle(lh);
		return Cast<UPProg>(std::move(lh));
	}
	GLFBufferTmp& GLRes::getTmpFramebuffer() const {
		return *_tmpFb;
	}
	HTex GLRes::getEmptyTexture() const {
		return _hlEmptyTex->get();
	}
	bool GLRes::deviceStatus() const {
		return _bInit;
	}
	bool GLRes::isInDtor() const {
		return _bInDtor;
	}
	void GLRes::onDeviceLost() {
		if(_bInit) {
			_bInit = false;
			for(auto& r : *this)
				r->onDeviceLost();
			_tmpFb.reset(nullptr);
			_upFb->onDeviceLost();
		}
	}
	void GLRes::onDeviceReset() {
		if(!_bInit) {
			_bInit = true;
			_upFb->onDeviceReset();
			_tmpFb.reset(new GLFBufferTmp(_upFb->getBufferID()));
			for(auto& r : *this)
				r->onDeviceReset();
		}
	}
	spn::LHandle GLRes::loadResource(spn::AdaptStream& ast, const spn::URI& uri) {
		auto ext = uri.getExtension();
		spn::LHandle ret;
		// is it Texture?
		if(ext=="png" || ext=="jpg" || ext=="bmp")
			ret = loadTexture(uri);
		// is it Effect(Shader)?
		else if(ext == "glx")
			ret = loadEffect(uri);
		return std::move(ret);
	}
}
