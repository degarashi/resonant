#include "colview.hpp"
#include "../glresource.hpp"
#include "test.hpp"

#define U_Def(str)			rs::IEffect::GlxId::GenUnifId(str)
#define T_Def(tstr, pstr)	rs::IEffect::GlxId::GenTechId(tstr, pstr)
const rs::IdValue
	T_ColFill		= T_Def("Collision", "Fill"),
	T_ColLine		= T_Def("Collision", "Line");

// --------------- ColBox ---------------
rs::HLVb ColBox::MakeVertex() {
	rs::HLVb hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
	const vertex::colview vtx[] = {
		{{-1,-1,-1}}, {{1,-1,-1}},
		{{-1,1,-1}},  {{1,1,-1}},
		{{-1,-1,1}}, {{1,-1,1}},
		{{-1,1,1}},  {{1,1,1}}
	};
	hlVb->get()->initData(vtx, countof(vtx), sizeof(vtx[0]));
	return hlVb;
}
rs::HLIb ColBox::MakeIndex() {
	rs::HLIb hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
	const GLubyte c_index[] = {
		0,2,3, 3,1,0,
		4,6,2, 2,0,4,
		1,3,7, 7,5,1,
		4,5,7, 7,6,4,
		3,2,6, 6,7,3,
		0,1,5, 5,4,0
	};
	hlIb->get()->initData(c_index, countof(c_index));
	return hlIb;
}
void ColBox::setAlpha(float a) {
	_alpha = a;
}
void ColBox::setColor(const spn::Vec3& c) {
	_color = c;
}
ColBox::ColBox() {
	_color = spn::Vec3(1.f);
	_alpha = 1.f;
}

#include "../sys_uniform_value.hpp"
#include "engine.hpp"
void ColBox::draw(rs::IEffect& e) const {
	auto& en = static_cast<Engine&>(e);
	const auto fn = [&en, this](auto tp){
		en.setTechPassId(tp);
		en.setVDecl(rs::DrawDecl<vdecl::colview>::GetVDecl());
		en.setVStream(getVertex(), 0);
		auto ib = getIndex();
		en.setIStream(ib);
		en.setUniform(rs::unif::Color, _color.asVec4(_alpha));
		en.ref3D().setWorld(getToWorld().convertA44());
		en.drawIndexed(GL_TRIANGLES, ib->get()->getNElem());
	};
	fn(T_ColFill);
	fn(T_ColLine);
}

// --------------- ColBoxObj ---------------
struct ColBoxObj::St_Default : StateT<St_Default> {
	void onDraw(const ColBoxObj& self, rs::IEffect& e) const override {
		self.draw(e);
	}
};
ColBoxObj::ColBoxObj() {
	setStateNew<St_Default>();
}
const rs::SPVDecl& rs::DrawDecl<vdecl::colview>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION}
	});
	return vd;
}
