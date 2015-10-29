#include "glresource.hpp"
#include "glx.hpp"
#include "adaptsdl.hpp"
#include "systeminfo.hpp"

namespace rs {
	const std::string& IGLResource::getResourceName() const {
		static std::string str("IGLResource");
		return str;
	}

	namespace {
		template <class Tex_t, class... Ts>
		auto MakeStaticTex(Ts&&... ts) {
			return [&](const spn::URI& uri){
				return UPResource(new Tex_t(uri, std::forward<Ts>(ts)...));
			};
		}
	}

	std::shared_ptr<FxBlock> InitFxBlock() {
		return std::make_shared<FxBlock>();
	}
	const std::string GLRes::cs_rtname[] = {
		"texture",
		"effect"
	};
	GLRes::GLRes(): ResMgrApp(cs_rtname) {
		_bInit = false;
		_bInDtor = false;
		_upFb.reset(new GLFBuffer());
		// 既にデバイスがアクティブだったらonDeviceResetを呼ぶ
		_cbInit =
			[&](auto h){
				if(_bInit)
					h.ref()->onDeviceReset();
			};

		// EmptyTexture = 1x1の単色テクスチャ
		uint32_t buff1 = 0xffffffff;
		_hlEmptyTex.reset(new HLTex(createTextureInit(spn::Size(1,1), GL_RGBA, false, true, GL_UNSIGNED_BYTE, spn::AB_Byte(&buff1, 1))));
	}
	GLRes::~GLRes() {
		// 破棄フラグを立ててからOnDeviceLost関数を呼ぶ
		_bInDtor = true;
		onDeviceLost();
	}
	const std::string& GLRes::getResourceName(spn::SHandle sh) const {
		if(sh) {
			IGLResource* gr = HRes::FromHandle(sh)->get();
			return gr->getResourceName();
		}
		const static std::string name("IGLResource");
		return name;
	}
	spn::URI GLRes::_modifyResourceName(spn::URI& key) const {
		spn::URI key_k = base_type::_modifyResourceName(key);
		// Cubeプリフィックスを持っている時は末尾に加える
		if(_chPostfix) {
			auto str = key_k.getLast_utf8();
			str += *_chPostfix;
			key_k.popBack();
			key_k.pushBack(str);
		}
		return key_k;
	}
	HLTex GLRes::loadTexture(const std::string& name, MipState miplevel, OPInCompressedFmt fmt) {
		_setResourceTypeId(ResourceType::Texture);
		return loadTextureUri(_uriFromResourceName(name), miplevel, fmt);
	}
	HLTex GLRes::loadTextureUri(const spn::URI& uri, MipState miplevel, OPInCompressedFmt fmt) {
		_chPostfix = spn::none;
		return Cast<UPTexture>(
			loadResourceApp(uri,
				MakeStaticTex<Texture_StaticURI>(miplevel, fmt),
				_cbInit)
		);
	}
	HLTex GLRes::loadCubeTexture(const std::string& name, MipState miplevel, OPInCompressedFmt fmt) {
		_setResourceTypeId(ResourceType::Texture);
		// 0を付加してリソース検索
		spn::PathBlock pb(name);
		pb.setPathNum([](auto /*num*/) -> spn::Int_OP{ return 0; });

		pb = _uriFromResourceName(pb.plain_utf8()).path();
		// 末尾の0を除く
		pb.setPathNum([](auto) ->spn::Int_OP{ return spn::none; });
		return loadCubeTextureUri(spn::URI("file", pb), miplevel, fmt);
	}
	HLTex GLRes::loadCubeTextureUri(const spn::URI& uri, MipState miplevel, OPInCompressedFmt fmt) {
		// 連番CubeTexutreの場合はキーとなるURIの末尾に"@"を付加する
		_chPostfix = '@';
		// Uriの連番展開
		return Cast<UPTexture>(
					loadResourceApp(uri,
						MakeStaticTex<Texture_StaticCubeURI>(miplevel, fmt),
						_cbInit)
				);
	}
	// 連番キューブ: Key=(Path+@, ext) URI=(Path, ext)
	HLTex GLRes::_loadCubeTexture(MipState miplevel, OPInCompressedFmt fmt, const spn::URI& uri0, const spn::URI& uri1, const spn::URI& uri2,
								  const spn::URI& uri3, const spn::URI& uri4, const spn::URI& uri5)
	{
		_chPostfix = spn::none;
		// 個別指定CubeTextureの場合はリソース名はUriを全部つなげた文字列とする
		std::string tmp(uri0.plainUri_utf8());
		auto fn = [&tmp](const spn::URI& u) { tmp.append(u.plainUri_utf8()); };
		fn(uri1); fn(uri2); fn(uri3); fn(uri4); fn(uri5);

		return Cast<UPTexture>(
			loadResourceApp(spn::URI("file", tmp),
				[&](const spn::URI&){ return UPResource(new Texture_StaticCubeURI(uri0,uri1,uri2,uri3,uri4,uri5,miplevel,fmt)); },
				_cbInit)
		);
	}
	HLTex GLRes::createTexture(const spn::Size& size, GLInSizedFmt fmt, bool bStream, bool bRestore) {
		LHdl lh = base_type::acquire(UPResource(new Texture_Mem(false, fmt, size, bStream, bRestore)));
		_cbInit(lh);
		return Cast<UPTexture>(std::move(lh));
	}
	HLTex GLRes::createTextureInit(const spn::Size& size, GLInSizedFmt fmt, bool bStream, bool bRestore, GLTypeFmt srcFmt, spn::AB_Byte data) {
		LHdl lh = base_type::acquire(UPResource(new Texture_Mem(false, fmt, size, bStream, bRestore, srcFmt, data)));
		_cbInit(lh);
		return Cast<UPTexture>(std::move(lh));
	}
	HLSh GLRes::makeShader(ShType type, const std::string& src) {
		LHdl lh = base_type::acquire(UPResource(new GLShader(type, src)));
		_cbInit(lh);
		return Cast<UPShader>(std::move(lh));
	}
	HLFx GLRes::loadEffect(const std::string& name, const CBCreateFx& cb) {
		_chPostfix = spn::none;
		_setResourceTypeId(ResourceType::Effect);
		auto ret = this->base_t::acquire(name, [&](const spn::URI& u){
				return UPResource(cb(u.plain_utf8()));
		});
		if(ret.second)
			_cbInit(ret.first);
		return Cast<UPEffect>(ret.first);
	}
	HLVb GLRes::makeVBuffer(GLuint dtype) {
		LHdl lh = base_type::acquire(UPResource(new GLVBuffer(dtype)));
		_cbInit(lh);
		return Cast<UPVBuffer>(std::move(lh));
	}
	HLIb GLRes::makeIBuffer(GLuint dtype) {
		LHdl lh = base_type::acquire(UPResource(new GLIBuffer(dtype)));
		_cbInit(lh);
		return Cast<UPIBuffer>(std::move(lh));
	}

