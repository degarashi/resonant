#include "sprite3d.hpp"
#include "sprite.hpp"
#include "test.hpp"
#include "engine.hpp"
#include "spinner/structure/profiler.hpp"
#include "../camera.hpp"

const rs::IdValue Sprite3D::T_Sprite = GlxId::GenTechId("Sprite", "Default3D");
// ----------------------- Sprite3D -----------------------
Sprite3D::Sprite3D(rs::HTex hTex) {
	_hlTex = hTex;
	_alpha = 1.f;
	std::tie(_hlVb, _hlIb) = Sprite::InitBuffer();
}
void Sprite3D::setAlpha(float a) {
	_alpha = a;
}
void Sprite3D::draw(Engine& e) const {
	auto typ = e.getDrawType();
	using DT = Engine::DrawType;
	switch(typ) {
		case DT::Normal:
		case DT::CubeNormal:
		case DT::DL_Shade:
			{
				spn::profiler.beginBlockObj("sprite3D::draw");
				e.setTechPassId(T_Sprite);
				e.setVDecl(rs::DrawDecl<vdecl::sprite>::GetVDecl());
				e.setUniform(rs::unif3d::texture::Diffuse, _hlTex);
				e.setUniform(rs::unif::Alpha, _alpha);
				// カメラと正対するように回転
				auto cam = e.ref3D().getCamera();
				auto& pose = cam->getPose();
				auto m = spn::Mat44::LookDirLH({0,0,0}, -pose.getDir(), pose.getUp());
				m.invert();
				const auto& sc = getScale();
				m *= spn::Mat44::Scaling(sc.x, sc.y, sc.z, 1);
				m *= spn::Mat44::Translation(getOffset());
				e.ref<rs::SystemUniform3D>().setWorld(m);
				e.setVStream(_hlVb, 0);
				e.setIStream(_hlIb);
				e.drawIndexed(GL_TRIANGLES, 6);
			}
			break;
		default:
			break;
	}
}
void Sprite3D::exportDrawTag(rs::DrawTag& d) const {
	d.idTex[0] = _hlTex;
	d.idVBuffer[0] = _hlVb;
	d.idIBuffer = _hlIb;
	d.zOffset = 0;
}
// ----------------------- PointSprite3D -----------------------
PointSprite3D::PointSprite3D(rs::HTex hTex, const spn::Vec3& pos):
	Sprite3D(hTex)
{
	setOffset(pos);
	setStateNew<St_Default>();
}
struct PointSprite3D::St_Default : StateT<St_Default> {
	void onDraw(const PointSprite3D& self, rs::IEffect& e) const override {
		self.draw(static_cast<Engine&>(e));
	}
};
