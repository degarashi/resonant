#include "engine.hpp"
#include "../systeminfo.hpp"
#include "../camera.hpp"

using GlxId = rs::IEffect::GlxId;
namespace myunif {
	const rs::IdValue U_Position = GlxId::GenUnifId("u_lightPos"),
						U_Color = GlxId::GenUnifId("u_lightColor"),
						U_Dir =	GlxId::GenUnifId("u_lightDir"),
						U_Mat = GlxId::GenUnifId("u_lightMat"),
						U_Coeff = GlxId::GenUnifId("u_lightCoeff"),
						U_Depth = GlxId::GenUnifId("u_texLightDepth"),
						U_CubeDepth = GlxId::GenUnifId("u_texCubeDepth"),
						U_LightIVP = GlxId::GenUnifId("u_lightViewProj"),
						U_TexZPass = GlxId::GenUnifId("u_texZPass"),
						U_TexLAccum = GlxId::GenUnifId("u_texLAccum"),
						U_DepthRange = GlxId::GenUnifId("u_depthRange"),
						U_LineLength = GlxId::GenUnifId("u_lineLength"),
						U_ScrLightPos = GlxId::GenUnifId("u_scrLightPos"),
						U_ScrLightDir = GlxId::GenUnifId("u_scrLightDir"),
						U_ScreenSize = GlxId::GenUnifId("u_scrSize");
}
const rs::IdValue T_ZPass = GlxId::GenTechId("DeferredLight", "ZPass"),
					T_LAccum  = GlxId::GenTechId("DeferredLight", "LightAccum"),
					T_Shading = GlxId::GenTechId("DeferredLight", "Shading"),
					T_LAccumS = GlxId::GenTechId("DeferredLight", "LightAccumShadow");
