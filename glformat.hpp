#pragma once
#include "glhead.hpp"
#include <boost/preprocessor.hpp>
#include <boost/variant.hpp>
#include <unordered_map>
#include "spinner/optional.hpp"
#include "spinner/misc.hpp"
#include "sdlformat.hpp"
#include "spinner/structure/niftycounter.hpp"

namespace rs {
	#ifdef USE_OPENGLES2
		#define PSEQ_COMPRESSED
		#define PSEQ_INTERNAL			(GL_ALPHA)(GL_LUMINANCE)(GL_LUMINANCE_ALPHA)(GL_RGB)(GL_RGBA)
		#define PSEQ_DEPTHFORMAT		(GL_DEPTH_COMPONENT)(GL_DEPTH_COMPONENT16)
		#define PSEQ_STENCILFORMAT		(GL_STENCIL_INDEX)(GL_STENCIL_INDEX8)
		#define PSEQ_DSFORMAT

		#define SEQ_INTERNAL			PSEQ_INTERNAL
		#define SEQ_INTERNALSIZED		SEQ_INTERNAL
		#define SEQ_INTERNALCOMPRESSED	SEQ_INTERNALSIZED PSEQ_COMPRESSED
		#define SEQ_INTERNALREAD		(GL_ALPHA)(GL_RGB)(GL_RGBA)
		#define SEQ_INTERNALRENDER		(GL_RGBA4)(GL_RGB565)(GL_RGB5_A1)(GL_DEPTH_COMPONENT16)(GL_STENCIL_INDEX8)
		#define SEQ_TYPE				(GL_UNSIGNED_BYTE)(GL_UNSIGNED_SHORT_5_6_5)(GL_UNSIGNED_SHORT_4_4_4_4)(GL_UNSIGNED_SHORT_5_5_5_1)
	#else
		//! 圧縮フォーマット
		#define PSEQ_COMPRESSED		(GL_COMPRESSED_RED)(GL_COMPRESSED_RG)(GL_COMPRESSED_RGB)(GL_COMPRESSED_RGBA)(GL_COMPRESSED_SRGB)(GL_COMPRESSED_SRGB_ALPHA) \
			(GL_COMPRESSED_RED_RGTC1)(GL_COMPRESSED_SIGNED_RED_RGTC1)(GL_COMPRESSED_RG_RGTC2)(GL_COMPRESSED_SIGNED_RG_RGTC2) \
			(GL_COMPRESSED_RGBA_BPTC_UNORM_ARB)(GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB)(GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB)(GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB)
		#define PSEQ_INTERNAL (GL_RED)(GL_RG)(GL_RGB)(GL_BGR)(GL_RGBA)(GL_BGRA)
		//! 深度フォーマットシーケンス
		#define PSEQ_DEPTHFORMAT (GL_DEPTH_COMPONENT)(GL_DEPTH_COMPONENT16)(GL_DEPTH_COMPONENT24)(GL_DEPTH_COMPONENT32)(GL_DEPTH_COMPONENT32F)
		//! ステンシルフォーマットシーケンス
		#define PSEQ_STENCILFORMAT (GL_STENCIL_INDEX)(GL_STENCIL_INDEX4)(GL_STENCIL_INDEX8)(GL_STENCIL_INDEX16)
		//! 深度&ステンシルフォーマットシーケンス
		#define PSEQ_DSFORMAT (GL_DEPTH_STENCIL)(GL_DEPTH24_STENCIL8)

