#pragma once
#include "../updater.hpp"
#include "../util/screenrect.hpp"

extern const rs::IdValue
	U_ViewPos,
	U_AlphaRange,
	U_DiffUVRect,
	U_Elevation,
	U_Normal,
	U_SrcUVOffset,
	U_SrcUVUnit,
	U_SrcUVRect,
	U_DestTexelRect,
	U_LightDir,
	U_LtoLayer,
	U_LayerSize,
	U_Ratio,
	T_Draw,
	T_TestPoly,
	U_SrcUVRect,
	U_SrcUVUnit,
	T_Upsampling,
	T_Sampling,
	T_MakeNormal;

namespace vertex {
	struct clip_cache {
		spn::Vec2	pos,
					uv;
	};
	struct clip {
		spn::Vec2	pos;
	};
}
class Engine;
struct IClipSource {
	struct Data {
		rs::HLTex	tex;
		spn::Vec4	uvrect;
		spn::Vec2	unit;

		Data(rs::HTex t, const spn::RectF& r, const spn::SizeF& s);
		Data(rs::HTex t, const spn::Rect& r, const spn::Size& s);
	};
	//! ソース全体のサイズ
	virtual spn::Size getSize() const = 0;
	//! 指定された矩形のデータをテクスチャとして取得
	/*! 次回の呼び出しで返されたテクスチャの内容が変更される場合がある */
	// type=(R16 texture) 512 levels
	virtual Data getDataRect(const spn::Rect& r) = 0;
};
class ClipTexSource : public IClipSource {
	private:
		rs::HLTex	_texture;
	public:
		ClipTexSource(rs::HTex t);

		spn::Size getSize() const override;
		Data getDataRect(const spn::Rect& r) override;
};
class ClipTestSource : public IClipSource {
	private:
		const float		_freqH,
						_freqV;
		rs::HLTex		_hTex;
		const float		_aux;
	public:
		ClipTestSource(float fH, float fV, spn::PowSize size, float aux);

		void save(const std::string& path) const;
		spn::Size getSize() const override;
		Data getDataRect(const spn::Rect& r) override;
};
using IClipSource_SP = std::shared_ptr<IClipSource>;

class Clipmap {
	public:
		using M4 = spn::AMat44;
		using M3 = spn::Mat33;
		using Vec2 = spn::Vec2;
		using Vec3 = spn::Vec3;
		using Vec4 = spn::Vec4;
		using Rect = spn::Rect;
		using RectF = spn::RectF;
		using Size = spn::Size;
		using PowSize = spn::PowSize;
		using PowInt = spn::PowInt;
		struct DrawCount {
			int				draw,
							not_draw;
		};
	private:
		// 頂点バッファ
		struct {
			rs::HLVb		block,			// 基本ブロック
							side,			// 縦横
							lshape,			// 左下, 右上
							degeneration;	// レイヤー間の継ぎ目
		} _vb;
		// インデックスバッファ
		struct {
			rs::HLIb		block[2],
							side[2],
							lshape[2],
							degeneration[2];
		} _ib;
		// 視点カリング用のサイズ情報
		struct {
			RectF			block,
							side,
							lshape,
							degeneration;
		} _gsize;
		enum Shape {
			EBlock,
			ESide,
			ELShape,
			EDegeneration
		};
		using VIBuff = std::pair<rs::HVb, rs::HIb>;

		rs::HLCam			_camera;
		spn::Vec3			_scale;
		const int			_tileWidth,		//!< 1レイヤーの頂点幅(2^n -1)
							_upsamp_n;
		rs::util::Rect01	_rect01;
		mutable DrawCount  _drawCount;

	public:
		class Layer : public IClipSource {
			private:
				rs::HLTex			_normal,
									_elevation;
				int					_cx, _cy;		//!< インクリメンタル更新用。直前に更新されキャッシュに収まっている範囲
				float				_scale;
				IClipSource_SP		_srcElev,
									_srcDiffuse;

