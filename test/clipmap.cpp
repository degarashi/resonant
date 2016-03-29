#include "clipmap.hpp"
#include "../glresource.hpp"
#include "../camera.hpp"
#include "engine.hpp"

namespace boom {
	using IndexV = std::vector<uint16_t>;
}
#define U_Def(str)			rs::IEffect::GlxId::GenUnifId(str)
#define T_Def(tstr, pstr)	rs::IEffect::GlxId::GenTechId(tstr, pstr)
const rs::IdValue
	U_ViewPos		= U_Def("u_viewPos"),
	U_AlphaRange	= U_Def("u_alphaRange"),
	U_ViewUVRect	= U_Def("u_viewUVRect"),
	U_Elevation		= U_Def("u_elevation"),
	U_Normal		= U_Def("u_normal"),
	U_SrcUVOffset	= U_Def("u_srcUVOffset"),
	U_SrcUVUnit		= U_Def("u_srcUVUnit"),
	U_SrcUVRect		= U_Def("u_srcUVRect"),
	U_DestTexelRect	= U_Def("u_destTexelRect"),
	U_LightDir		= U_Def("u_lightDir"),
	U_LtoLayer		= U_Def("u_toLayer"),
	U_LayerSize		= U_Def("u_layerSize"),
	U_Ratio			= U_Def("u_ratio"),
	T_Upsampling	= T_Def("Clipmap", "Upsampling"),
	T_Sampling		= T_Def("Clipmap", "Sampling"),
	T_MakeNormal	= T_Def("Clipmap", "MakeNormal"),
	T_Draw			= T_Def("Clipmap", "Draw"),
	T_TestPoly		= T_Def("Clipmap", "TestPolygon");

