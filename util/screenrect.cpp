#include "screenrect.hpp"
#include "../glx.hpp"
#include "../drawtag.hpp"
#include "../sys_uniform.hpp"
#include "../gameloophelper.hpp"

namespace rs {
	namespace util {
		namespace {
			HLVb MakeVertex_Base(float minv, float maxv) {
				HLVb hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
				const vertex::screen c_vertex[] = {
					{{minv, minv}, {0,0}},
					{{minv, maxv}, {0,1}},
					{{maxv, maxv}, {1,1}},
					{{maxv, minv}, {1,0}}
				};
				hlVb->get()->initData(c_vertex, countof(c_vertex), sizeof(c_vertex[0]));
				return hlVb;
			}
		}
		// ---------------------- Rect01 ----------------------
		HLVb Rect01::MakeVertex() {
			return MakeVertex_Base(0.f, 1.f);
		}
		HLIb Rect01::MakeIndex() {
			const GLubyte c_index[] = {
				0,1,2, 2,3,0
			};
			HLIb hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
			hlIb->get()->initData(c_index, countof(c_index));
			return hlIb;
		}
		// ---------------------- Rect11 ----------------------
		HLVb Rect11::MakeVertex() {
			return MakeVertex_Base(-1.f, 1.f);
		}
		HLIb Rect11::MakeIndex() {
			return Rect01::MakeIndex();
		}
		// ---------------------- WindowRect ----------------------
		WindowRect::WindowRect():
			_color(1,1,1),
			_alpha(1),
			_depth(1)
		{}
		void WindowRect::setColor(const spn::Vec3& c) {
			_color = c;
		}
		void WindowRect::setAlpha(float a) {
			_alpha = a;
		}
		void WindowRect::setDepth(float d) {
			_depth = d;
		}
		void WindowRect::exportDrawTag(DrawTag& tag) const {
			tag.idIBuffer = _rect01.getIndex();
			tag.idVBuffer[0] = _rect01.getVertex();
			tag.zOffset = _depth;
		}
		void WindowRect::draw(IEffect& e, SystemUniform2D& s2) const {
			auto lkb = sharedbase.lock();
			auto s = lkb->screenSize;
			float rx = spn::Rcp22Bit(s.width/2),
					ry = spn::Rcp22Bit(s.height/2);
			auto& sc = getScale();
			auto& ofs = getOffset();
			auto m = spn::AMat33::Scaling(rx*sc.x, -ry*sc.y, 1.f);
			m *= spn::AMat33::Translation({-1, 1});
			m *= spn::AMat33::Translation({ofs.x*rx, -ofs.y*ry});
			e.setVDecl(DrawDecl<vdecl::screen>::GetVDecl());
			s2.setWorld(m);
			e.setUniform(unif2d::Color, _color);
			e.setUniform(unif2d::Alpha, _alpha);
			e.setUniform(unif2d::Depth, _depth);
			e.setVStream(_rect01.getVertex(), 0);
			e.setIStream(_rect01.getIndex());
			e.drawIndexed(GL_TRIANGLES, _rect01.getIndex()->get()->getNElem());
		}
		// ---------------------- ScreenRect ----------------------
		void ScreenRect::exportDrawTag(DrawTag& tag) const {
			tag.idIBuffer = _rect11.getIndex();
			tag.idVBuffer[0] = _rect11.getVertex();
		}
		void ScreenRect::draw(IEffect& e) const {
			e.setVDecl(DrawDecl<vdecl::screen>::GetVDecl());
			e.setVStream(_rect11.getVertex(), 0);
			e.setIStream(_rect11.getIndex());
			e.drawIndexed(GL_TRIANGLES, _rect11.getIndex()->get()->getNElem());
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
