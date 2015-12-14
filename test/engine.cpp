#include "engine.hpp"
#include "../systeminfo.hpp"

namespace myunif {
	namespace light {
		using GlxId = rs::IEffect::GlxId;
		const rs::IdValue Position = GlxId::GenUnifId("m_vLightPos"),
							Color = GlxId::GenUnifId("m_vLightColor"),
							Dir =	GlxId::GenUnifId("m_vLightDir"),
							Power =	GlxId::GenUnifId("m_fLightPower"),
							Depth = GlxId::GenUnifId("m_texLightDepth"),
							CubeDepth = GlxId::GenUnifId("m_texCubeDepth"),
							DepthRange = GlxId::GenUnifId("m_depthRange"),
							LightMat = GlxId::GenUnifId("m_mLight"),
							LineLength = GlxId::GenUnifId("m_lineLength");
	}
}
DefineDrawGroup(MyDrawGroup)
ImplDrawGroup(MyDrawGroup, 0x0000)
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
	if(auto idv = getUnifId(myunif::light::CubeDepth))
		setUniform(*idv, getCubeColorBuff(), true);
	if(auto idv = getUnifId(myunif::light::Depth))
		setUniform(*idv, getLightColorBuff(), true);
	if(auto idv = getUnifId(myunif::light::LightMat))
		setUniform(*idv, getLightMatrix(), true);
	if(auto idv = getUnifId(myunif::light::LineLength))
		setUniform(*idv, 1.f, true);
}
#include "../camera.hpp"
spn::RFlagRet Engine::_refresh(rs::HLRb& rb, LightDepth*) const {
	auto& sz = getLightDepthSize();
	rb = mgr_gl.makeRBuffer(sz.width, sz.height, GL_DEPTH_COMPONENT16);
	return {true, 0};
}
spn::RFlagRet Engine::_refresh(rs::HLTex& tex, CubeColorBuff*) const {
	tex = mgr_gl.createCubeTexture(getLightDepthSize(), GL_RG16, false, false);
	tex->get()->setFilter(true, true);
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

				e.setFramebuffer(rs::HFb());
				e.clearFramebuffer(rs::draw::ClearParam{spn::Vec4{0,0,1,0}, 1.f, spn::none});
				// カメラ位置を元に戻して通常描画
				engine.ref3D().setCamera(cam);
				engine._drawType = DrawType::CubeNormal;
				engine._hlDg->get()->onDraw(e);
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
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_PTR_NOCTOR(Engine, Engine,
	NOTHING,
	(setLightPosition<const spn::Vec3&>)
	(setLightColor<const spn::Vec3&>)
	(setLightDir<const spn::Vec3&>)
	(setLightPower<float>)
	(setLightDepthSize<const spn::Size&>)
	(setDepthRange<const spn::Vec2&>)
	(setDispersion)
	(getDrawScene)
	(getCubeScene)
	(addSceneObject)
	(remSceneObject)
	(setOutputFramebuffer)
	(clearScene))
