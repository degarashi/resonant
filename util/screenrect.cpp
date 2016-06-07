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
		GeomP Rect01::MakeGeom(...) {
			const GLubyte c_index[] = {
				0,1,2, 2,3,0
			};
			HLIb hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
			hlIb->get()->initData(c_index, countof(c_index));
			return {MakeVertex_Base(0.f, 1.f), std::move(hlIb)};
		}
		void Rect01::draw(rs::IEffect& e) const {
			const auto& g = getGeom();
			DrawRect(e, GL_TRIANGLES, g.first, g.second);
		}
		// ---------------------- WireRect01 ----------------------
		GeomP WireRect01::MakeGeom(...) {
			const GLubyte c_index[] = {
				0, 1, 2, 3
			};
			HLIb hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
			hlIb->get()->initData(c_index, countof(c_index));
			return {Rect01::MakeGeom().first, std::move(hlIb)};
		}
		void WireRect01::draw(rs::IEffect& e) const {
			const auto& g = getGeom();
			DrawRect(e, GL_LINE_LOOP, g.first, g.second);
		}
		// ---------------------- Rect11 ----------------------
		GeomP Rect11::MakeGeom(...) {
			return {MakeVertex_Base(-1.f, 1.f), Rect01::MakeGeom().second};
		}
		void Rect11::draw(rs::IEffect& e) const {
			auto& g = getGeom();
			DrawRect(e, GL_TRIANGLES, g.first, g.second);
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
			auto& g = (_bWire) ? _wrect01.getGeom() : _rect01.getGeom();
			tag.idIBuffer = g.second;
			tag.idVBuffer[0] = g.first;
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
			auto& g = _rect11.getGeom();
			tag.idIBuffer = g.second;
			tag.idVBuffer[0] = g.first;
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
