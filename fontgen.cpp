#include "font.hpp"
#include "lane.hpp"
#include "glx.hpp"
#include "sys_uniform.hpp"
#include "spinner/emplace.hpp"
#include "drawtag.hpp"

namespace rs {
	namespace {
		// 最低サイズ2bits, Layer1=4bits, Layer0=6bits = 12bits(4096)
		using LAlloc = LaneAlloc<6,4,2>;
	}
	// --------------------------- Face::DepPair ---------------------------
	Face::DepPair::DepPair(const SPString& name, const spn::PowSize& sfcSize, CCoreID cid):
		dep(*name, cid), cplane(sfcSize, dep.height(), UPLaneAlloc(new LAlloc()))
	{}
	// --------------------------- Face ---------------------------
	// フォントのHeightとラインのHeightは違う！
	Face::Face(const SPString& name, const spn::PowSize& size, CCoreID cid, FontChMap& m):
		faceName(name),
		coreID(cid),
		sfcSize(size),
		fontMap(m)
	{}
	bool Face::operator != (const std::string& name) const {
		return !(this->operator == (name));
	}
	bool Face::operator ==(const std::string& name) const {
		return *faceName == name;
	}
	bool Face::operator != (CCoreID cid) const {
		return !(this->operator == (cid));
	}
	bool Face::operator == (CCoreID cid) const {
		return coreID == cid;
	}
	Face::DepPair& Face::getDepPair(CCoreID coreID) {
		return spn::TryEmplace(depMap, coreID, faceName, sfcSize, coreID).first->second;
	}
	const CharPos* Face::getCharPos(CharID chID) {
		// キャッシュが既にあればそれを使う
		auto itr = fontMap.find(chID);
		if(itr != fontMap.end())
			return &itr->second;

		// CharMapにエントリを作る
		CharPos& cp = fontMap[chID];
		// Dependクラスから文字のビットデータを取得
		auto& dp = getDepPair(chID);
		auto res = dp.dep.getChara(chID.code);
		// この時点では1ピクセル8bitなので、32bitRGBAに展開
		res.first = Convert8Bit_Packed32Bit(&res.first[0], res.second.width(), res.second.width(), res.second.height());
		cp.box = res.second;
		cp.space = dp.dep.width(chID.code);
		if(res.second.width() <= 0) {
			cp.uv *= 0;
			cp.hTex = mgr_gl.getEmptyTexture();
		} else {
			LaneRaw lraw;
			dp.cplane.rectAlloc(lraw, res.second.width());
			cp.hTex = lraw.hTex;

			// ビットデータをglTexSubImage2Dで書き込む
			auto* u = reinterpret_cast<Texture_Mem*>(lraw.hTex.ref().get());
			u->writeRect(spn::AB_Byte(std::move(res.first)), lraw.rect.width(), lraw.rect.x0, lraw.rect.y0, GL_UNSIGNED_BYTE);

			// UVオフセットを計算
			const auto& sz = dp.cplane.getSurfaceSize();
			float invW = spn::Rcp22Bit(static_cast<float>(sz.width)),
				invH = spn::Rcp22Bit(static_cast<float>(sz.height));

			float h = res.second.height();
			const auto& uvr = lraw.rect;
			cp.uv = spn::RectF(uvr.x0 * invW,
							uvr.x1 * invW,
							uvr.y0 * invH,
							(uvr.y0+h) * invH);
		}
		return &cp;
	}

