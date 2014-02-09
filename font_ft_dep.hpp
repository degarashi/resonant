//! FontCache - FreeTypeを使った実装
#pragma once
#include "spinner/dir.hpp"
#include "font_base.hpp"
#include "ftwrap.hpp"

namespace rs {
	#define mgr_font reinterpret_cast<::rs::FontFamily&>(::rs::FontFamily::_ref())
	//! 対象ディレクトリからフォントファイルを列挙しリストアップ
	class FontFamily : public FTLibrary {
		public:
			struct Item {
				int 	faceIndex;
				HLRW	hlRW;
				using OPPath = spn::Optional<std::string>;
				OPPath	path;

				Item(Item&& it);
				Item(int fIdx, HRW hRW);
				Item(int fIdx, const std::string& p);
				HLFT makeFont() const;
			};
		private:
			using SPFace = std::shared_ptr<FTFace>;
			//! [FamilyName -> FullPath]
			using FontMap = std::unordered_map<std::string, Item>;
			FontMap			_fontMap;

		public:
			void loadFamilyWildCard(spn::To8Str pattern);
			void loadFamily(HRW hRW);

			//! FamilyNameからフォントを特定
			HLFT fontFromFamilyName(const std::string& name) const;
			//! ファイル名を指定してフォントを探す
			HLFT fontFromFile(const std::string& path);
			//! サイズや形式からフォントを探す
			HLFT fontFromID(CCoreID id) const;
	};
	//! フォント作成クラス: 環境依存
	/*! フォントの設定毎に用意する */
	class Font_FTDep {
		CCoreID			_coreID;
		// FTFaceとCCoreIDの対応
		HLFT			_hlFT;
		// 一時バッファは対応する最大サイズで確保
		spn::ByteBuff	_buff;
		//! キャラクタフラグ (AAや縁取りなど)
		int				_charType;
		int				_boldP;
		bool			_bItalic;
		//! 描画に必要な範囲を取得
		spn::Rect _boundingRect(char32_t code) const;

		public:
			Font_FTDep(Font_FTDep&& dep);
			Font_FTDep(const std::string& name, CCoreID cid);
			Font_FTDep& operator = (Font_FTDep&& dep);

			//! 結果的にCCoreIDが同じになるパラメータの値を統一
			/*! (依存クラスによってはサイズが縦しか指定できなかったりする為) */
			CCoreID adjustParams(CCoreID cid);

			//! 使用テクスチャとUV範囲、カーソル移動距離など取得
			/*! \return first=フォントピクセルデータ(各ピクセル8bit)
						second=フォント原点に対する描画オフセット */
			std::pair<spn::ByteBuff, spn::Rect> getChara(char32_t code);
			int maxWidth() const;
			int height() const;
			int width(char32_t c);
	};
	using FontArray_Dep = Font_FTDep;
}
namespace std {
	template <>
	struct hash<rs::FontFamily::Item> {
		size_t operator()(const rs::FontFamily::Item& it) const {
			if(it.hlRW)
				return it.hlRW.get().getValue();
			return hash<std::string>()(it.path.get());
		}
	};
}