// --------------------- Clipmap ---------------------
Clipmap::Clipmap(const PowInt n, const int l, const int upsamp_n):
	_tileWidth(n-1),
	_upsamp_n(upsamp_n),
	_bRefresh(true)
{
	AssertT(Trap, l>=0, std::invalid_argument, "invalid layer N (n>=1)")
	AssertT(Trap, upsamp_n>=0, std::invalid_argument, "invalid upsamp_n(n>=0)")
	AssertT(Trap, l>upsamp_n, std::invalid_argument, "nLayer > upsamp_n")
	_layer.resize(l);

	// タイルサイズ*1.5の値を2の乗数で切り上げたサイズをキャッシュとする
	const int w = _tileWidth*2 - (_tileWidth>>1);
	const PowSize cs{w, w};
	float sc = 1.f;
	float hsc = 1.f/133;
	auto hlTex = mgr_gl.loadTexture("block.jpg", rs::MipState::MipmapLinear);
	hlTex->get()->setLinear(true);
	auto dsrc = std::make_shared<ClipTexSource>(hlTex);
	for(int i=upsamp_n ; i<l ; i++) {
		auto& l = _layer[i];
		l = std::make_shared<Layer>(cs, sc);
		l->setElevSource(std::make_shared<ClipTestSource>(hsc, hsc, cs, 1.f));
		l->setDiffuseSource(dsrc);
		hsc *= 2;
		sc *= 2;
	}
	sc = .5f;
	for(int i=upsamp_n-1 ; i>=0 ; i--) {
		auto& l = _layer[i];
		l = std::make_shared<Layer>(cs, sc);
		l->setElevSource(_layer[i+1]);
		l->setDiffuseSource(dsrc);
		sc /= 2;
	}
	_initGLBuffer();
	_initGSize();
}
int Clipmap::_getBlockSize() const {
	return (_tileWidth+1)/4;
}
namespace {
	// 三角形2つ分のインデックスから四角ポリゴンを求める
	template <class T>
	bool GetRectIndex(T* pDst, const T* pSrc) {
		int tmp[3] = {
			pSrc[3],
			pSrc[4],
			pSrc[5]
		};
		auto find = [&tmp](const int idx){
			for(int i=0 ; i<3 ; i++) {
				if(idx == tmp[i]) {
					tmp[i] = -1;
					return i;
				}
			}
			return -1;
		};
		auto remain = [&tmp]() {
			for(auto t : tmp) {
				if(t >= 0)
					return t;
			}
			AssertFP(Trap, "")
		};
		const int at[3] = {
			find(pSrc[0]),
			find(pSrc[1]),
			find(pSrc[2])
		};
		pDst[0] = pSrc[0];
		const int rem = remain();
		if(at[0] < 0) {
			// 1,2 が接している
			pDst[1] = pSrc[1];
			pDst[2] = rem;
			pDst[3] = pSrc[2];
			return false;
		} else if(at[1] < 0) {
			// 2,0 が接している
			pDst[1] = pSrc[1];
			pDst[2] = pSrc[2];
			pDst[3] = rem;
			return true;
		} else {
			// 0,1 が接している
			pDst[1] = rem;
			pDst[2] = pSrc[1];
			pDst[3] = pSrc[2];
			return true;
		}
	}
	template <class T>
	void RectToTriangle(T* pDst, const T (&rect)[4], const bool bFirst) {
		const int c_idx[2][6] = {
			{0,1,2, 2,3,0},
			{0,1,3, 1,2,3}
		};
		const int (&idx)[6] = c_idx[bFirst ? 0 : 1];
		for(auto i : idx)
			*pDst++ = rect[i];
	}
	// 三角形2つで構成される四角ポリゴンのエッジを反転
	template <class T>
	void InvertEdge(T* pDst, const T* pSrc) {
		T tmp[4];
		RectToTriangle(pDst, tmp, !GetRectIndex(tmp, pSrc));
	}
	template <class T>
	std::vector<T> InvertRects(const std::vector<T>& src) {
		const int nI = src.size();
		std::vector<T> ret(nI);
		auto* pDst = ret.data();
		auto* pSrc = src.data();
		for(int i=0 ; i<nI ; i+=6)
			InvertEdge(pDst+i, pSrc+i);
		return ret;
	}
}
void Clipmap::_initGSize() {
	const int m = _getBlockSize(),
			m1 = m-1;
	// レイヤーを構成する正方形ブロックM=(n+1)/4
	_gsize.block = RectF(0, m1, 0, m1);
	// 正方形ブロックの横を埋める3xMブロック
	_gsize.side = RectF(0, m1, 0, 2);
	// レイヤー内部の隙間を埋めるL字ブロック
	_gsize.lshape = RectF(0, 2*m1+1 +1, 0, 2*m1+1 +1);
	// レイヤーの境界の隙間を埋める輪っか
	_gsize.degeneration = RectF(0, 4*m1+2, 0, 4*m1+2);
}
void Clipmap::_initGLBuffer() {
	using VArray = std::vector<spn::Vec2>;
	using IArray = boom::IndexV;
	auto fnMakeVb = [](auto&& va){
		auto h = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
		h->get()->initData(std::move(va));
		return h;
	};
	auto fnMakeIb = [](auto&& ia){
		auto h = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
		h->get()->initData(std::move(ia));
		return h;
	};
	auto fnMakeVI = [&fnMakeVb, &fnMakeIb](auto& vb, auto& ib, auto& va, auto& ia) {
		vb = fnMakeVb(va);
		ib = fnMakeIb(ia);
	};
	const int c_index[6] = {1,0,2, 2,3,1};
	auto fnTri = [&c_index](auto*& dst, const int (&idx)[4]) {
		for(auto c : c_index)
			*dst++ = idx[c];
	};
	VArray va;
	IArray ia;
	const int m = _getBlockSize();
	uint16_t* pIa;
	// レイヤーを構成する正方形ブロックM=(n+1)/4
	{
		va.resize(m*m);
		for(int i=0 ; i<m ; i++) {
			for(int j=0 ; j<m ; j++) {
				va[i*m+j] = spn::Vec2(j,i);
			}
		}
		ia.resize((m-1)*(m-1)*6);
		pIa = ia.data();
		for(int i=0 ; i<m-1 ; i++) {
			for(int j=0 ; j<m-1 ; j++) {
				const int idx[4] = {i*m+j,			i*m+j+1,
					(i+1)*m+j,		(i+1)*m+j+1};
				fnTri(pIa, idx);
			}
		}
		Assert(Trap, pIa == ia.data()+ia.size())
		fnMakeVI(_vb.block, _ib.block[0], va, ia);
		_ib.block[1] = _ib.block[0];
	}
	// 正方形ブロックの横を埋める3xMブロック
	{
		va.resize(3*m);
		for(int i=0 ; i<3 ; i++) {
			for(int j=0 ; j<m ; j++) {
				va[i*m+j] = spn::Vec2(j,i);
			}
		}
		ia.resize((2*(m-1))*6);
		pIa = ia.data();
		for(int i=0 ; i<2 ; i++) {
			for(int j=0 ; j<m-1 ; j++) {
				const int idx[4] = {j+m*i,		j+m*i+1,
									j+m*(i+1),	j+m*(i+1)+1};
				fnTri(pIa, idx);
			}
		}
		Assert(Trap, pIa == ia.data()+ia.size())
		_ib.side[1] = fnMakeIb(InvertRects(ia));
		fnMakeVI(_vb.side, _ib.side[0], va, ia);
	}
	// レイヤー内部の隙間を埋めるL字ブロック
	{
		const int nW = 2*m+1;
		va.resize(nW*2 + (nW-1)*2);
		// 横
		for(int i=0 ; i<nW ; i++) {
			va[i] = spn::Vec2(i,0);
			va[i+nW] = spn::Vec2(i,1);
		}
		// 縦
		auto* pVa = &va[nW*2];
		for(int i=1 ; i<nW ; i++) {
			pVa[i-1] = spn::Vec2(nW-2, i);
			pVa[nW-1 + i-1] = spn::Vec2(nW-1, i);
		}
		ia.resize(((nW-1)+(nW-2))*6);
		pIa = ia.data();
		// 横
		for(int i=0 ; i<nW-1 ; i++) {
			const int idx[4] = {
				i, i+1,
				nW+i, nW+i+1
			};
			fnTri(pIa, idx);
		}
		// 縦
		const int ofs = nW*2;
		for(int i=0 ; i<nW-2 ; i++) {
			const int idx[4] = {
				ofs+(nW-1)+i, ofs+(nW-1)+i+1,
				ofs+i, ofs+i+1
			};
			fnTri(pIa, idx);
		}
		Assert(Trap, pIa == ia.data()+ia.size())
		_ib.lshape[1] = fnMakeIb(InvertRects(ia));
		fnMakeVI(_vb.lshape, _ib.lshape[0], va, ia);
	}
	// レイヤーの境界の隙間を埋める輪っか
	{
		using Vec2 = spn::Vec2;
		const int nWI = 2*m,
			  nWIi = nWI-1;
		const int nWI_1 = (nWI-1)*2+1,
			  nWI_1i = nWIi*3;
		va.resize(nWI_1 * 4);
		ia.resize(nWI_1i * 4);
		auto* pV = va.data();
		auto* pI = ia.data();
		auto fnTri = [&pI](int i0, int i1, int i2, int ofs){
			*pI++ = i0+ofs;
			*pI++ = i1+ofs;
			*pI++ = i2+ofs;
		};
		// Left
		for(int i=0 ; i<nWI_1 ; i++)
			*pV++ = Vec2(0, i);
		for(int i=0 ; i<nWI_1i/3 ; i++)
			fnTri(0, 2, 1, i*2);
		AssertP(Trap, pV==va.data()+nWI_1)
		AssertP(Trap, pI==ia.data()+nWI_1i)

		// Top
		for(int i=0 ; i<nWI_1 ; i++)
			*pV++ = Vec2(i, (nWI-1)*2);
		for(int i=0 ; i<nWI_1i/3 ; i++)
			fnTri(0, 2, 1, i*2+nWI_1);
		AssertP(Trap, pV==va.data()+nWI_1*2)
		AssertP(Trap, pI==ia.data()+nWI_1i*2)

		// Right
		for(int i=0 ; i<nWI_1 ; i++)
			*pV++ = Vec2((nWI-1)*2, (nWI-1)*2-i);
		for(int i=0 ; i<nWI_1i/3 ; i++)
			fnTri(0, 2, 1, i*2+nWI_1*2);
		AssertP(Trap, pV==va.data()+nWI_1*3)
		AssertP(Trap, pI==ia.data()+nWI_1i*3)

		// Bottom
		for(int i=0 ; i<nWI_1 ; i++)
			*pV++ = Vec2(i, 0);
		for(int i=0 ; i<nWI_1i/3 ; i++)
			fnTri(0, 2, 1, i*2+nWI_1*3);
		Assert(Trap, pV == va.data()+nWI_1*4)
		Assert(Trap, pI == ia.data()+nWI_1i*4)
		fnMakeVI(_vb.degeneration, _ib.degeneration[0], va, ia);
		_ib.degeneration[1] = _ib.degeneration[0];
	}
}
void Clipmap::setGridSize(const spn::SizeF& s, float h) {
	_scale = {s.width, h, s.height};
	_bRefresh = true;
}
void Clipmap::setCamera(rs::HCam hCam) {
	_camera = hCam;
	_bRefresh = true;
}
Clipmap::VIBuff Clipmap::_getVIBuffer(Shape shape, const int iFlip) const {
	switch(shape) {
		case EBlock:
			return VIBuff(_vb.block, _ib.block[iFlip]);
		case ESide:
			return VIBuff(_vb.side, _ib.side[iFlip]);
		case ELShape:
			return VIBuff(_vb.lshape, _ib.lshape[iFlip]);
		case EDegeneration:
			return VIBuff(_vb.degeneration, _ib.degeneration[iFlip]);
	}
	__assume(0);
}
const spn::RectF& Clipmap::_getGeometryRect(Shape shape) const {
	switch(shape) {
		case EBlock:
			return _gsize.block;
		case ESide:
			return _gsize.side;
		case ELShape:
			return _gsize.lshape;
		case EDegeneration:
			return _gsize.degeneration;
	}
	__assume(0);
}
void Clipmap::_checkCache(rs::IEffect& e) const {
	Assert(Throw, _camera)
	auto cofs = e.ref3D().getCamera()->getPose().getOffset();
	auto fnL = [this, cofs](auto&& cb){
		const Vec2 cofs2(cofs.x, cofs.z);
		const int nL = _layer.size();
		auto sc = float(1 << (nL-_upsamp_n-1));
		for(int i=nL-1 ; i>=0 ; i--) {
			auto ofs = Vec2(std::floor(cofs2.x/(sc*2)),
							std::floor(cofs2.y/(sc*2))) * 2;
			sc /= 2;
			cb(*_layer[i], ofs);
		}
	};
	const auto twh = (_tileWidth-1)/2;
	if(_bRefresh) {
		// 全更新
		_bRefresh = false;
		fnL([twh, &e, this](auto& layer, auto& ofs){
			layer.refresh(_tileWidth, ofs.x-twh, ofs.y-twh, e, _rect01);
		});
	} else {
		// インクリメンタル更新
		fnL([twh, &e, this](auto& layer, auto& ofs){
			layer.incrementalRefresh(_tileWidth, ofs.x-twh, ofs.y-twh, e, _rect01);
		});
	}
}
void Clipmap::draw(rs::IEffect& e) const {
	try {
		auto& en = static_cast<Engine&>(e);
		_checkCache(e);
		_testDrawPolygon(en);
	} catch(...) {
		// 必要な変数がセットされていない
	}
}
namespace {
	const spn::Vec3 c_color[] = {
		{1,1,1},
		{1,0,0},
		{0,1,0},
		{0.5f, 0.5f, 1}
	};
	template <class T, int N>
	void Zeromat(T (&t)[N]) {
		for(auto& m : t) {
			if(std::abs(m) < 1e-5f)
				m = 0;
		}
	}
}
#include "../sys_uniform_value.hpp"
#include "spinner/rectdiff.hpp"
#include "boomstick/geom3D.hpp"
#include "colview.hpp"
#include "spinner/misc.hpp"
void Clipmap::_testDrawPolygon(Engine& e) const {
	const Vec2 aR(_getBlockSize()+1 +1,
					(_tileWidth-1)/2-1 -1);
	auto c = e.ref3D().getCamera();
	auto cofs = c->getPose().getOffset();
	const Vec2 cofs2(cofs.x, cofs.z);
	const auto lowscale = _layer[0]->getScale();
	const auto iofs0 = Layer::GetDrawOffsetLocal(cofs2, lowscale);
	const Vec2 cmod2(iofs0.mx, iofs0.my);
	_drawCount.draw = _drawCount.not_draw = 0;
	// ビューカリングの為にカメラの視錐台を渡す
	const auto vf = boom::geo3d::FrustumM(c->getVFrustum());
	const auto vp = vf.getPoints(true);
	boom::geo3d::ConvexPM vfp(vp.point, countof(vp.point));
	auto fnDraw = [aR, &e, &cmod2, &cofs2, lowscale, this, &vfp](rs::HTex hElev, rs::HTex hNml, const auto& srcDiffuse,  Shape shape, const bool bFlip, const M3& toLayer, const float sc) {
		using spn::rect::LoopValueD;
		using spn::rect::LoopValue;
		const auto ls2 = lowscale*2;
		const int r = int(std::round(sc/lowscale));
		auto ix = LoopValue(int(LoopValueD(cofs2.x, ls2)), r),
				iy = LoopValue(int(LoopValueD(cofs2.y, ls2)), r);

		const Vec2 mofs(-ix*ls2 - cmod2.x,
						-iy*ls2 - cmod2.y);
		auto mW = M4::Scaling(sc,sc,0,1) * M4::Translation( Vec3(sc,sc,0)+cofs2.asVec3(0)+mofs.asVec3(0) ) * M4::RotationX(spn::DegF(90));
		Zeromat(mW.data);
		{
			auto rect = _getGeometryRect(shape);
			auto l0 = Vec3(rect.x0, rect.y0, 1) * toLayer,
				 l1 = Vec3(rect.x1, rect.y1, 1) * toLayer;
			// LayerSpace -> WorldSpace
			auto v0 = (l0.asVec4(1) * mW).asVec3(),
				v1 = (l1.asVec4(1) * mW).asVec3();
			v0.y = -64/4;
			v1.y = 64/4;
			boom::geo3d::AABBM ab;
			ab.vmin = ab.vmax = v0;
			ab.vmin.selectMin(v1);
			ab.vmax.selectMax(v1);

			// Vec3 col;
			// switch(shape) {
			// 	case Shape::EDegeneration: col=Vec3(1,0,0); break;
			// 	case Shape::EBlock: col=Vec3(0,1,0); break;
			// 	case Shape::ESide: col=Vec3(0,0,1); break;
			// 	case Shape::ELShape: col=Vec3(1,1,1); break;
			// }
			// Vec3 cent(ab.vmax - ab.vmin);
			// cent /= 2;
			// ColBox cb;
			// cb.setOffset(ab.vmin + cent);
			// cb.setScale(cent);
			// cb.setAlpha(0.25f);
			// cb.setColor(col);
			// cb.draw(e);

			// 視界に入ってなければ描画しない
			const bool b0 = boom::geo3d::GSimplex(ab, vfp).getResult();
			if(!b0) {
				++_drawCount.not_draw;
				return;
			}
		}
		++_drawCount.draw;
		auto toL = toLayer;
		Zeromat(toL.data);
		e.setTechPassId(T_TestPoly);
		e.setVDecl(rs::DrawDecl<rs::vdecl::screen>::GetVDecl());
		e.ref3D().setWorld(mW);
		e.setUniform(U_LtoLayer, toL, true);
		e.setUniform(U_Elevation, hElev);
		e.setUniform(U_Normal, hNml);
		{
			auto srcuv = Vec2(
							std::floor(cofs2.x/(sc*2)),
							std::floor(cofs2.y/(sc*2))
						)*2;
			e.setUniform(U_SrcUVOffset, srcuv);

			const int isc = sc;
			const int x = int(srcuv.x) * isc * _tileWidth,
						y = int(srcuv.y) * isc * _tileWidth;
			auto dif = srcDiffuse->getDataRect({x,
					x + _tileWidth*isc,
					y,
					y + _tileWidth*isc});
			e.setUniform(rs::unif::texture::Diffuse, dif.tex);
			e.setUniform(U_ViewUVRect, dif.uvrect);
		}
		{
			const auto sz = hElev->get()->getSize();
			const Vec2 uvunit(1.f/sz.width, 1.f/sz.height);
			e.setUniform(U_SrcUVUnit, uvunit);
		}
		e.setUniform(rs::unif::Color, c_color[shape]);
		e.setUniform(U_ViewPos, -mofs/sc);
		e.setUniform(U_AlphaRange, aR);
		auto buff = _getVIBuffer(shape, bFlip ? 1 : 0);
		e.setVStream(buff.first, 0);
		e.setIStream(buff.second);
		e.drawIndexed(GL_TRIANGLES, buff.second->get()->getNElem(), 0);
	};
	const int bs = _getBlockSize()-1;
	_layer[0]->drawLayer0(bs, fnDraw);
	const int nL = _layer.size();
	for(int i=1 ; i<nL ; i++)
		_layer[i]->drawLayerN(cofs2, bs, fnDraw);
}
Clipmap::HLTexV Clipmap::getCache() const {
	const int nL = _layer.size();
	HLTexV ret(nL);
	for(int i=0 ;i<nL ; i++) {
		ret[i] = _layer[i]->getCache();
	}
	return ret;
}
Clipmap::HLTexV Clipmap::getNormalCache() const {
	const int nL = _layer.size();
	HLTexV ret(nL);
	for(int i=0 ;i<nL ; i++) {
		ret[i] = _layer[i]->getNormalCache();
	}
	return ret;
}
Clipmap::DrawCount Clipmap::getDrawCount() const {
	return _drawCount;
}
