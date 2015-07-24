#include "screenrect.hpp"
#include "../glx.hpp"
#include "../drawtag.hpp"

namespace rs {
	namespace util {
		// ---------------------- Rect01 ----------------------
		HLVb Rect01::MakeVertex() {
			HLVb hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
			const vertex::screen c_vertex[] = {
				{{-1,-1}, {0,0}},
				{{-1,1}, {0,1}},
				{{1,1}, {1,1}},
				{{1,-1}, {1,0}}
			};
			hlVb->get()->initData(c_vertex, countof(c_vertex), sizeof(c_vertex[0]));
			return hlVb;
		}
		HLIb Rect01::MakeIndex() {
			const GLubyte c_index[] = {
				0,1,2, 2,3,0
			};
			HLIb hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
			hlIb->get()->initData(c_index, countof(c_index));
			return hlIb;
		}
		// ---------------------- ScreenRect ----------------------
		void ScreenRect::exportDrawTag(DrawTag& tag) const {
			tag.idIBuffer = _rect01.getIndex();
			tag.idVBuffer[0] = _rect01.getVertex();
		}
		void ScreenRect::draw(GLEffect& e) const {
			e.setVDecl(DrawDecl<vdecl::screen>::GetVDecl());
			e.setVStream(_rect01.getVertex(), 0);
			e.setIStream(_rect01.getIndex());
			e.drawIndexed(GL_TRIANGLES, _rect01.getIndex()->get()->getNElem());
		}
	}
	// ---------------------- Screen頂点宣言 ----------------------
	const SPVDecl& DrawDecl<vdecl::screen>::GetVDecl() {
		static SPVDecl vd(new VDecl{
			{0,0, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::POSITION},
			{0,8, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::TEXCOORD0}
		});
		return vd;
	}
}
