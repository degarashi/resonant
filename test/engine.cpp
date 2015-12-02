#include "engine.hpp"

namespace myunif {
	namespace light {
		using GlxId = rs::IEffect::GlxId;
		const rs::IdValue Position = GlxId::GenUnifId("m_vLightPos"),
							Color = GlxId::GenUnifId("m_vLightColor"),
							Dir =	GlxId::GenUnifId("m_vLightDir"),
							Power =	GlxId::GenUnifId("m_fLightPower"),
							Depth = GlxId::GenUnifId("m_texLightDepth"),
							DepthRange = GlxId::GenUnifId("m_depthRange"),
							LightMat = GlxId::GenUnifId("m_mLight");
	}
}
DefineDrawGroup(MyDrawGroup)
ImplDrawGroup(MyDrawGroup, 0x0000)
Engine::Engine(const std::string& name):
	rs::util::GLEffect_2D3D(name),
	_drawType(DrawType::Normal)
{
	_hlDg = rs_mgr_obj.makeDrawGroup<MyDrawGroup>(rs::DSortV{rs::cs_dsort_priority_asc}, true). first;
	setLightPosition(spn::Vec3(0,0,0));
	setLightDir(spn::Vec3(0,0,1));
	setDepthRange(spn::Vec2(0, 10));
	setLightDepthSize(spn::Size{256,256});
}
Engine::DrawType::E Engine::getDrawType() const {
	return _drawType;
}
void Engine::_prepareUniforms() {
	rs::util::GLEffect_2D3D::_prepareUniforms();

	#define DEF_SETUNIF(name, func) \
		if(auto idv = getUnifId(myunif::light::name)) \
			setUniform(*idv, func##name(), true);
	DEF_SETUNIF(Position, getLight)
	DEF_SETUNIF(Color, getLight)
	DEF_SETUNIF(Dir, getLight)
	DEF_SETUNIF(Power, getLight)
	DEF_SETUNIF(DepthRange, get)
	#undef DEF_SETUNIF
	if(auto idv = getUnifId(myunif::light::Depth))
		setUniform(*idv, getLightColorBuff(), true);
	if(auto idv = getUnifId(myunif::light::LightMat))
		setUniform(*idv, getLightMatrix(), true);
}
#include "../camera.hpp"
spn::RFlagRet Engine::_refresh(rs::HLTex& tex, LightDepth*) const {
	tex = mgr_gl.createTexture(getLightDepthSize(), GL_R16, false, false);
	return {true, 0};
}
spn::RFlagRet Engine::_refresh(rs::HLCam& c, LightCamera*) const {
	c = mgr_cam.emplace();
	auto& pose = c->refPose();
	pose.setOffset(getLightPosition());
	pose.setRot(spn::AQuat::Rotation({0,0,1}, getLightDir()));
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
	tex = mgr_gl.createTexture(getLightDepthSize(), GL_R16, false, false);
	tex->get()->setFilter(true, true);
	return {true, 0};
}
spn::RFlagRet Engine::_refresh(rs::HLFb& fb, LightFB*) const {
	if(!fb)
		fb = mgr_gl.makeFBuffer();
	fb->get()->attachTexture(rs::GLFBuffer::Att::COLOR0, getLightColorBuff());
	fb->get()->attachTexture(rs::GLFBuffer::Att::DEPTH, getLightDepth());
	return {true, 0};
}
#include "../util/screen.hpp"
class Engine::DrawScene : public rs::DrawableObjT<DrawScene> {
	private:
		struct St_Default : StateT<St_Default> {
			void onDraw(const DrawScene&, rs::IEffect& e) const override {
				auto& engine = static_cast<Engine&>(e);
				e.setFramebuffer(engine.getLightFB());
				e.clearFramebuffer(rs::draw::ClearParam{spn::Vec4(0), 1.f, spn::none});
				engine._drawType = DrawType::Depth;
				// カメラをライト位置へ
				rs::HLCam cam = engine.ref3D().getCamera();
				engine.ref3D().setCamera(engine.getLightCamera());
				engine._hlDg->get()->onDraw(e);

				e.setFramebuffer(rs::HFb());
				e.clearFramebuffer(rs::draw::ClearParam{spn::Vec4{0,0,1,0}, 1.f, spn::none});
				// カメラ位置を元に戻す
				engine.ref3D().setCamera(cam);
				engine._drawType = DrawType::Normal;
				engine._hlDg->get()->onDraw(e);
			}
		};
	public:
		DrawScene() {
			setStateNew<St_Default>();
		}
};
Engine::~Engine() {
	if(!rs::ObjMgr::Initialized())
		_hlDg.setNull();
}
rs::HLDObj Engine::getDrawScene(rs::Priority dprio) const {
	auto ret = rs_mgr_obj.makeDrawable<DrawScene>();
	ret.second->setDrawPriority(dprio);
	return ret.first;
}
void Engine::addSceneObject(rs::HDObj hdObj) {
	_hlDg->get()->addObj(hdObj);
}
void Engine::remSceneObject(rs::HDObj hdObj) {
	_hlDg->get()->remObj(hdObj);
}
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_PTR_NOCTOR(Engine, Engine,
	NOTHING,
	(setLightPosition<const spn::Vec3&>)
	(setLightColor<const spn::Vec3&>)
	(setLightDir<const spn::Vec3&>)
	(setLightPower<float>)
	(setLightDepthSize<const spn::Size&>)
	(setDepthRange<const spn::Vec2&>)
	(getDrawScene)
	(addSceneObject)
	(remSceneObject))
