#include "glresource.hpp"
#include "glx.hpp"

namespace rs {
	GLRes::GLRes() {
		_bInit = false;
		_upFb.reset(new GLFBuffer());

		// EmptyTexture = 1x1の単色テクスチャ
		_hlEmptyTex.reset(new HLTex(createTexture(spn::Size(1,1), GL_RGBA8, true)));
		uint32_t buff1 = 0xffffffff;
		auto* t = reinterpret_cast<TexEmpty*>(_hlEmptyTex->ref().get());
		t->writeData(GL_RGBA8, spn::AB_Byte(&buff1, 1), 1, GL_UNSIGNED_BYTE, true);
	}
	GLRes::~GLRes() {
		onDeviceLost();
	}

	GLRes::LHdl GLRes::_common(const spn::PathStr& key, std::function<UPResource ()> cb) {
		LHdl lh = getFromKey(key);
		if(!lh.valid())
			lh = base_type::acquire(key, cb()).first;
		initHandle(lh);
		return std::move(lh);
	}
	HLTex GLRes::loadTexture(const spn::PathStr& path, bool bCube) {
		LHdl lh = _common(path, [&](){return UPResource(new TexFile(path,bCube));});
		return Cast<UPTexture>(std::move(lh));
	}
	HLTex GLRes::createTexture(const spn::Size& size, GLInSizedFmt fmt, bool bRestore) {
		LHdl lh = base_type::acquire(UPResource(new TexEmpty(size, fmt, bRestore)));
		initHandle(lh);
		return Cast<UPTexture>(std::move(lh));
	}

	HLSh GLRes::makeShader(GLuint flag, const std::string& src) {
		LHdl lh = base_type::acquire(UPResource(new GLShader(flag, src)));
		initHandle(lh);
		return Cast<UPShader>(std::move(lh));
	}
	HLFx GLRes::loadEffect(const spn::PathStr& path) {
		LHdl lh = _common(path, [&](){ return UPResource(new GLEffect(path)); });
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
}