		//! 色フォーマットシーケンス
		#define SEQ_INTERNAL			PSEQ_INTERNAL
		#define SEQ_INTERNALSIZED		SEQ_INTERNAL (GL_R8)(GL_R8_SNORM)(GL_R16)(GL_R16_SNORM) \
			(GL_RG8)(GL_RG8_SNORM)(GL_RG16)(GL_RG16_SNORM)(GL_R3_G3_B2)(GL_RGB4)(GL_RGB5)(GL_RGB8)(GL_RGB8_SNORM)(GL_RGB10)(GL_RGB12)(GL_RGB16_SNORM) \
			(GL_RGBA2)(GL_RGBA4)(GL_RGB5_A1)(GL_RGBA8)(GL_RGBA8_SNORM)(GL_RGB10_A2)(GL_RGB10_A2UI)(GL_RGBA12)(GL_RGBA16)(GL_SRGB8)(GL_SRGB8_ALPHA8) \
			(GL_R16F)(GL_RG16F)(GL_RGB16F)(GL_RGBA16F)(GL_R32F)(GL_RG32F)(GL_RGB32F)(GL_RGBA32F)(GL_R11F_G11F_B10F)(GL_RGB9_E5)(GL_R8I) \
			(GL_R8UI)(GL_R16I)(GL_R16UI)(GL_R32I)(GL_R32UI)(GL_RG8I)(GL_RG8UI)(GL_RG16I)(GL_RG16UI)(GL_RG32I)(GL_RG32UI)(GL_RGB8I)(GL_RGB8UI) \
			(GL_RGB16I)(GL_RGB16UI)(GL_RGB32I)(GL_RGB32UI)(GL_RGBA8I)(GL_RGBA8UI)(GL_RGBA16I)(GL_RGBA16UI)(GL_RGBA32I)(GL_RGBA32UI)
		#define SEQ_INTERNALCOMPRESSED	SEQ_INTERNALSIZED PSEQ_COMPRESSED
		#define SEQ_INTERNALREAD		SEQ_INTERNAL (GL_DEPTH_COMPONENT)(GL_STENCIL_INDEX)
		#define SEQ_INTERNALRENDER		SEQ_INTERNALSIZED PSEQ_DEPTHFORMAT PSEQ_STENCILFORMAT PSEQ_DSFORMAT
		#define SEQ_TYPE				(GL_UNSIGNED_BYTE)(GL_BYTE)(GL_UNSIGNED_SHORT)(GL_SHORT)(GL_UNSIGNED_INT) \
			(GL_INT)(GL_HALF_FLOAT)(GL_FLOAT)(GL_UNSIGNED_BYTE_3_3_2)(GL_UNSIGNED_BYTE_2_3_3_REV)(GL_UNSIGNED_SHORT_5_6_5) \
			(GL_UNSIGNED_SHORT_5_6_5_REV)(GL_UNSIGNED_SHORT_4_4_4_4)(GL_UNSIGNED_SHORT_4_4_4_4_REV)(GL_UNSIGNED_SHORT_5_5_5_1) \
			(GL_UNSIGNED_SHORT_1_5_5_5_REV)(GL_UNSIGNED_INT_8_8_8_8)(GL_UNSIGNED_INT_8_8_8_8_REV)(GL_UNSIGNED_INT_10_10_10_2) \
			(GL_UNSIGNED_INT_2_10_10_10_REV)(GL_UNSIGNED_INT_24_8)(GL_UNSIGNED_INT_10F_11F_11F_REV)(GL_UNSIGNED_INT_5_9_9_9_REV) \
			(GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
	#endif

	struct FmtID {
		uint32_t fmtID, fmtGLID;
		FmtID() = default;
		FmtID(uint32_t fmt, uint32_t fmtgl): fmtID(fmt), fmtGLID(fmtgl) {}
		bool operator == (const FmtID& f) const {
			return ((fmtID ^ f.fmtID) | (fmtGLID ^ f.fmtGLID)) == 0;
		}
		bool operator != (const FmtID& f) const { return !this->operator==(f); }
	};
}
namespace std {
	template <>
	struct hash<rs::FmtID> {
		uint32_t operator()(const rs::FmtID& id) const {
			return (id.fmtID*23 ^ id.fmtGLID*13);
		}
	};
}
namespace rs {
	struct GLFormatDesc {
		GLenum		format,			//!< 定義するOpenGLのピクセルフォーマット
					elementType;	//!< 保存する時の変数Type
		int			numElem;		//!< 1画素に使うElement数
		GLenum		baseType;		//!< BaseFormatにする場合の型
		uint32_t	sdlFormat;		//!< GLFormatと一致するSDLの型(ない場合はUNKNOWN)
		uint32_t	sdlLossFormat;