				void _drawCache(rs::IEffect& e,
								const rs::util::Rect01& rect01,
								const Rect& rectG,
								const Rect& rectL);
				void _upDrawCache(rs::IEffect& e,
								const rs::util::Rect01& rect01,
								const Rect& rectG,
								const Rect& rectL,
								bool bNormal);
				using HLObjV = std::vector<rs::HLObj>;
				HLObjV _colview;
			public:
				//! テクスチャキャッシュの初期化
				/*!
					\param[in] s			確保するキャッシュのサイズ
				*/
				Layer(spn::PowSize s, float sc);
				void setElevSource(const IClipSource_SP& srcElev);
				void setDiffuseSource(const IClipSource_SP& srcDiffuse);
				float getScale() const;
				Size getSize() const override;
				IClipSource::Data getDataRect(const spn::Rect& r) override;
				bool isUpsamp() const;

				//! キャッシュの全更新
				/*!
					\param[in] tileW		レイヤーが内包する頂点一辺のサイズ
					\param[in] ox			タイル単位でのオフセットX
					\param[in] oy			タイル単位でのオフセットY
					\param[in] e			描画コール対象
					\param[in] rect01		描画コールのためのスクリーン矩形
				*/
				void refresh(int tileW, int ox, int oy,
								rs::IEffect& e,
								const rs::util::Rect01& rect01);
				//! キャッシュのインクリメンタル更新
				void incrementalRefresh(int tileW, int ox, int oy,
								rs::IEffect& e,
								const rs::util::Rect01& rect01);

				using CBf = std::function<void (rs::HTex, rs::HTex, const IClipSource_SP&, Shape, bool, const M3&, float)>;
				using IOfsP = std::pair<int,int>;
				// レイヤー左上のオフセットを指定する
				void drawLayer0(int bs, const CBf& cb) const;
				void drawLayerN(const spn::Vec2& center, int bs, const CBf& cb) const;
				void drawBlock12(int bs, const CBf& cb) const;
				void drawLShape(int bs, const IOfsP& inner, const CBf& cb) const;
				void save(const std::string& path) const;
				Size getCacheSize() const;
				rs::HTex getCache() const;
				rs::HTex getNormalCache() const;

				struct IOfs {
					int		ix, iy;
					float	mx, my;

					IOfs(const spn::Vec2& c, float s);
				};
				static IOfs GetDrawOffsetLocal(const spn::Vec2& center, float ratio);
				// ----- テスト用 -----
				static std::vector<Vec2> MakeTestHeight(PowSize s, float ratio);
				static std::vector<Vec4> MakeNormal(PowSize s, const std::vector<Vec2>& src, float ratio);
				static std::vector<Vec2> MakeUpsample(PowSize ps, const std::vector<Vec2>& src);
				static float TestElev(int w, float ratio, float x, float y);
		};

	private:
		using Layer_SP = std::shared_ptr<Layer>;
		using LayerV = std::vector<Layer_SP>;
		mutable LayerV		_layer;
		mutable bool		_bRefresh;		// 全クリアフラグ
		void _checkCache(rs::IEffect& e) const;
		void _initGLBuffer();
		void _initGSize();
		VIBuff _getVIBuffer(Shape shape, int iFlip) const;
		const RectF& _getGeometryRect(Shape shape) const;

		int _getBlockSize() const;
		void _testDrawPolygon(Engine& e) const;

	public:
		/*!
			\param[in] n			分割タイル幅
			\param[in] l			レイヤー数
			\param[in] upsamp_n		アップサンプリングレイヤ数
		*/
		Clipmap(spn::PowInt n, int l, int upsamp_n);
		// グリッド全体のサイズ
		void setGridSize(const spn::SizeF& s, float h);
		void setCamera(rs::HCam hCam);
		void draw(rs::IEffect& e) const;
		void save(const spn::PathBlock& pb) const;

		using HLTexV = std::vector<rs::HLTex>;
		HLTexV getCache() const;
		HLTexV getNormalCache() const;
		DrawCount getDrawCount() const;
};
class ClipmapObj : public rs::DrawableObjT<ClipmapObj>, public Clipmap {
	private:
		struct St_Def;
	public:
		ClipmapObj(spn::PowInt n, int l, int upsamp_n);
};
DEF_LUAIMPORT(ClipmapObj)
