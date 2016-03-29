// #include "../glx_if.hpp"
// #include "engine.hpp"

// namespace boom {
// 	using IndexV = std::vector<uint16_t>;
// }
// #define U_Def(str)			rs::IEffect::GlxId::GenUnifId(str)
// #define T_Def(tstr, pstr)	rs::IEffect::GlxId::GenTechId(tstr, pstr)
// const rs::IdValue
// 	U_ViewPos		= U_Def("u_viewPos"),
// 	U_AlphaBlend	= U_Def("u_alphaBlend"),
// 	U_DiffUVRect	= U_Def("u_diffUVRect"),
// 	U_Elevation		= U_Def("u_elevation"),
// 	U_Normal		= U_Def("u_normal"),
// 	U_SrcUVOffset	= U_Def("u_srcUVOffset"),
// 	U_SrcUVUnit		= U_Def("u_srcUVUnit"),
// 	U_SrcUVRect		= U_Def("u_srcUVRect"),
// 	U_DestTexelRect	= U_Def("u_destTexelRect"),
// 	U_LightDir		= U_Def("u_lightDir"),
// 	U_LtoLayer		= U_Def("u_toLayer"),
// 	U_LayerSize		= U_Def("u_layerSize"),
// 	T_Upsampling		= T_Def("Clipmap", "Upsampling"),
// 	T_Sampling			= T_Def("Clipmap", "Sampling"),
// 	T_MakeNormal		= T_Def("Clipmap", "MakeNormal"),
// 	T_Draw			= T_Def("Clipmap", "Draw"),
// 	T_TestPoly		= T_Def("Clipmap", "TestPolygon");
//
// Clipmap::Clipmap(const spn::PowInt n, const int l, const spn::PowInt fine_scale):
// 	_tileWidth(n-1),
// 	_nLayer(l),
// 	_fineScale(fine_scale),
// 	_bRefresh(true)
// {
// 	AssertT(Trap, l>=int(spn::Bit::MSB_N(fine_scale)+1), std::invalid_argument, "invalid layer N (n>=(fine_scale-1))")
// 	AssertT(Trap, int(fine_scale)>=1, std::invalid_argument, "invalid fine_scale (s>=1)")
// 	_layer.resize(_nLayer);
// 	const int w = _tileWidth*2 - (_tileWidth>>1);
// 	const spn::Size cs{w, w};
// 	spn::PowInt fs = 0x100/fine_scale;
// 	float hsc = 1.f/128;
// 	for(auto& l : _layer) {
// 		l = spn::construct(cs, fs);
// 		l->setHeightMap(std::make_shared<ClipTestSource>(hsc, hsc, cs, 1.f));
// 		fs = fs*2;
// 		hsc *= 2;
// 	}
// 	_initGLBuffer();
// }
// int Clipmap::_getBlockSize() const {
// 	return (_tileWidth+1)/4;
// }
// namespace {
// 	// 三角形2つ分のインデックスから四角ポリゴンを求める
// 	template <class T>
// 	bool GetRectIndex(T* pDst, const T* pSrc) {
// 		int tmp[3] = {
// 			pSrc[3],
// 			pSrc[4],
// 			pSrc[5]
// 		};
// 		auto find = [&tmp](const int idx){
// 			for(int i=0 ; i<3 ; i++) {
// 				if(idx == tmp[i]) {
// 					tmp[i] = -1;
// 					return i;
// 				}
// 			}
// 			return -1;
// 		};
// 		auto remain = [&tmp]() {
// 			for(auto t : tmp) {
// 				if(t >= 0)
// 					return t;
// 			}
// 			AssertFP(Trap, "")
// 		};
// 		const int at[3] = {
// 			find(pSrc[0]),
// 			find(pSrc[1]),
// 			find(pSrc[2])
// 		};
// 		pDst[0] = pSrc[0];
// 		const int rem = remain();
// 		if(at[0] < 0) {
// 			// 1,2 が接している
// 			pDst[1] = pSrc[1];
// 			pDst[2] = rem;
// 			pDst[3] = pSrc[2];
// 			return false;
// 		} else if(at[1] < 0) {
// 			// 2,0 が接している
// 			pDst[1] = pSrc[1];
// 			pDst[2] = pSrc[2];
// 			pDst[3] = rem;
// 			return true;
// 		} else {
// 			// 0,1 が接している
// 			pDst[1] = rem;
// 			pDst[2] = pSrc[1];
// 			pDst[3] = pSrc[2];
// 			return true;
// 		}
// 	}
// 	template <class T>
// 	void RectToTriangle(T* pDst, const T (&rect)[4], const bool bFirst) {
// 		const int c_idx[2][6] = {
// 			{0,1,2, 2,3,0},
// 			{0,1,3, 1,2,3}
// 		};
// 		const int (&idx)[6] = c_idx[bFirst ? 0 : 1];
// 		for(auto i : idx)
// 			*pDst++ = rect[i];
// 	}
// 	// 三角形2つで構成される四角ポリゴンのエッジを反転
// 	template <class T>
// 	void InvertEdge(T* pDst, const T* pSrc) {
// 		T tmp[4];
// 		RectToTriangle(pDst, tmp, !GetRectIndex(tmp, pSrc));
// 	}
// 	template <class T>
// 	std::vector<T> InvertRects(const std::vector<T>& src) {
// 		const int nI = src.size();
// 		std::vector<T> ret(nI);
// 		auto* pDst = ret.data();
// 		auto* pSrc = src.data();
// 		for(int i=0 ; i<nI ; i+=6)
// 			InvertEdge(pDst+i, pSrc+i);
// 		return ret;
// 	}
// }
// void Clipmap::_initGLBuffer() {
// 	using VArray = std::vector<spn::Vec2>;
// 	using IArray = boom::IndexV;
// 	auto fnMakeVb = [](auto&& va){
// 		auto h = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
// 		h->get()->initData(std::move(va));
// 		return h;
// 	};
// 	auto fnMakeIb = [](auto&& ia){
// 		auto h = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
// 		h->get()->initData(std::move(ia));
// 		return h;
// 	};
// 	auto fnMakeVI = [&fnMakeVb, &fnMakeIb](auto& vb, auto& ib, auto& va, auto& ia) {
// 		vb = fnMakeVb(va);
// 		ib = fnMakeIb(ia);
// 	};
// 	const int c_index[6] = {1,0,2, 2,3,1};
// 	auto fnTri = [&c_index](auto*& dst, const int (&idx)[4]) {
// 		for(auto c : c_index)
// 			*dst++ = idx[c];
// 	};
// 	VArray va;
// 	IArray ia;
// 	const int m = _getBlockSize();
// 	uint16_t* pIa;
// 	// レイヤーを構成する正方形ブロックM=(n+1)/4
// 	{
// 		va.resize(m*m);
// 		for(int i=0 ; i<m ; i++) {
// 			for(int j=0 ; j<m ; j++) {
// 				va[i*m+j] = spn::Vec2(j,i);
// 			}
// 		}
// 		ia.resize((m-1)*(m-1)*6);
// 		pIa = ia.data();
// 		for(int i=0 ; i<m-1 ; i++) {
// 			for(int j=0 ; j<m-1 ; j++) {
// 				const int idx[4] = {i*m+j,			i*m+j+1,
// 					(i+1)*m+j,		(i+1)*m+j+1};
// 				fnTri(pIa, idx);
// 			}
// 		}
// 		Assert(Trap, pIa == ia.data()+ia.size())
// 		fnMakeVI(_vb.block, _ib.block[0], va, ia);
// 		_ib.block[1] = _ib.block[0];
// 	}
// 	// 正方形ブロックの横を埋める3xMブロック
// 	{
// 		va.resize(3*m);
// 		for(int i=0 ; i<3 ; i++) {
// 			for(int j=0 ; j<m ; j++) {
// 				va[i*m+j] = spn::Vec2(j,i);
// 			}
// 		}
// 		ia.resize((2*(m-1))*6);
// 		pIa = ia.data();
// 		for(int i=0 ; i<2 ; i++) {
// 			for(int j=0 ; j<m-1 ; j++) {
// 				const int idx[4] = {j+m*i,		j+m*i+1,
// 									j+m*(i+1),	j+m*(i+1)+1};
// 				fnTri(pIa, idx);
// 			}
// 		}
// 		Assert(Trap, pIa == ia.data()+ia.size())
// 		_ib.side[1] = fnMakeIb(InvertRects(ia));
// 		fnMakeVI(_vb.side, _ib.side[0], va, ia);
// 	}
// 	// レイヤー内部の隙間を埋めるL字ブロック
// 	{
// 		const int nW = 2*m+1;
// 		va.resize(nW*2 + (nW-1)*2);
// 		// 横
// 		for(int i=0 ; i<nW ; i++) {
// 			va[i] = spn::Vec2(i,0);
// 			va[i+nW] = spn::Vec2(i,1);
// 		}
// 		// 縦
// 		auto* pVa = &va[nW*2];
// 		for(int i=1 ; i<nW ; i++) {
// 			pVa[i-1] = spn::Vec2(nW-2, i);
// 			pVa[nW-1 + i-1] = spn::Vec2(nW-1, i);
// 		}
// 		ia.resize(((nW-1)+(nW-2))*6);
// 		pIa = ia.data();
// 		// 横
// 		for(int i=0 ; i<nW-1 ; i++) {
// 			const int idx[4] = {i, i+1,
// 								nW+i, nW+i+1};
// 			fnTri(pIa, idx);
// 		}
// 		// 縦
// 		const int ofs = nW*2;
// 		for(int i=0 ; i<nW-2 ; i++) {
// 			const int idx[4] = {ofs+i, ofs+i+1,
// 								ofs+(nW-1)+i, ofs+(nW-1)+i+1};
// 			fnTri(pIa, idx);
// 		}
// 		Assert(Trap, pIa == ia.data()+ia.size())
// 		_ib.lshape[1] = fnMakeIb(InvertRects(ia));
// 		fnMakeVI(_vb.lshape, _ib.lshape[0], va, ia);
// 	}
// 	// レイヤーの境界の隙間を埋める輪っか
// 	{
// 		using Vec2 = spn::Vec2;
// 		const int nWI = 2*m,
// 			  nWIi = nWI-1;
// 		const int nWI_1 = (nWI-1)*2+1,
// 			  nWI_1i = nWIi*3;
// 		va.resize(nWI_1 * 4);
// 		ia.resize(nWI_1i * 4);
// 		auto* pV = va.data();
// 		auto* pI = ia.data();
// 		auto fnTri = [&pI](int i0, int i1, int i2, int ofs){
// 			*pI++ = i0+ofs;
// 			*pI++ = i1+ofs;
// 			*pI++ = i2+ofs;
// 		};
// 		// Left
// 		for(int i=0 ; i<nWI_1 ; i++)
// 			*pV++ = Vec2(0, i);
// 		for(int i=0 ; i<nWI_1i/3 ; i++)
// 			fnTri(0, 2, 1, i*2);
// 		AssertP(Trap, pV==va.data()+nWI_1)
// 		AssertP(Trap, pI==ia.data()+nWI_1i)
//
// 		// Top
// 		for(int i=0 ; i<nWI_1 ; i++)
// 			*pV++ = Vec2(i, (nWI-1)*2);
// 		for(int i=0 ; i<nWI_1i/3 ; i++)
// 			fnTri(0, 2, 1, i*2+nWI_1);
// 		AssertP(Trap, pV==va.data()+nWI_1*2)
// 		AssertP(Trap, pI==ia.data()+nWI_1i*2)
//
// 		// Right
// 		for(int i=0 ; i<nWI_1 ; i++)
// 			*pV++ = Vec2((nWI-1)*2, (nWI-1)*2-i);
// 		for(int i=0 ; i<nWI_1i/3 ; i++)
// 			fnTri(0, 2, 1, i*2+nWI_1*2);
// 		AssertP(Trap, pV==va.data()+nWI_1*3)
// 		AssertP(Trap, pI==ia.data()+nWI_1i*3)
//
// 		// Bottom
// 		for(int i=0 ; i<nWI_1 ; i++)
// 			*pV++ = Vec2(i, 0);
// 		for(int i=0 ; i<nWI_1i/3 ; i++)
// 			fnTri(0, 2, 1, i*2+nWI_1*3);
// 		Assert(Trap, pV == va.data()+nWI_1*4)
// 		Assert(Trap, pI == ia.data()+nWI_1i*4)
// 		fnMakeVI(_vb.degeneration, _ib.degeneration[0], va, ia);
// 		_ib.degeneration[1] = _ib.degeneration[0];
// 	}
// }
// std::pair<int,int> Clipmap::_getTilePos() const {
// 	const auto& cp = _camera->getPose();
// 	const auto ofs = cp.getOffset();
// 	return std::make_pair(ofs.x/_scale.x,
// 						ofs.y/_scale.z);
// }
// void Clipmap::_checkCache(rs::IEffect& e) const {
// 	Assert(Throw, _detailSource && _camera)
// 	int ox, oy;
// 	std::tie(ox,oy) = _getTilePos();
// 	std::function<void (Layer&)> func;
// 	if(_bRefresh) {
// 		// 全更新
// 		_bRefresh = false;
// 		func = [&e,ox,oy,this](Layer& l) {
// 			l.refresh(_tileWidth, ox, oy,
// 						e, _rect01);
// 		};
// 	} else {
// 		// 部分更新 or 更新なし
// 		func = [&e,ox,oy,this](Layer& l) {
// 			l.incrementalRefresh(_tileWidth, ox, oy,
// 						e, _rect01);
// 		};
// 	}
// 	for(auto& l : _layer)
// 		func(*l);
// }
// void Clipmap::setGridSize(const spn::SizeF& s, float h) {
// 	_scale = {s.width, h, s.height};
// 	_bRefresh = true;
// }
// void Clipmap::setCamera(rs::HCam hCam) {
// 	_camera = hCam;
// 	_bRefresh = true;
// }
// void Clipmap::setDetailMap(const IClipSource_SP& src) {
// 	_detailSource = src;
// 	_bRefresh = true;
// }
// void Clipmap::setDetailMapTexture(rs::HTex h) {
// 	setDetailMap(std::make_shared<ClipTexSource>(h));
// }
// Clipmap::VIBuff Clipmap::_getVIBuffer(Shape shape, const int iFlip) const {
// 	switch(shape) {
// 		case EBlock:
// 			return VIBuff(_vb.block, _ib.block[iFlip]);
// 		case ESide:
// 			return VIBuff(_vb.side, _ib.side[iFlip]);
// 		case ELShape:
// 			return VIBuff(_vb.lshape, _ib.lshape[iFlip]);
// 		case EDegeneration:
// 			return VIBuff(_vb.degeneration, _ib.degeneration[iFlip]);
// 	}
// 	__assume(0);
// }
// namespace {
// 	const spn::Vec3 c_color[] = {
// 		{1,1,1},
// 		{1,0,0},
// 		{0,1,0},
// 		{0.5f, 0.5f, 1}
// 	};
// }
// #include "../sys_uniform_value.hpp"
// namespace {
// 	template <class T, int N>
// 	void Zeromat(T (&t)[N]) {
// 		for(auto& m : t) {
// 			if(std::abs(m) < 1e-5f)
// 				m = 0;
// 		}
// 	}
// }
// void Clipmap::_testDrawPolygon(Engine& e) const {
// 	e.setTechPassId(T_TestPoly);
// 	e.setVDecl(rs::DrawDecl<rs::vdecl::screen>::GetVDecl());
// // 	e.setUniform(U_LightDir, );
//
// 	auto c = e.ref3D().getCamera();
// 	auto cofs = c->getPose().getOffset();
// 	const Vec2 cofs2(cofs.x, cofs.z);
// 	const auto iofs0 = Layer::GetDrawOffsetLocal(cofs2, 128);
// 	const Vec2 cmod2(iofs0.mx, iofs0.my);
// 	auto fnDraw = [&e, &cmod2, &cofs2, this](rs::HTex hElev, rs::HTex hNml, Shape shape, const bool bFlip, const M3& toLayer, const float sc) {
// 		const float sc2 = sc*2;
// 		float mx = std::fmod(cofs2.x, sc2);
// 		if(cofs2.x < 0)
// 			mx = sc2 + mx;
// 		float my = std::fmod(cofs2.y, sc2);
// 		if(cofs2.y < 0)
// 			my = sc2 + my;
// 		const int ix = static_cast<int>(std::floor(mx));
// 		const int iy = static_cast<int>(std::floor(my));
// 		const Vec2 mofs(-ix - cmod2.x,
// 						-iy - cmod2.y);
// 		auto mW = M4::Scaling(sc,sc,0,1) * M4::Translation( Vec3(sc,sc,0)+cofs2.asVec3(0)+mofs.asVec3(0)) * M4::RotationX(spn::DegF(90));
// 		Zeromat(mW.data);
// 		auto toL = toLayer;
// 		Zeromat(toL.data);
// 		e.ref3D().setWorld(mW);
// 		e.setUniform(U_LtoLayer, toL, true);
// 		e.setUniform(U_Elevation, hElev);
// 		auto tmp = Vec2(std::floor(cofs2.x/(sc*2)),
// 						std::floor(cofs2.y/(sc*2)))*2;
// 		e.setUniform(U_SrcUVOffset, tmp);
// 		e.setUniform(U_Normal, hNml);
// // 		e.setUniform(rs::unif::texture::Diffuse, );
// 		const auto sz = hElev->get()->getSize();
// 		const Vec2 uvunit(1.f/sz.width, 1.f/sz.height);
// 		e.setUniform(U_SrcUVUnit, uvunit);
// // 		e.setUniform(U_DiffUVRect, );
//
// 		e.setUniform(rs::unif::Color, c_color[shape]);
// 		e.setUniform(U_ViewPos, -mofs/sc);
// 		e.setUniform(U_AlphaBlend, 0.6f);
// 		e.setUniform(U_LayerSize, float(_tileWidth-1));
// 		auto buff = _getVIBuffer(shape, bFlip ? 1 : 0);
// 		e.setVStream(buff.first, 0);
// 		e.setIStream(buff.second);
// 		e.drawIndexed(GL_TRIANGLES, buff.second->get()->getNElem(), 0);
// 	};
// 	const int bs = _getBlockSize()-1;
// 	_layer[0]->drawLayer0(bs, fnDraw);
// 	const int nL = _layer.size();
// 	for(int i=1 ; i<nL ; i++)
// 		_layer[i]->drawLayerN(cofs2, bs, fnDraw);
// }
// void Clipmap::draw(rs::IEffect& e) const {
// 	if(0) {
// 	auto cofs = e.ref3D().getCamera()->getPose().getOffset();
// 	const Vec2 cofs2(cofs.x, cofs.z);
// 	const auto kusoge = (_tileWidth-1)/2;
// 	static int count = 0;
// 	if(count==0) {
// 		float sc = 1.f;
// 		for(auto& l : _layer) {
// 			Vec2 ofs(std::floor(cofs2.x/(sc*2)),
// 					std::floor(cofs2.y/(sc*2)));
// 			ofs *= 2;
// 			sc *= 2;
// 			l->refresh(_tileWidth, ofs.x-kusoge, ofs.y-kusoge, e, _rect01);
// 			l->incrementalRefresh(_tileWidth, ofs.x-kusoge, ofs.y-kusoge, e, _rect01);
// 		}
// 	} else {
// 		float sc = 1.f;
// 		for(auto& l : _layer) {
// 			Vec2 ofs(std::floor(cofs2.x/(sc*2)),
// 					std::floor(cofs2.y/(sc*2)));
// 			ofs *= 2;
// 			sc *= 2;
// 			l->incrementalRefresh(_tileWidth, ofs.x-kusoge, ofs.y-kusoge, e, _rect01);
// 		}
// 	}
// 	if(count == 3) {
// 		save(spn::PathBlock("/tmp"));
// 		std::quick_exit(0);
// 	}
// 	++count;
// 	Layer::Test({32,32}, 256, e);
// 	}
// 	auto& en = static_cast<Engine&>(e);
// 	_testDrawPolygon(en);
// 	int ox, oy;
// 	std::tie(ox,oy) = _getTilePos();
// 	try {
// 		// キャッシュの更新チェック(全更新 or インクリメンタル更新)
// 		_checkCache(e);
// 		_layer[0]->drawLayer0(ox, oy, _tileWidth, fnDraw);
// 		int width=1;
// 		const int nL = _layer.size();
// 		for(int i=1 ; i<nL ; i++) {
// 			const int lx = (ox / width) & 0x01,
// 					ly = (oy / width) & 0x01;
// 			_layer[i]->drawLayerN(ox, oy, _tileWidth, lx, ly, fnDraw);
// 			width <<= 1;
// 		}
// 	} catch(...) {
// 		// 必要な変数がセットされていない
// 	}
// }