DefineDrawGroup(MyDrawGroup)
ImplDrawGroup(MyDrawGroup, 0x0000)
spn::SizeF Engine::Getter::operator()(const spn::SizeF& s, ScreenSize*, const Engine&) const {
	auto& sr = const_cast<spn::SizeF&>(s);
	sr = mgr_info.getScreenSize();
	return sr;
}
uint32_t Engine::GetterC::operator()(const spn::none_t&, D_Camera*, const Engine& e) const {
	return e.ref3D().getCamera()->getAccum();
}
Engine::Engine(const std::string& name):
	rs::util::GLEffect_2D3D(name),
	_gauss(0),
	_drawType(DrawType::Normal)
{
	_hlDg = rs_mgr_obj.makeDrawGroup<MyDrawGroup>(rs::DSortV{rs::cs_dsort_priority_asc}, true). first;
	setLightPosition(spn::Vec3(0,0,0));
	setLightDir(spn::Vec3(0,0,1));
	setDepthRange(spn::Vec2(0, 10));
	setLightDepthSize(spn::Size{256,256});
	setLineLength(1.f);
	setLightColor(spn::Vec3(1,1,1));
}
Engine::DrawType::E Engine::getDrawType() const {
	return _drawType;
}
void Engine::_prepareUniforms() {
	rs::util::GLEffect_2D3D::_prepareUniforms();

	#define DEF_SETUNIF(name, func) \
		if(auto idv = getUnifId(myunif::name)) \
			setUniform(*idv, func(), true);
	DEF_SETUNIF(U_Position, getLightPosition)
	DEF_SETUNIF(U_Color, getLightColor)
	DEF_SETUNIF(U_Dir, getLightDir)
	DEF_SETUNIF(U_DepthRange, getDepthRange)
	DEF_SETUNIF(U_CubeDepth, getCubeColorBuff)
	DEF_SETUNIF(U_Depth, getLightColorBuff)
	DEF_SETUNIF(U_Mat, getLightMatrix)

	DEF_SETUNIF(U_LineLength , getLineLength)
	DEF_SETUNIF(U_TexLAccum, getLightAccumBuff)
	DEF_SETUNIF(U_TexZPass, getZPrePassBuff)
	DEF_SETUNIF(U_Coeff, getLightCoeff)
	DEF_SETUNIF(U_LightIVP, getLightIVP)
	#undef DEF_SETUNIF
	{
		auto& scl = getLightScLit();
		if(auto idv = getUnifId(myunif::U_ScrLightPos))
			setUniform(*idv, scl.pos, true);
		if(auto idv = getUnifId(myunif::U_ScreenSize))
			setUniform(*idv, scl.size, true);
		if(auto idv = getUnifId(myunif::U_ScrLightDir))
			setUniform(*idv, scl.dir, true);
	}
}
#include "../camera.hpp"
spn::RFlagRet Engine::_refresh(rs::HLRb& rb, LightDepth*) const {
	auto& sz = getLightDepthSize();
	rb = mgr_gl.makeRBuffer(sz.width, sz.height, GL_DEPTH_COMPONENT16);
	return {true, 0};
}
spn::RFlagRet Engine::_refresh(typename ScreenAspect::value_type& sc, ScreenAspect*) const {
	auto ret = _rflag.getWithCheck(this, sc);
	bool b = ret.second;
	if(b) {
		auto sz = getScreenSize();
		sc = sz.width / sz.height;
	}
	return {b, 0};
}
spn::RFlagRet Engine::_refresh(rs::HLTex& tex, CubeColorBuff*) const {
	tex = mgr_gl.createCubeTexture(getLightDepthSize(), GL_RG16, false, false);
	tex->get()->setFilter(true, true);
	return {true, 0};
}
spn::RFlagRet Engine::_refresh(typename ZPrePassBuff::value_type& tex, ZPrePassBuff*) const {
	auto ret = _rflag.getWithCheck(this, tex);
	bool b = ret.second;
	if(b) {
		tex = mgr_gl.createTexture(*std::get<0>(ret.first), GL_RGBA16F, false, false);
		tex->get()->setFilter(true, true);
	}
	return {b, 0};
}
spn::RFlagRet Engine::_refresh(typename LightAccumBuff::value_type& tex, LightAccumBuff*) const {
	auto ret = _rflag.getWithCheck(this, tex);
	bool b = ret.second;
	if(b) {
		tex = mgr_gl.createTexture(*std::get<0>(ret.first), GL_RGB16F, false, false);
		tex->get()->setFilter(true, true);
	}
	return {b, 0};
}
spn::RFlagRet Engine::_refresh(typename LightScLit::value_type& sclit, LightScLit*) const {
	auto ret = _rflag.getWithCheck(this, sclit);
	bool b = ret.second;
	if(b) {
		auto& cam = ref3D().getCamera().cref();
		auto& mView = cam.getView();
		sclit.pos = std::get<0>(ret.first)->asVec4(1) * mView;
		sclit.dir = std::get<1>(ret.first)->asVec4(0) * mView;
		float t = std::tan(cam.getFov().get()/2);
		sclit.size = spn::Vec2(t * cam.getAspect(), t);
	}
	return {b, 0};
}
spn::RFlagRet Engine::_refresh(typename LightIVP::value_type& ivp, LightIVP*) const {
	auto ret = _rflag.getWithCheck(this, ivp);
	bool b = ret.second;
	if(b) {
		// Invert(View) * Light(ViewProj)
		auto& cam = ref3D().getCamera().cref();
		auto mView = cam.getView().convertA44();
		mView.invert();
		ivp = mView * *std::get<1>(ret.first);
	}
	return {b, 0};
}
spn::RFlagRet Engine::_refresh(rs::HLCam& c, LightCamera*) const {
	c = mgr_cam.emplace();
	auto& pose = c->refPose();
	pose.setOffset(getLightPosition());
	spn::AQuat q;
	if(getLightDir().z < -1.f+1e-4f)
		q = spn::AQuat::RotationY(spn::DegF(180.f));
	else
		q = spn::AQuat::Rotation({0,0,1}, getLightDir());
	pose.setRot(q);
	c->setFov(spn::DegF(90));
	c->setZPlane(0.01f, 500.f);
	return {true, 0};
}
spn::RFlagRet Engine::_refresh(spn::Mat44& m, LightMatrix*) const {
	auto& c = getLightCamera().cref();
	m = c.getViewProj();
	return {true, 0};
}
spn::RFlagRet Engine::_refresh(rs::HLTex& tex, LightColorBuff*) const {
	tex = mgr_gl.createTexture(getLightDepthSize(), GL_RG16, false, false);
	tex->get()->setFilter(true, true);
	return {true, 0};
}
spn::RFlagRet Engine::_refresh(rs::HLFb& fb, LightFB*) const {
	if(!fb)
		fb = mgr_gl.makeFBuffer();
	fb->get()->attachTexture(rs::GLFBuffer::Att::COLOR0, getLightColorBuff());
	fb->get()->attachRBuffer(rs::GLFBuffer::Att::DEPTH, getLightDepth());
	return {true, 0};
}
#include "../util/screen.hpp"
class Engine::DLScene : public rs::DrawableObjT<DLScene> {
	private:
		struct St_Default : StateT<St_Default> {
			void onDraw(const DLScene&, rs::IEffect& e) const override {
				rs::HLFb fb0 = e.getFramebuffer();
				auto& engine = static_cast<Engine&>(e);
				rs::HLCam cam = engine.ref3D().getCamera();
				cam->setAspect(engine.getScreenAspect());
				rs::HLFb fb = mgr_gl.makeFBuffer();
				using Att = rs::GLFBuffer::Att;
				// Z-PrePass
				// アウトプット先深度バッファを流用
				Assert(Trap, engine._hlFb)
				fb->get()->attachOther(Att::DEPTH, Att::DEPTH, engine._hlFb);
				fb->get()->attachTexture(Att::COLOR0, engine.getZPrePassBuff());
				e.setFramebuffer(fb);
				e.clearFramebuffer(rs::draw::ClearParam{spn::Vec4(0), 1.f, spn::none});
				engine._drawType = DrawType::DL_ZPrePass;
				engine._hlDg->get()->onDraw(e);

				for(int i=0 ; i<1 ; i++) {
					{	// LightDepth
						rs::HLFb fbDepth = mgr_gl.makeFBuffer();
						fbDepth->get()->attachRBuffer(Att::DEPTH, engine.getLightDepth());
						fbDepth->get()->attachTexture(Att::COLOR0, engine.getLightColorBuff());
						e.setFramebuffer(fbDepth);
						e.clearFramebuffer(rs::draw::ClearParam{spn::none, 1.f, spn::none});
						engine._drawType = DrawType::DL_Depth;
						// ライトの位置にカメラをセット
						engine.ref3D().setCamera(engine.getLightCamera());
						engine._hlDg->get()->onDraw(e);
					}
					{	// LightAccumulation
						fb->get()->attachTexture(Att::COLOR0,
								engine.getLightAccumBuff());
						e.setFramebuffer(fb);
						if(i==0)
							e.clearFramebuffer(rs::draw::ClearParam{spn::Vec4(0.1f), spn::none, spn::none});
						engine.ref3D().setCamera(cam);
						e.setTechPassId(T_LAccumS);
						rs::util::ScreenRect sr;
						sr.draw(e);
					}
				}

				// Shading
				e.setFramebuffer(engine._hlFb);
				e.clearFramebuffer(rs::draw::ClearParam{spn::Vec4{0,0,0.5f,1}, spn::none, spn::none});
				engine._drawType = DrawType::DL_Shade;
				engine.ref3D().setCamera(cam);
				engine._hlDg->get()->onDraw(e);

				e.setFramebuffer(fb0);
			}
		};
	public:
		DLScene() {
			setStateNew<St_Default>();
		}
};
class Engine::DrawScene : public rs::DrawableObjT<DrawScene> {
	private:
		struct St_Default : StateT<St_Default> {
			void onDraw(const DrawScene&, rs::IEffect& e) const override {
				rs::HLFb fb = e.getFramebuffer();

				auto& engine = static_cast<Engine&>(e);
				e.setFramebuffer(engine.getLightFB());
				e.clearFramebuffer(rs::draw::ClearParam{spn::Vec4(0), 1.f, spn::none});
				engine._drawType = DrawType::Depth;
				// カメラをライト位置へ
				rs::HLCam cam = engine.ref3D().getCamera();
				engine.ref3D().setCamera(engine.getLightCamera());
				engine._hlDg->get()->onDraw(e);

				// ライト深度にブラーをかける
				engine._gauss.setSource(engine.getLightColorBuff());
				engine._gauss.setDest(engine.getLightColorBuff());
				engine._gauss.onDraw(e);

				e.setFramebuffer(engine._hlFb);
				e.clearFramebuffer(rs::draw::ClearParam{spn::Vec4{0,0,0,1}, 1.f, spn::none});
				// カメラ位置を元に戻す
				engine.ref3D().setCamera(cam);
				engine._drawType = DrawType::Normal;
				engine._hlDg->get()->onDraw(e);

				e.setFramebuffer(fb);
			}
		};
	public:
		DrawScene() {
			setStateNew<St_Default>();
		}
};
namespace {
	const spn::DegF c_deg90(90);
	const spn::AQuat c_cubedir[6] = {
		spn::AQuat::RotationY(c_deg90),			// NegativeX
		spn::AQuat::RotationY(-c_deg90),		// PositiveX
		spn::AQuat::RotationX(c_deg90),			// PositiveY
		spn::AQuat::RotationX(-c_deg90),		// NegativeY
		spn::AQuat::RotationY(spn::DegF(0)),	// PositiveZ
		spn::AQuat::RotationY(c_deg90*2),		// NegativeZ
	};
}
class Engine::CubeScene : public rs::DrawableObjT<CubeScene> {
	private:
		struct St_Default : StateT<St_Default> {
			mutable int count = 0;
			void onDraw(const CubeScene&, rs::IEffect& e) const override {
				rs::HLFb fb0 = e.getFramebuffer();
				auto& engine = static_cast<Engine&>(e);

				rs::HLCam cam = engine.ref3D().getCamera();
				auto hlC = mgr_cam.emplace();
				hlC->setFov(spn::DegF(90));
				hlC->setAspect(1.f);
				hlC->setZPlane(0.01f, 100.0f);
				engine.ref3D().setCamera(hlC);

				rs::HLFb fb = mgr_gl.makeFBuffer();
				fb->get()->attachRBuffer(rs::GLFBuffer::Att::DEPTH,
										engine.getLightDepth());
				auto& pose = hlC->refPose();
				pose.setOffset(engine.getLightPosition());
				engine._drawType = DrawType::CubeDepth;
				for(int i=0 ; i<6 ; i++) {
					fb->get()->attachTextureFace(rs::GLFBuffer::Att::COLOR0,
												engine.getCubeColorBuff(),
												static_cast<rs::CubeFace>(i));
					e.setFramebuffer(fb);
					e.clearFramebuffer(rs::draw::ClearParam{spn::Vec4(0), 1.f, spn::none});
					// カメラの方向をキューブの各面へ
					pose.setRot(c_cubedir[i]);
					// 深度描画
					engine._hlDg->get()->onDraw(e);
				}

				e.setFramebuffer(engine._hlFb);
				e.clearFramebuffer(rs::draw::ClearParam{spn::Vec4{0,0,1,0}, 1.f, spn::none});
				// カメラ位置を元に戻して通常描画
				engine.ref3D().setCamera(cam);
				engine._drawType = DrawType::CubeNormal;
				engine._hlDg->get()->onDraw(e);

				e.setFramebuffer(fb0);
			}
		};
	public:
		CubeScene() {
			setStateNew<St_Default>();
		}
};
Engine::~Engine() {
	if(!rs::ObjMgr::Initialized())
		_hlDg.setNull();
}
rs::HLDObj Engine::getDLScene(rs::Priority dprio) const {
	auto ret = rs_mgr_obj.makeDrawable<DLScene>();
	ret.second->setDrawPriority(dprio);
	return ret.first;
}
rs::HLDObj Engine::getDrawScene(rs::Priority dprio) const {
	auto ret = rs_mgr_obj.makeDrawable<DrawScene>();
	ret.second->setDrawPriority(dprio);
	return ret.first;
}
rs::HLDObj Engine::getCubeScene(rs::Priority dprio) const {
	auto ret = rs_mgr_obj.makeDrawable<CubeScene>();
	ret.second->setDrawPriority(dprio);
	return ret.first;
}
void Engine::addSceneObject(rs::HDObj hdObj) {
	_hlDg->get()->addObj(hdObj);
}
void Engine::remSceneObject(rs::HDObj hdObj) {
	_hlDg->get()->remObj(hdObj);
}
void Engine::clearScene() {
	_hlDg->get()->clear();
}
void Engine::setDispersion(float d) {
	_gauss.setDispersion(d);
}
void Engine::setOutputFramebuffer(rs::HFb hFb) {
	_hlFb = hFb;
}
void Engine::moveFrom(rs::IEffect& e) {
	GLEffect_2D3D::moveFrom(e);
	auto& pe = static_cast<Engine&>(e);
	_hlFb = std::move(pe._hlFb);
	_hlDg = std::move(pe._hlDg);
	_drawType = pe._drawType;
	_gauss = std::move(pe._gauss);
	_rflag = std::move(pe._rflag);
}
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_PTR_NOCTOR(Engine, Engine,
	NOTHING,
	(setLineLength<float>)
	(setLightPosition<const spn::Vec3&>)
	(setLightColor<const spn::Vec3&>)
	(setLightDir<const spn::Vec3&>)
	(setLightDepthSize<const spn::Size&>)
	(setDepthRange<const spn::Vec2&>)
	(setDispersion)
	(getDLScene)
	(getDrawScene)
	(getCubeScene)
	(addSceneObject)
	(remSceneObject)
	(setOutputFramebuffer)
	(clearScene))