	HLProg GLRes::makeProgram(HSh vsh, HSh psh) {
		LHdl lh = base_type::acquire(UPResource(new GLProgram(vsh,psh)));
		_cbInit(lh);
		return Cast<UPProg>(std::move(lh));
	}
	HLProg GLRes::makeProgram(HSh vsh, HSh gsh, HSh psh) {
		LHdl lh = base_type::acquire(UPResource(new GLProgram(vsh,gsh,psh)));
		_cbInit(lh);
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
			// 一旦DeviceLostしたらフレームバッファはデフォルトに戻る(in Gameloop)
			_tmpFb->use_begin();
			_tmpFb.reset(nullptr);

			_upFb->onDeviceLost();
		}
	}
	void GLRes::onDeviceReset() {
		if(!_bInit) {
			_bInit = true;
			_upFb->onDeviceReset();
			_tmpFb.reset(new GLFBufferTmp(_upFb->getBufferID(), mgr_info.getScreenSize()));
			for(auto& r : *this)
				r->onDeviceReset();
		}
	}
	spn::LHandle GLRes::loadResource(spn::AdaptStream& /*ast*/, const spn::URI& uri) {
		auto ext = uri.getExtension();
		spn::LHandle ret;
		// is it Texture?
		if(ext=="png" || ext=="jpg" || ext=="bmp")
			ret = loadTextureUri(uri);
		// is it Effect(Shader)?
		else if(ext == "glx") {
			HLRW hlRW = mgr_rw.fromURI(uri, RWops::Read);
			ret = loadEffect(uri.plain_utf8(), [](auto& name){
					return new GLEffect(name); });
		}
		return ret;
	}
	HLFb GLRes::makeFBuffer() {
		LHdl lh = base_type::acquire(UPResource(new GLFBuffer()));
		_cbInit(lh);
		return Cast<UPFBuffer>(lh);
	}
	HLRb GLRes::makeRBuffer(int w, int h, GLInRenderFmt fmt) {
		LHdl lh = base_type::acquire(UPResource(new GLRBuffer(w, h, fmt)));
		_cbInit(lh);
		return Cast<UPRBuffer>(lh);
	}

	namespace draw {
		Clear::Clear(const ClearParam& p):
			_param(p)
		{}
		void Clear::exec() {
			GLenum flag = 0;
			if(_param.color) {
				flag |= GL_COLOR_BUFFER_BIT;
				auto& c = *_param.color;
				GL.glClearColor(c.x, c.y, c.z, c.w);
			}
			if(_param.depth) {
				flag |= GL_DEPTH_BUFFER_BIT;
#ifndef USE_OPENGLES2
				GL.glClearDepth
#else
				GL.glClearDepthf
#endif
					(*_param.depth);
			}
			if(_param.stencil) {
				flag |= GL_STENCIL_BUFFER_BIT;
				GL.glClearStencil(*_param.stencil);
			}
			// WindowsではDepthMaskをTrueにしないとクリアされない為、一旦値を退避
			GLboolean b;
			GL.glGetBooleanv(GL_DEPTH_TEST, &b);
			GL.glDepthMask(GL_TRUE);
			GL.glClear(flag);
			// DepthMask値を戻す
			GL.glDepthMask(b);
		}
	}
}