	// --------------------------- VDecl for text ---------------------------
	const SPVDecl& DrawDecl<drawtag::text>::GetVDecl() {
		static SPVDecl vd(new VDecl({
			{0, 0, GL_FLOAT, GL_FALSE, 2, (GLuint)VSem::POSITION},
			{0, 8, GL_FLOAT, GL_FALSE, 3, (GLuint)VSem::TEXCOORD0}
		}));
		return vd;
	}
	// --------------------------- TextObj ---------------------------
	TextObj::TextObj(Face& face, CCoreID coreID, std::u32string&& s): _text(std::move(s)), _coreID(coreID), _faceName(face.faceName) {
		_init(face);
	}
	void TextObj::_init(Face& face) {
		auto& dp = face.getDepPair(_coreID);
		int height = dp.dep.height();
		// CharPosリストの作成
		// 1文字につき4頂点 + インデックス6個
		struct CPair {
			const CharPos*	cp;
			int ofsx, ofsy;
			float timeval;

			CPair(const CharPos* c, int x, int y, float t): cp(c), ofsx(x), ofsy(y), timeval(t) {}
		};
		int ofsx = 0,
			ofsy = -height;
		// テクスチャが複数枚に渡る時はフォント頂点(座標)を使いまわし、UV-tだけを差し替え
		std::map<HTex, std::vector<CPair>>	tpM;
		float dt = spn::Rcp22Bit(_text.length()),
			t = 0;
		for(auto& c : _text) {
			auto* p = face.getCharPos(CharID(c, _coreID));
			// 幾つのテクスチャが要るのかカウントしつつ、フォントを配置
			if(c == U'\n') {
				ofsy -= height;
				ofsx = 0;
			} else {
				if(p->box.width() > 0)
					tpM[p->hTex].emplace_back(p, ofsx, ofsy, t);
				ofsx += p->space;
			}
			t += dt;
		}

		const uint16_t c_index[] = {0,1,2, 2,3,0};
		const std::pair<int,int> c_rectI[] = {{0,2}, {1,2}, {1,3}, {0,3}};

		int nplane = tpM.size();
		_drawSet.resize(nplane);
		_rectSize *= 0;
		auto itr = tpM.begin();
		for(int i=0 ; i<nplane ; i++, ++itr) {
			auto& ds = _drawSet[i];
			std::vector<CPair>& cpl = itr->second;
			int nC = cpl.size(),
				vbase = 0;

			// フォント配置
			spn::ByteBuff vbuff(nC * 4 * sizeof(vertex::text));
			spn::U16Buff ibuff(nC * 6);
			auto* pv = reinterpret_cast<vertex::text*>(&vbuff[0]);
			auto* pi = &ibuff[0];
			for(int i=0 ; i<nC ; i++) {
				auto& cp = cpl[i];
				// 頂点生成
				for(auto& r : c_rectI) {
					pv->pos = spn::Vec2(cp.ofsx + cp.cp->box.ar[r.first], cp.ofsy - cp.cp->box.ar[r.second]);
					pv->uvt = spn::Vec3(cp.cp->uv.ar[r.first], cp.cp->uv.ar[r.second], cp.timeval);
					_rectSize.width = std::max(_rectSize.width, pv->pos.x);
					_rectSize.height = std::min(_rectSize.height, pv->pos.y);
					++pv;
				}
				// インデックス生成
				for(auto idx : c_index)
					*pi++ = vbase + idx;
				vbase += 4;
			}

			ds.hTex = itr->first;
			ds.nChar = nC;
			// GLバッファにセット
			ds.hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
			ds.hlVb.ref()->initData(std::move(vbuff), sizeof(vertex::text));
			ds.hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
			ds.hlIb.ref()->initData(std::move(ibuff));
		}
	}
	void TextObj::exportDrawTag(DrawTag& d) const {
		constexpr int maxVb = sizeof(d.idVBuffer)/sizeof(d.idVBuffer[0]),
					maxTex = sizeof(d.idTex)/sizeof(d.idTex[0]);
		int curVb = 0,
			curTex = 0;
		bool bFirst = true;
		for(auto& ds : _drawSet) {
			if(curVb != maxVb)
				d.idVBuffer[curVb++] = ds.hlVb.get();
			if(curTex != maxTex)
				d.idTex[curTex++] = ds.hTex;
			if(bFirst) {
				d.idIBuffer = ds.hlIb.get();
				bFirst = false;
			}
		}
	}
	void TextObj::onCacheLost() {
		// フォントキャッシュを消去
		_drawSet.clear();
	}
	void TextObj::onCacheReset(Face& face) {
		// 再度テキストデータ(CharPosポインタ配列)を作成
		_init(face);
	}
	CCoreID& TextObj::refCoreID() { return _coreID; }
	const SPString& TextObj::getFaceName() const { return _faceName; }
	void TextObj::draw(GLEffect* gle) const {
		gle->setVDecl(DrawDecl<drawtag::text>::GetVDecl());
		for(auto& ds : _drawSet) {
			gle->setUniform(unif::texture::Diffuse, ds.hTex);
			gle->setVStream(ds.hlVb.get(), 0);
			gle->setIStream(ds.hlIb.get());
			gle->drawIndexed(GL_TRIANGLES, ds.nChar*6, 0);
		}
	}
	const spn::SizeF& TextObj::getSize() const {
		return _rectSize;
	}

