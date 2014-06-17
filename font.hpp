#pragma once
#ifdef WIN32
	#include <intrin.h>
#endif
#include "spinner/size.hpp"
#include "spinner/resmgr.hpp"
#include "spinner/abstbuff.hpp"
#include "font_base.hpp"
#include "font_ft_dep.hpp"

namespace rs {
	//! CharCodeとフォントテクスチャ対応付け (全Face共通)
	using FontChMap = std::unordered_map<CharID, CharPos>;
	// (FaceNameを複数箇所で共有する都合上)
	using SPString = std::shared_ptr<std::string>;
	struct Face {
		struct DepPair {
			FontArray_Dep	dep;
			CharPlane		cplane;

			DepPair(const SPString& name, const spn::PowSize& sfcSize, CCoreID cid);
			DepPair(DepPair&&) = default;
		};
		using DepMap = std::unordered_map<CCoreID, DepPair>;
		DepMap			depMap;
		SPString		faceName;
		CCoreID			coreID;		// family=0としたCoreID
		const spn::PowSize& sfcSize;
		FontChMap&		fontMap;

		Face(Face&& f) = default;
		Face(const SPString& name, const spn::PowSize& size, CCoreID cid, FontChMap& m);
		bool operator == (const std::string& name) const;
		bool operator != (const std::string& name) const;
		bool operator == (CCoreID cid) const;
		bool operator != (CCoreID cid) const;
		const CharPos* getCharPos(CharID cid);
		DepPair& getDepPair(CCoreID coreID);
	};
	//! 文章の描画に必要なフォントや頂点を用意
	/*! TriangleList形式。とりあえず改行だけ対応
		折り返し表示機能は無し
		全体の位置やサイズはシェーダーのconst変数で指定
		オフセットの基準をXYそれぞれで始点、終点、中央　のいずれか選択
		---------------- Uniform変数 ----------------
		Texture: EUnif::Diffuse */
	class TextObj {
		//! 文字列表示用頂点
		const static SPVDecl cs_vDecl;
		struct TextV {
			spn::Vec2	pos;
			spn::Vec3	uvt;
		};
		using CPosL = std::vector<const CharPos*>;
		struct DrawSet {
			HTex	hTex;
			HLVb	hlVb;	//!< フォント頂点
			HLIb	hlIb;
			int		nChar;	//!< スペースなど制御文字を除いた文字数
		};
		using DrawSetL = std::vector<DrawSet>;

		/*! GLリソース再構築時の為に元の文字列も保存
			このクラス自体はテクスチャを持たないのでIGLResourceは継承しない */
		std::u32string	_text;
		DrawSetL		_drawSet;
		CCoreID			_coreID;
		SPString		_faceName;
		spn::SizeF		_rectSize;

		//! フォントテクスチャと描画用頂点を用意
		void _init(Face &face);

		public:
			TextObj(TextObj&& t) = default;
			/*! \param[in] dep フォントデータを生成するための環境依存クラス
				\param[in] s 生成する文字列 */
			TextObj(Face& face, CCoreID coreID, std::u32string&& s);
			// FontGenから呼び出す
			void onCacheLost();
			void onCacheReset(Face& face);
			// ------- clearCache(bRestore=true)時 -------
			// FontGenから呼び出してFaceIDを再設定する用
			CCoreID& refCoreID();
			const SPString& getFaceName() const;
			// 上位クラスで位置調整など行列をセットしてからメソッドを呼ぶ
			void draw(GLEffect* gle);
			const spn::SizeF& getSize() const;
	};
	#define mgr_text (::rs::FontGen::_ref())

	//! フォント作成クラス: 共通
	/*! Depで生成するのはあくまでもフォントデータのみであって蓄積はこのクラスで行う */
	class FontGen : public spn::ResMgrN<TextObj, FontGen, std::allocator, std::u32string> {
		//! フォントの名前リスト
		/*! そんなに数いかないと思うのでvectorを使う */
		using FaceList = std::vector<Face>;
		//! フォント名リスト (通し番号)
		FaceList	_faceL;
		//! [CCoreID + CharCode]と[GLTexture + UV + etc..]を関連付け
		FontChMap	_fontMap;
		// OpenGLサーフェスのサイズは2の累乗サイズにする (余った領域は使わない)
		spn::PowSize	_sfcSize;

		/*! 既にFaceはmakeCoreIDで作成されてる筈 */
		Face& _getArray(CCoreID cid);
		//! 文字列先頭にCCoreIDの文字列を付加したものを返す
		/*! ハッシュキーはUTF32文字列に統一 */
		static std::u32string _MakeTextTag(CCoreID cid, const std::u32string& s);

		template <class S>
		CCoreID _makeCoreID(const S& name, CCoreID cid);

		public:
			/*! sfcSizeは強制的に2の累乗サイズに合わせられる
				\param[in] sfcSize	フォントを蓄えるOpenGLサーフェスのサイズ */
			FontGen(const spn::PowSize& sfcSize);
			//! フォントの設定から一意のIDを作る
			/*! \param[in] cid フォントの見た目情報。FaceIDは無視される
				\param[in] name フォントファミリの名前
				\return FaceIDを更新したcid */
			CCoreID makeCoreID(const std::string& name, CCoreID cid);
			// 上記のnameがSPStringバージョン
			/*! 動作は同じだが、もしFace名が無ければそのポインタを受け継いでFaceListに加える */
			CCoreID makeCoreID(const SPString& name, CCoreID cid);
			//! キャッシュを全て破棄
			/*! 文字列ハンドルは有効
				\param[in] bRestore trueならキャッシュを再確保する */
			void clearCache(bool bRestore);

			// 同じFaceの同じ文字列には同一のハンドルが返されるようにする
			/*! \param[in] cid makeCoreIDで作成したFaceID設定済みID
				\param[in] s 表示したい文字列 */
			LHdl createText(CCoreID cid, spn::To32Str str) {
				std::u32string str32(str.moveTo());
				// CCoreIDを付加した文字列をキーにする
				auto& ar = _getArray(cid);
				auto tag = _MakeTextTag(cid, str32);
				LHdl lh = emplace(std::move(tag), TextObj(ar, cid, std::move(str32))).first;
				return std::move(lh);
			}
			// デバイスロストで処理が必要なのはテクスチャハンドルだけなので、
			// onDeviceLostやonDeviceResetは特に必要ない
	};
}