		std::ostream& print(std::ostream& os) const;
	};
	std::ostream& operator << (std::ostream& os, const GLFormatDesc& desc);
	enum class GLSLType : uint32_t {
		IntT,
		FloatT,
		DoubleT,
		BoolT,
		TextureT,
		IntTextureT,
		MatrixT,
		DMatrixT
	};
	struct GLSLFormatDesc {
		GLSLType	type;
		uint32_t	dim;
		bool		bUnsigned;
		bool		bCubed;

		std::ostream& print(std::ostream& os) const;
	};
	std::ostream& operator << (std::ostream& os, const GLSLFormatDesc& desc);
	class GLFormat {
		public:
			enum ID : uint32_t {
				Internal_Core = 0x01,
				Internal_Sized = 0x02 | Internal_Core,
				Internal_Compressed = 0x04 | Internal_Sized,
				Internal_Render = 0x08,
				Internal_Read = 0x10,
				Depth = 0x20,
				Stencil = 0x40,
				DepthStencil = Depth | Stencil,
				Internal = Internal_Compressed | DepthStencil | Internal_Render | Internal_Read,
				Type = 0x100,
				Query_All = 0x200,
				Query_DSC,
				Query_Info,
				Query_TypeSize,	//!< Typeのバイトサイズ
				Query_GLSLTypeInfo,
				Invalid = 0xffffffff
			};
			using ID_t = std::underlying_type_t<ID>;
			GLenum	value;

		private:
			// uint64_t
			// フォーマット判定: (32bit:種別 32bit:OpenGLフォーマット値) -> ID(種別) 本当は0固定でも良い
			// フォーマット検索: (32bit: Query_??? 32bit:OpenGLフォーマット値) -> ID(種別)
			using IDMap = std::unordered_map<FmtID, boost::variant<ID_t,GLFormatDesc, GLSLFormatDesc>>;
			// SDL_PixelFormatEnum -> OpenGLFormatDesc
			using SDLtoGL = std::unordered_map<uint32_t, const GLFormatDesc&>;
			using GLValueMap = std::unordered_map<GLenum, std::string>;

			static IDMap* s_idMap;
			static SDLtoGL* s_SDLtoGL;
			static GLValueMap* s_valueMap;

		public:
			using OPInfo = spn::Optional<const GLFormatDesc&>;
			using OPGLSLInfo = spn::Optional<const GLSLFormatDesc&>;

			GLFormat() = default;
			GLFormat(const GLFormat& fmt) = default;
			GLFormat(GLenum fmt);
			GLenum get() const;
			static bool Check(GLenum fmt, ID id);
			//! fmtに対応するGLFormat::IDを検索
			static ID QueryFormat(GLenum fmt, ID tag);
			//! Formatに関する様々な情報を引き出す
			static OPInfo QueryInfo(GLenum fmt);
			//! SDLフォーマットからOpenGLフォーマットへの変換
			static OPInfo QuerySDLtoGL(uint32_t fmt);
			//! GLTypeFmtのバイト数
			static size_t QuerySize(GLenum typ);
			//! fmtをtypで保存した場合の1画素のバイト数を計算
			static size_t QueryByteSize(GLenum fmt, GLenum typ);
			//! GLenum値を文字列に変換(デバッグ用)
			static const std::string& QueryEnumString(GLenum value);

			static OPGLSLInfo QueryGLSLInfo(GLenum fmt);

