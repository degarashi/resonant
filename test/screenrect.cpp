#include "test.hpp"
#include "screenrect.hpp"
#include "engine.hpp"

rs::WVb ScreenRect::s_wVb;
rs::WIb ScreenRect::s_wIb;
// ---------------------- ScreenRect ----------------------
ScreenRect::ScreenRect() {
	if(auto h = s_wVb.lock())
		_hlVb = h;
	else {
		_hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
		const vertex::sprite c_vertex[] = {
			{{-1,-1}, {0,0}},
			{{-1,1}, {0,1}},
			{{1,1}, {1,1}},
			{{1,-1}, {1,0}}
		};
		_hlVb->get()->initData(c_vertex, countof(c_vertex), sizeof(c_vertex[0]));
		s_wVb = _hlVb.weak();
	}
	if(auto h = s_wIb.lock())
		_hlIb = h;
	else {
		const GLubyte c_index[] = {
			0,1,2, 2,3,0
		};
		_hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
		_hlIb->get()->initData(c_index, countof(c_index));
		s_wIb = _hlIb.weak();
	}
}
void ScreenRect::exportDrawTag(rs::DrawTag& tag) const {
	tag.idIBuffer = _hlIb;
	tag.idVBuffer[0] = _hlVb;
}
void ScreenRect::draw(rs::GLEffect& e) const {
	e.setVDecl(rs::DrawDecl<drawtag::sprite>::GetVDecl());
	e.setVStream(_hlVb, 0);
	e.setIStream(_hlIb);
	e.drawIndexed(GL_TRIANGLES, _hlIb->get()->getNElem());
}
