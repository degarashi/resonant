#pragma once
#include "spinner/size.hpp"
#include "glresource.hpp"

namespace rs {
	//! キャラクタの属性値を32bitで表す
	struct CharIDDef : spn::BitDef<uint32_t,
		spn::BitF<0,8>, // Width
		spn::BitF<8,8>, // Height
		spn::BitF<16,8>, // FaceID
		spn::BitF<24,2>, // Flag
		spn::BitF<26,1>, // Italic
		spn::BitF<27,3>, // Weight
		spn::BitF<30,2> // SizeType
	> {
		enum { Width, Height, FaceID, CharFlag, Italic, Weight, SizeType };
		enum CharFlagT {
			CharFlag_AA = 0x01,
			CharFlag_Hemming = 0x02
		};
		//! フォントのサイズ指定方法
		enum SizeTypeT {
			SizeType_Pixel,			//!< ピクセル単位の指定
			SizeType_Point,			//!< DPIに相対したポイント指定
			SizeType_LineHeight		//!< ラインに収まるサイズ
		};
	};
	//! フォントのサイズやAAの有無を示す値
	struct CCoreID : spn::BitField<CharIDDef> {
		CCoreID() = default;
		CCoreID(const CCoreID& id) = default;
		CCoreID(int w, int h, uint32_t charFlag, bool bItalic, int weightID, CharIDDef::SizeTypeT sizeType, int faceID=-1) {
			at<Width>() = w;
			at<Height>() = h;
			at<CharFlag>() = charFlag;
			at<Italic>() = static_cast<int>(bItalic);
			at<Weight>() = weightID;
			at<FaceID>() = faceID;
			at<SizeType>() = sizeType;
		}
	};
	//! CCoreID + 文字コード(UCS4)
	struct CharID : CCoreID {
		char32_t	code;

		CharID() = default;
		CharID(const CharID& id) = default;
		CharID(char32_t ccode, CCoreID coreID): CCoreID(coreID), code(ccode) {}
		CharID(char32_t ccode, int w, int h, int faceID, CharIDDef::CharFlagT flag, bool bItalic, int weightID, CharIDDef::SizeTypeT sizeType):
			CCoreID(w, h, flag, bItalic, weightID, sizeType, faceID), code(ccode) {}

		uint64_t get64Bit() const {
			uint64_t val = code;
			val <<= 32;
			val |= cleanedValue();
			return val;
		}
		bool operator == (const CharID& cid) const {
			return get64Bit() == cid.get64Bit();
		}
		bool operator != (const CharID& cid) const {
			return !(this->operator==(cid));
		}
		bool operator < (const CharID& cid) const {
			return get64Bit() < cid.get64Bit();
		}
	};
	using FontName = spn::FixStr<char,32>;
}
namespace std {
	template <>
	struct hash<rs::CharID> {
		uint32_t operator()(const rs::CharID& cid) const {
			// MSBを必ず0にする
			uint32_t tmp = cid.code ^ 0x1234abcd;
			return ((cid.cleanedValue() * cid.code) ^ tmp) & 0x7fffffff;
		}
	};
	template <>
	struct hash<rs::FontName> {
		uint32_t operator()(const rs::FontName& fn) const {
			// MSBを必ず1にする
			return hash<std::string>()(std::string(fn)) | 0x80000000;
		}
	};
	template <>
	struct hash<rs::CCoreID> {
		uint32_t operator()(const rs::CCoreID& cid)	const {
			return cid.cleanedValue();
		}
	};
}
namespace rs {
	struct LaneRaw {
		HTex		hTex;
		spn::Rect	rect;	//!< 管理している領域
	};
	struct Lane : LaneRaw {
		Lane	*pNext = nullptr;
		Lane(HTex hT, const spn::Rect& r): LaneRaw{hT, r} {}
	};
	struct ILaneAlloc {
		virtual ~ILaneAlloc() {}
		virtual bool alloc(LaneRaw& dst, size_t w) = 0;
		virtual void addFreeLane(HTex hTex, const spn::Rect& rect) = 0;
		virtual void clear() = 0;
	};
	using UPLaneAlloc = std::unique_ptr<ILaneAlloc>;
	//! CharPlaneと、その位置
	struct CharPos {
		HTex		hTex;		//!< フォントが格納されているテクスチャ (ハンドル所有権は別途CharPlaneが持つ)
		spn::RectF	uv;			//!< 参照すべきUV値
		spn::Rect	box;		//!< フォント原点に対する相対描画位置 (サイズ)
		int			space;		//!< カーソルを進めるべき距離
	};
	//! フォントのGLテクスチャ
	/*! 縦幅は固定。横は必要に応じて確保 */
	class CharPlane {
		using PlaneVec = std::vector<HLTex>;
		PlaneVec		_plane;
		spn::PowSize	_sfcSize;
		const int	_fontH;		//!< フォント縦幅 (=height)
		UPLaneAlloc	_lalloc;	//!< レーンの残り幅管理
		int			_nUsed;		//!< 割り当て済みのChar数(動作には影響しない)
		int			_nH;		//!< Plane一枚のLane数
		float		_dV;		//!< 1文字のVサイズ

		//! キャッシュテクスチャを一枚追加 -> Lane登録
		void _addCacheTex();

		public:
			//! フォントキャッシュテクスチャの確保
			/*! \param size[in] テクスチャ1辺のサイズ
				\param fw[in] Char幅
				\param fh[in] Char高 */
			CharPlane(const spn::PowSize& size, int fh, UPLaneAlloc&& a);
			CharPlane(CharPlane&& cp);
			//! 新しいChar登録領域を確保
			/*! まだどこにも登録されてないcodeである事はFontArray_Depが保証する
				\param[out] dst uv, hTexを書き込む */
			void rectAlloc(LaneRaw& dst, int width);
			const spn::PowSize& getSurfaceSize() const;
	};
}