			static void Initialize();
			static void Terminate();
	};
	#define DEF_FMTCHECK(z,data,elem)	template <> struct data<elem> { \
		constexpr static bool valid=true; };
	#define DEF_CHECKER(Name, IDC, Seq)	template <unsigned D=0> struct Name { \
		constexpr static GLFormat::ID id = IDC; \
		constexpr static bool valid=false; static bool check(GLenum fmt) { return GLFormat::Check(fmt, IDC); } }; BOOST_PP_SEQ_FOR_EACH(DEF_FMTCHECK, Name, Seq)

	DEF_CHECKER(DFmtCheck, GLFormat::Depth, PSEQ_DEPTHFORMAT)
	DEF_CHECKER(SFmtCheck, GLFormat::Stencil, PSEQ_STENCILFORMAT)
	DEF_CHECKER(DSFmtCheck, GLFormat::DepthStencil, PSEQ_DSFORMAT)
	DEF_CHECKER(IFmtCheck, GLFormat::Internal, SEQ_INTERNAL)
	DEF_CHECKER(ISFmtCheck, GLFormat::Internal_Sized, SEQ_INTERNALSIZED)
	DEF_CHECKER(ICFmtCheck, GLFormat::Internal_Compressed, SEQ_INTERNALCOMPRESSED)
	DEF_CHECKER(RenderFmtCheck, GLFormat::Internal_Render, SEQ_INTERNALRENDER)
	DEF_CHECKER(ReadFmtCheck, GLFormat::Internal_Read, SEQ_INTERNALREAD)
	DEF_CHECKER(TypeFmtCheck, GLFormat::Type, SEQ_TYPE)

	#undef DEF_CHECKER
	#undef DEF_FMTCHECK

	DEF_NIFTY_INITIALIZER(GLFormat)

	/*! 何らかの有効なフォーマット値が保証されている */
	template <template <unsigned> class Chk>
	class GLFormatBase : private GLFormat {
		friend class GLFormatV;
		GLFormatBase(GLenum fmt, std::nullptr_t): GLFormat(fmt) {}

		public:
			GLFormatBase(GLenum fmt): GLFormat(fmt) {
				// 深度のフォーマットかチェック (デバッグ時)
				AssertP(Trap, Chk<0>::check(fmt));
			}
			GLFormatBase& operator = (GLenum fmt) {
				Chk<0>::check(fmt);
				return *this;
			}
			GLFormatBase& operator = (const GLFormatBase& fmt) {
				value = fmt.value;
				return *this;
			}

			template <unsigned D>
			GLFormatBase(): GLFormat(D) {
				// コンパイル時のフォーマットチェック
				static_assert(Chk<D>::valid, "invalid format");
			}
			GLenum get() const { return value; }
			operator GLenum () const { return value; }
	};

	/*	上位クラスでのGLenumによる初期化はフォーマットのチェックが入る
		通常はテンプレート引数付きの方で初期化する */
	using GLDepthFmt = GLFormatBase<DFmtCheck>;
	using GLStencilFmt = GLFormatBase<SFmtCheck>;
	using GLDSFmt = GLFormatBase<DSFmtCheck>;
	using GLInFmt = GLFormatBase<IFmtCheck>;
	using GLInSizedFmt = GLFormatBase<ISFmtCheck>;
	using GLInCompressedFmt = GLFormatBase<ICFmtCheck>;
	using GLInRenderFmt = GLFormatBase<RenderFmtCheck>;
	using GLInReadFmt = GLFormatBase<ReadFmtCheck>;
	using GLTypeFmt = GLFormatBase<TypeFmtCheck>;
	using OPInSizedFmt = spn::Optional<GLInSizedFmt>;
	using OPInCompressedFmt = spn::Optional<GLInCompressedFmt>;

	using tagGLFormatV = boost::variant<boost::blank, GLDepthFmt, GLStencilFmt, GLDSFmt, GLInFmt, GLInSizedFmt, GLInCompressedFmt, GLInRenderFmt, GLInReadFmt, GLTypeFmt>;
	class GLFormatV : public tagGLFormatV {
		using base_type = tagGLFormatV;
		public:
			GLFormatV(): tagGLFormatV(boost::blank()) {}
			GLFormatV(const GLFormatV& v): tagGLFormatV(static_cast<const tagGLFormatV&>(v)) {}
			GLFormatV(GLFormatV&& v): tagGLFormatV(static_cast<tagGLFormatV&&>(v)) {}
			template <class T>
			GLFormatV(T&& t): tagGLFormatV(std::forward<T>(t)) {}
			GLenum get() const;

			using RetFormatV = std::function<GLFormatV (GLenum)>;
			const static RetFormatV cs_retV[];
			static GLFormatV Detect(GLenum fmt);
	};
}
