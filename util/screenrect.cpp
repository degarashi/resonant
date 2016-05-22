#include "screenrect.hpp"
#include "../glx.hpp"
#include "../drawtag.hpp"
#include "../sys_uniform.hpp"
#include "../systeminfo.hpp"

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
			void DrawRect(rs::IEffect& e, GLenum dtype, rs::HVb hVb, rs::HIb hIb) {
				e.setVDecl(rs::DrawDecl<rs::vdecl::screen>::GetVDecl());
				e.setVStream(hVb, 0);
				e.setIStream(hIb);
				const int nElem = hIb->get()->getNElem();
				e.drawIndexed(dtype, nElem, 0);
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
		void Rect01::draw(rs::IEffect& e) const {
			DrawRect(e, GL_TRIANGLES, getVertex(), getIndex());
		}
		// ---------------------- WireRect01 ----------------------
		HLVb WireRect01::MakeVertex() {
			return Rect01::MakeVertex();
		}
		HLIb WireRect01::MakeIndex() {
			const GLubyte c_index[] = {
				0, 1, 2, 3
			};
			HLIb hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
			hlIb->get()->initData(c_index, countof(c_index));
			return hlIb;
		}
		void WireRect01::draw(rs::IEffect& e) const {
			DrawRect(e, GL_LINE_LOOP, getVertex(), getIndex());
		}
		// ---------------------- Rect11 ----------------------
		HLVb Rect11::MakeVertex() {
			return MakeVertex_Base(-1.f, 1.f);
		}
		HLIb Rect11::MakeIndex() {
			return Rect01::MakeIndex();
		}
		void Rect11::draw(rs::IEffect& e) const {
			DrawRect(e, GL_TRIANGLES, getVertex(), getIndex());
		}
		// ---------------------- WindowRect ----------------------
		WindowRect::WindowRect():
			_color(1,1,1),
			_alpha(1),
			_depth(1),
			_bWire(false)
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
			tag.idIBuffer = (_bWire) ? _wrect01.getIndex() : _rect01.getIndex();
			tag.idVBuffer[0] = (_bWire) ? _wrect01.getVertex() : _rect01.getVertex();
			tag.zOffset = _depth;
		}
		void WindowRect::setWireframe(const bool bWireframe) {
			_bWire = bWireframe;
		}
		void WindowRect::draw(IEffect& e) const {
			const auto s = mgr_info.getScreenSize();
			const float rx = spn::Rcp22Bit(s.width/2),
						ry = spn::Rcp22Bit(s.height/2);
			const auto& sc = getScale();
			const auto& ofs = getOffset();

			const float rv = _bWire ? -1 : 0;
			auto m = spn::AMat33::Scaling(rx*(sc.x+rv), -ry*(sc.y+rv), 1.f);
			m *= spn::AMat33::Translation({-1+rx/2, 1-ry/2});
			m *= spn::AMat33::Translation({ofs.x*rx, -ofs.y*ry});
			e.setVDecl(DrawDecl<vdecl::screen>::GetVDecl());
			auto& s2 = e.ref2D();
			s2.setWorld(m);
			e.setUniform(unif2d::Color, _color);
			e.setUniform(unif2d::Alpha, _alpha);
			e.setUniform(unif2d::Depth, _depth);
			if(_bWire)
				_wrect01.draw(e);
			else
				_rect01.draw(e);
		}
		// ---------------------- ScreenRect ----------------------
		void ScreenRect::exportDrawTag(DrawTag& tag) const {
			tag.idIBuffer = _rect11.getIndex();
			tag.idVBuffer[0] = _rect11.getVertex();
		}
		void ScreenRect::draw(IEffect& e) const {
			_rect11.draw(e);
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