	// --------------------------- FontGen ---------------------------
	namespace {
		SPString ToSp(const std::string& s) { return SPString(new std::string(s)); }
		SPString ToSp(const SPString& s) { return s; }
		const std::string& ToCp(const std::string& s) { return s; }
		const std::string& ToCp(const SPString& s) { return *s; }
	}
	FontGen::FontGen(const spn::PowSize& sfcSize): _sfcSize(sfcSize) {}

	template <class S>
	CCoreID FontGen::_makeCoreID(const S& name, CCoreID cid) {
		// cid = フォントファミリを無視した値
		auto itr = std::find(_faceL.begin(), _faceL.end(), ToCp(name));
		if(itr == _faceL.end()) {
			// 新しくFaceを作成
			cid.at<CCoreID::FaceID>() = static_cast<int>(_faceL.size());
			_faceL.emplace_back(ToSp(name), _sfcSize, cid, _fontMap);
		} else {
			cid.at<CCoreID::FaceID>() = itr - _faceL.begin();
		}
		return cid;
	}

	CCoreID FontGen::makeCoreID(const std::string& name, CCoreID cid) {
		return _makeCoreID(name, cid);
	}
	CCoreID FontGen::makeCoreID(const SPString& name, CCoreID cid) {
		return _makeCoreID(name, cid);
	}
	Face& FontGen::_getArray(CCoreID cid) {
		// FaceList線形探索
		auto itr = std::find_if(_faceL.begin(), _faceL.end(), [cid](const Face& fc){
			return fc.coreID.at<CCoreID::FaceID>() == cid.at<CCoreID::FaceID>();
		});
		AssertP(Trap, itr != _faceL.end())
		return *itr;
	}
	void FontGen::clearCache(bool bRestore) {
		// あくまでもキャッシュのクリアなので文字列データ等は削除しない
		_fontMap.clear();
		_faceL.clear();
		// 文字列クラスのキャッシュも破棄
		for(auto& text : *this)
			text.onCacheLost();

		if(bRestore) {
			for(auto& text: *this) {
				// FaceIDは使わず再度Face参照する
				auto& c = text.refCoreID();
				// TextからSPの名前を取り出してFaceIDを更新
				c = makeCoreID(text.getFaceName(), c);
				text.onCacheReset(_getArray(c));
			}
		}
	}
	std::u32string FontGen::_MakeTextTag(CCoreID cid, const std::u32string& s) {
		// ハンドルキー = CCoreIDの64bit数値 + _ + 文字列
		std::basic_stringstream<char32_t>	ss;
		ss << boost::lexical_cast<std::u32string>(cid.value()) << U'_' << s;
		return ss.str();
	}
	FontGen::LHdl FontGen::createText(CCoreID cid, spn::To32Str str) {
		std::u32string str32(str.moveTo());
		// CCoreIDを付加した文字列をキーにする
		auto& ar = _getArray(cid);
		auto tag = _MakeTextTag(cid, str32);
		return acquire(std::move(tag),
					[&](const auto&){ return TextObj(ar, cid, std::move(str32)); }).first;
	}
}
