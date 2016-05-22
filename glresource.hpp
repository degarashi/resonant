#pragma once
#ifdef WIN32
	#include <intrin.h>
#endif
#include <boost/variant.hpp>
#include "spinner/abstbuff.hpp"
#include "spinner/resmgr.hpp"
#include "spinner/dir.hpp"
#include "spinner/size.hpp"
#include "glformat.hpp"
#include "spinner/error.hpp"
#include "spinner/chunk.hpp"
#include "sdlwrap.hpp"
#include <array>
#include "apppath.hpp"
#include "tokenmemory.hpp"
#include "luaimport.hpp"
#include "spinner/structure/wrapper.hpp"

namespace rs {
	//! Tech:Pass の組み合わせを表す
	using GL16Id = std::array<uint8_t, 2>;
	//! ある(Tech:Pass)から別の(Tech:Pass)への遷移を表す
	using GLDiffId = std::array<GL16Id, 2>;
}
namespace std {
	template <> struct hash<::rs::GL16Id> {
		size_t operator() (const ::rs::GL16Id& id) const {
			return (id[1] << 8) | id[0];
		}
	};
	template <> struct hash<::rs::GLDiffId> {
		size_t operator() (const ::rs::GLDiffId& id) const {
			return (hash<::rs::GL16Id>()(id[1]) << 16)
					| (hash<::rs::GL16Id>()(id[0]));
		}
	};
}
namespace rs {
	template <class T>
	class RUser {
		const T&	_t;
		bool		_bRelease;
		public:
			RUser(RUser&& r):
				_t(r._t),
				_bRelease(true)
			{
				r._bRelease = false;
			}
			RUser(const RUser&) = delete;
			RUser(const T& t):
				_t(t),
				_bRelease(true)
			{
				_t.use_begin();
			}
			~RUser() {
				if(_bRelease)
					_t.use_end();
			}
	};

	using OPGLboolean = spn::Optional<GLboolean>;
	using OPGLint = spn::Optional<GLint>;
	using OPGLuint = spn::Optional<GLuint>;
	using OPGLfloat = spn::Optional<GLfloat>;
	#ifndef USE_OPENGLES2
	using OPGLdouble = spn::Optional<GLdouble>;
	#endif

	// ------------------ GL例外クラス ------------------
	using GLGetIV = decltype(&IGL::glGetShaderiv);
	using GLInfoFunc = decltype(&IGL::glGetShaderInfoLog);
	//! GLSLコンパイル関連のエラー基底
	struct GLE_ShProgBase : GLE_Error {
		GLGetIV 	_ivF;
		GLInfoFunc	_infoF;
		GLuint		_id;

		static const char* GetErrorName();
		GLE_ShProgBase(GLGetIV ivF, GLInfoFunc infoF, const std::string& aux, GLuint id);
	};
	//! GLSLシェーダーコンパイルエラー
	struct GLE_ShaderError : GLE_ShProgBase {
		static const char* GetErrorName();
		GLE_ShaderError(const std::string& src, GLuint id);
	};
	//! GLSLプログラムリンクエラー
	struct GLE_ProgramError : GLE_ShProgBase {
		static const char* GetErrorName();
		GLE_ProgramError(GLuint id);
	};
	//! GLSL変数が見つからないエラー
	struct GLE_ParamNotFound : GLE_Error {
		static const char* GetErrorName();
		std::string	_name;
		GLE_ParamNotFound(const std::string& name);
	};
	//! GLSLユーザー変数の型エラー
	struct GLE_InvalidArgument : GLE_Error {
		static const char* GetErrorName();
		std::string	_shName, _argName;
		GLE_InvalidArgument(const std::string& shname, const std::string& argname);
	};
	//! GLXファイルの論理的な記述ミス
	struct GLE_LogicalError : GLE_Error {
		static const char* GetErrorName();
		using GLE_Error::GLE_Error;
	};

	std::shared_ptr<FxBlock> InitFxBlock();
	void ReloadFxBlock(const spn::URI& uri);
	enum MipState {
		NoMipmap,
		MipmapNear,
		MipmapLinear
	};
	enum WrapState {
		ClampToEdge,
		ClampToBorder,
		MirroredRepeat,
		Repeat,
		MirrorClampToEdge,
		_Num
	};
	// ------------------ GLリソース管理 ------------------
	class GLFBufferTmp;
	struct AdaptSDL;
	#define mgr_gl (::rs::GLRes::_ref())
	struct FBInfo {
		GLint		redSize,
					greenSize,
					blueSize,
					alphaSize,
					depthSize,
					stencilSize,
					id;
		bool		bTex;
	};
	using FBInfo_OP = spn::Optional<FBInfo>;
	//! OpenGL関連のリソースマネージャ
	class GLRes : public ResMgrApp<UPResource, GLRes> {
		private:
			using base_type = ResMgrApp<UPResource, GLRes>;
			using Shared_t = GLSharedData<spn::none_t, 0x7fff0000>;
			Shared_t						_sharedObj;

			UPFBuffer						_upFb;
			std::unique_ptr<GLFBufferTmp>	_tmpFb;
			FBInfo_OP						_defaultDepth,
											_defaultColor;
			//! 空のテクスチャ (何もテクスチャをセットしない事を示す)
			/*! デバッグで色を変えたりしてチェックできる */
			std::unique_ptr<HLTex>			_hlEmptyTex;
			//! DeviceLost/Resetの状態管理
			bool	_bInit;
			//! デストラクタ内の時はtrue
			bool	_bInDtor;

			enum ResourceType {
				Texture,
				Effect,
				_Num
			};
			const static std::string cs_rtname[ResourceType::_Num];
			std::function<void (HRes)>	_cbInit;

			//! キューブマップの区別のためのポストフィックス
			spn::Optional<char>	_chPostfix;
			spn::URI _modifyResourceName(spn::URI& key) const override;
			void _initDefaultInfo();
			void _clearDefaultInfo();

		public:
			GLRes();
			~GLRes();
			bool deviceStatus() const;
			bool isInDtor() const;
			void onDeviceLost();
			void onDeviceReset();

			//! ベースクラスのacquireメソッドを隠す為のダミー
			void acquire();

			const std::string& getResourceName(spn::SHandle sh) const override;
			static void LuaExport(LuaState& lsc);
			// ------------ Texture ------------
			//! ファイルからテクスチャを読み込む
			/*! 圧縮テクスチャはファイルヘッダで判定
				\param[in] fmt OpenGLの内部フォーマット(not ファイルのフォーマット)<br>
								指定しなければファイルから推定 */
			HLTex loadTextureUri(const spn::URI& uri, MipState miplevel=NoMipmap, OPInCompressedFmt fmt=spn::none);
			HLTex loadTexture(const std::string& name, MipState miplevel=NoMipmap, OPInCompressedFmt fmt=spn::none);
			//! 連番ファイルからキューブテクスチャを作成
			HLTex loadCubeTextureUri(const spn::URI& uri, MipState miplevel=NoMipmap, OPInCompressedFmt fmt=spn::none);
			HLTex loadCubeTexture(const std::string& name, MipState miplevel=NoMipmap, OPInCompressedFmt fmt=spn::none);
			//! 個別のファイルからキューブテクスチャを作成
			/*! 画像サイズとフォーマットは全て一致していなければならない */
			HLTex _loadCubeTexture(MipState miplevel, OPInCompressedFmt fmt, const spn::URI& uri0, const spn::URI& uri1, const spn::URI& uri2,
								  const spn::URI& uri3, const spn::URI& uri4, const spn::URI& uri5);
			template <class... Ts>
			HLTex loadCubeTextureFromResource(MipState miplevel, OPInCompressedFmt fmt, Ts&&... ts) {
				return loadCubeTexture(miplevel, fmt, _uriFromResourceName(std::forward<Ts>(ts))...);
			}
			HLTex _createTexture(bool bCube, const spn::Size& size, GLInSizedFmt fmt, bool bStream, bool bRestore);
			//! 空のテクスチャを作成
			/*! 領域だけ確保 */
			HLTex createTexture(const spn::Size& size, GLInSizedFmt fmt, bool bStream, bool bRestore);
			/*! 用意したデータで初期化 */
			HLTex createTextureInit(const spn::Size& size, GLInSizedFmt fmt, bool bStream, bool bRestore, GLTypeFmt srcFmt, spn::AB_Byte data);
			//! 空のキューブテクスチャを作成
			HLTex createCubeTexture(const spn::Size& size, GLInSizedFmt fmt, bool bRestore, bool bStream);
			//! 共通のデータで初期化
			HLTex createCubeTextureInit(const spn::Size& size, GLInSizedFmt fmt, bool bRestore, bool bStream, spn::AB_Byte data);
			//! 個別のデータで初期化
			HLTex createCubeTextureInit(const spn::Size& size, GLInSizedFmt fmt, bool bRestore, bool bStream,
									spn::AB_Byte data0, spn::AB_Byte data1, spn::AB_Byte data2,
									spn::AB_Byte data3, spn::AB_Byte data4, spn::AB_Byte data5);

			// ------------ Shader ------------
			//! 文字列からシェーダーを作成
			HLSh makeShader(ShType type, const std::string& src);

			//! 複数のシェーダーからプログラムを作成 (vertex, geometry, fragment)
			HLProg makeProgram(HSh vsh, HSh gsh, HSh fsh);
			//! 複数のシェーダーからプログラムを作成 (vertex, fragment)
			HLProg makeProgram(HSh vsh, HSh fsh);

			// ------------ Buffer ------------
			using CBCreateFx = std::function<IEffect* (const std::string&)>;
			//! ファイルからエフェクトの読み込み
			HLFx loadEffect(const std::string& name, const CBCreateFx& cb);
			//! エフェクトファイルの置き換え
			UPResource replaceEffect(HLFx& fx, const CBCreateFx& cb);
			//! 頂点バッファの確保
			HLVb makeVBuffer(GLuint dtype);
			//! インデックスバッファの確保
			HLIb makeIBuffer(GLuint dtype);

			HTex getEmptyTexture() const;
			LHdl _common(const std::string& key, std::function<UPResource(const spn::URI&)> cb);
			GLFBufferTmp& getTmpFramebuffer() const;
			// --- from ResMgrBase ---
			spn::LHandle loadResource(spn::AdaptStream& ast, const spn::URI& uri) override;

			// ------------ FrameBuffer ------------
			HLFb makeFBuffer();
			// ------------ RenderBuffer ------------
			HLRb makeRBuffer(int w, int h, GLInRenderFmt fmt);

			const FBInfo_OP& getDefaultDepth() const;
			const FBInfo_OP& getDefaultColor() const;

			decltype(std::declval<Shared_t>().lock()) lockGL();
	};

	using Priority = uint32_t;
	using Priority64 = uint64_t;
	namespace draw {
		template <class T>
		struct TokenR : TokenT<T> {
			HLRes	_hlRes;
			TokenR(HRes hRes): _hlRes(hRes) {}
		};
		template <class T>
		struct Uniform : TokenR<T> {
			GLint	idUnif;
			Uniform(HRes hRes, GLint id): TokenR<T>(hRes), idUnif(id) {}
		};
		struct ClearParam {
			spn::Optional<spn::Vec4>	color;
			spn::Optional<float>		depth;
			spn::Optional<uint32_t>		stencil;
		};
		class Clear : public TokenT<Clear> {
			private:
				ClearParam	_param;
			public:
				Clear(const ClearParam& p);
				void exec() override;
		};
		class Viewport : public TokenT<Viewport> {
			private:
				bool		_bPixel;
				spn::RectF	_rect;
			public:
				Viewport(bool bPixel, const spn::RectF& r);
				void exec() override;
		};

		void Unif_Vec_Exec(int idx, GLint id, const void* ptr, int n);
		void Unif_Mat_Exec(int idx, GLint id, const void* ptr, int n, bool bT);
		//! ivec[1-4], fvec[1-4]対応
		template <class T, int DN>
		class Unif_Vec : public Uniform<Unif_Vec<T,DN>> {
			protected:
				using base_t = Uniform<Unif_Vec<T,DN>>;
				using SPT = std::shared_ptr<T>;
				SPT		_data;
				int		_nAr;

			public:
				template <bool A>
				Unif_Vec(GLint id, const spn::VecT<DN,A>& v): Unif_Vec(id, v.m, DN, 1) {}
				template <bool A>
				Unif_Vec(GLint id, const spn::VecT<DN,A>* vp, int n): Unif_Vec(id, vp->m, DN, n) {}
				Unif_Vec(GLint id, const T* v, int nElem, int n):
					base_t(HRes(), id),
					_data(new T[nElem*n]),
					_nAr(n)
				{
					std::memcpy(_data.get(), v, sizeof(T)*nElem*n);
				}
				void exec() override {
					Unif_Vec_Exec(std::is_integral<T>::value * 4 + DN-1, base_t::idUnif, _data.get(), _nAr);
				}
		};
		template <class T, int DN>
		class Unif_Mat : public Unif_Vec<T,DN> {
			private:
				using base_t = Unif_Vec<T,DN>;
				bool	_bT;
			public:
				template <bool A>
				Unif_Mat(GLint id, const spn::MatT<DN,DN,A>& m, bool bT): Unif_Mat(id, &m, 1, bT) {}
				template <bool A>
				Unif_Mat(GLint id, const spn::MatT<DN,DN,A>* mp, int n, bool bT):
					base_t(id, mp->data, DN*DN, n), _bT(bT) {}
				void exec() override {
					Unif_Mat_Exec(DN-2, base_t::idUnif, base_t::_data.get(), base_t::_nAr, _bT);
				}
				void clone(TokenDst& dst) const override {
					new(dst.allocate_memory(getSize(), draw::CalcTokenOffset<Unif_Mat>())) Unif_Mat(*this);
				}
				void takeout(TokenDst& dst) override {
					new(dst.allocate_memory(getSize(), draw::CalcTokenOffset<Unif_Mat>())) Unif_Mat(std::move(*this));
				}
				std::size_t getSize() const override {
					return sizeof(*this);
				}
		};
	}
	//! OpenGL関連のリソース
	/*! Android用にデバイスロスト対応 */
	struct IGLResource : spn::EnableFromThis<HRes> {
		virtual void onDeviceLost() {}
		virtual void onDeviceReset() {}
		virtual ~IGLResource() {}
		virtual const std::string& getResourceName() const;
	};

	//! GLシェーダークラス
	class GLShader : public IGLResource {
		GLuint				_idSh;
		ShType				_flag;
		const std::string	_source;
		void _initShader();

		public:
			//! 空シェーダーの初期化
			GLShader();
			GLShader(ShType flag, const std::string& src);
			~GLShader() override;
			bool isEmpty() const;
			int getShaderID() const;
			ShType getShaderType() const;
			void onDeviceLost() override;
			void onDeviceReset() override;
	};

	namespace draw {
		class Buffer;
		class VStream;
	}
	class GLBufferCore {
		private:
			friend class RUser<GLBufferCore>;
			friend class draw::Buffer;
			friend class draw::VStream;
			void use_begin() const;
			void use_end() const;
		protected:
			GLuint		_buffType,			//!< VERTEX_BUFFERなど
						_drawType,			//!< STATIC_DRAWなどのフラグ
						_stride,			//!< 要素1つのバイトサイズ
						_idBuff;			//!< OpenGLバッファID
		public:
			GLBufferCore(GLuint flag, GLuint dtype);
			virtual ~GLBufferCore() {}

			RUser<GLBufferCore> use() const;

			GLuint getBuffID() const;
			GLuint getBuffType() const;
			GLuint getStride() const;
	};
	namespace draw {
		class Buffer : public GLBufferCore, public TokenR<Buffer> {
			public:
				Buffer(const GLBufferCore& core, HRes hRes);
				void exec() override;
		};
		class Program : public TokenR<Program> {
			GLuint		_idProg;
			public:
				Program(HRes hRes, GLuint idProg);
				void exec() override;
		};
	}

	/*	getDrawTokenの役割:
		描画時にしか必要ないAPI呼び出しを纏める
		ただしDrawThreadからはリソースハンドルの参照が出来ないのでOpenGLの番号をそのまま格納 */
	template <class T>
	struct is_vector { constexpr static int value = 0; };
	template <class T>
	struct is_vector<std::vector<T>> { constexpr static int value = 1; };
	//! OpenGLバッファクラス
	class GLBuffer : public IGLResource, public GLBufferCore {
		using SPBuff = std::shared_ptr<void>;
		SPBuff			_buff;			//!< 再構築の際に必要となるデータ実体(std::vector<T>)
		void*			_pBuffer;		//!< bufferの先頭ポインタ
		GLuint			_buffSize;		//!< bufferのバイトサイズ
		void	_initData();

		template <class T>
		using Raw = typename std::decay<T>::type;
		template <class T>
		using ChkIfVector = typename std::enable_if<is_vector<Raw<T>>::value>::type;
		template <class T, class = ChkIfVector<T>>
		void _setVec(T&& src, GLuint stride) {
			using RawType = Raw<T>;
			auto fnDeleter = [](void* ptr) {
				auto* rt = reinterpret_cast<RawType*>(ptr);
				delete rt;
			};
			_stride = stride;
			_buffSize = src.size() * sizeof(typename Raw<T>::value_type);
			RawType* rt = new RawType(std::forward<T>(src));
			_pBuffer = rt->data();
			_buff = SPBuff(static_cast<void*>(rt), fnDeleter);
		}

		public:
			GLBuffer(GLuint flag, GLuint dtype);
			GLBuffer(const GLBuffer&) = delete;
			~GLBuffer() override;

			// 全域を書き換え
			void initData(const void* src, size_t nElem, GLuint stride);
			template <class T, class = ChkIfVector<T>>
			void initData(T&& src, GLuint stride=sizeof(typename Raw<T>::value_type)) {
				_setVec(std::forward<T>(src), stride);
				_initData();
			}
			// 部分的に書き換え
			void updateData(const void* src, size_t nElem, GLuint offset);
			GLuint getSize() const;
			GLuint getNElem() const;

			void onDeviceLost() override;
			void onDeviceReset() override;
			draw::Buffer getDrawToken() const;
	};
	// バッファのDrawTokenはVDeclとの兼ね合いからそのままリストに積まずに
	// StreamTagで一旦処理するのでスマートポインタではなく直接出力する

	//! 頂点バッファ
	class GLVBuffer : public GLBuffer {
		public:
			GLVBuffer(GLuint dtype);
	};
	//! インデックスバッファ
	class GLIBuffer : public GLBuffer {
		public:
			GLIBuffer(GLuint dtype);
			using GLBuffer::initData;
			void initData(const GLubyte* src, size_t nElem);
			void initData(const GLushort* src, size_t nElem);

			void updateData(const GLushort* src, size_t nElem, GLuint offset);
			void updateData(const GLubyte* src, size_t nElem, GLuint offset);
			GLenum getSizeFlag() const;
			static GLenum GetSizeFlag(int stride);
	};

	struct GLParamInfo : GLSLFormatDesc {
		std::string name;
		GLParamInfo() = default;
		GLParamInfo(const GLParamInfo&) = default;
		GLParamInfo(const GLSLFormatDesc& desc);
	};
	//! GLSLプログラムクラス
	class GLProgram : public IGLResource {
		HLSh		_shader[ShType::NUM_SHTYPE];
		GLuint		_idProg;

		void _initProgram();
		using InfoF = void (IGL::*)(GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*);
		GLParamInfo _getActiveParam(int n, InfoF infoF) const;
		int _getNumParam(GLenum flag) const;
		void _setShader(HSh hSh);

		void _init() {
			_initProgram();
		}
		template <class T, class... Ts>
		void _init(const T& t, const Ts&... ts) {
			_setShader(t);
			_init(ts...);
		}

		public:
			template <class... Ts>
			GLProgram(const Ts&... ts) {
				_init(ts...);
			}
			~GLProgram() override;
			void onDeviceLost() override;
			void onDeviceReset() override;
			void getDrawToken(draw::TokenDst& dst) const;
			const HLSh& getShader(ShType type) const;
			OPGLint getUniformID(const std::string& name) const;
			OPGLint getAttribID(const std::string& name) const;
			GLuint getProgramID() const;
			int getNActiveAttribute() const;
			int getNActiveUniform() const;
			GLParamInfo getActiveAttribute(int n) const;
			GLParamInfo getActiveUniform(int n) const;
			void use() const;
	};
	enum class CubeFace {
		PositiveX,
		NegativeX,
		PositiveY,
		NegativeY,
		PositiveZ,
		NegativeZ,
		Num
	};

	namespace draw {
		class Texture;
		class TextureA;
	}
	//! OpenGLテクスチャインタフェース
	/*!	フィルターはNEARESTとLINEARしか無いからboolで管理 */
	class IGLTexture : public IGLResource {
		public:
			friend class RUser<IGLTexture>;
			friend class draw::Texture;
			friend class draw::TextureA;

			void use_begin() const;
			void use_end() const;
		protected:
			GLuint		_idTex;
			int			_iLinearMag,	//!< Linearの場合は1, Nearestは0
						_iLinearMin;
			WrapState	_wrapS,
						_wrapT;
			GLuint		_actID;		//!< セットしようとしているActiveTextureID (for Use())
			//! [mipLevel][Nearest / Linear]
			const static GLuint cs_Filter[3][2];
			const MipState		_mipLevel;
			GLuint				_texFlag,	//!< TEXTURE_2D or TEXTURE_CUBE_MAP
								_faceFlag;	//!< TEXTURE_2D or TEXTURE_CUBE_MAP_POSITIVE_X
			float				_coeff;
			spn::Size			_size;
			OPInCompressedFmt	_format;	//!< 値が無効 = 不定

			bool _onDeviceReset();
			IGLTexture(MipState miplevel, OPInCompressedFmt fmt, const spn::Size& sz, bool bCube);
			IGLTexture(const IGLTexture& t);

		public:
			IGLTexture(IGLTexture&& t);
			virtual ~IGLTexture();

			void setFilter(bool bLinearMag, bool bLinearMin);
			void setLinear(bool bLinear);
			void setAnisotropicCoeff(float coeff);
			void setUVWrap(WrapState s, WrapState t);
			void setWrap(WrapState st);
			const std::string& getResourceName() const override;

			RUser<IGLTexture> use() const;

			const spn::Size& getSize() const;
			GLint getTextureID() const;
			const OPInCompressedFmt& getFormat() const;
			GLenum getTexFlag() const;
			GLenum getFaceFlag(CubeFace face=CubeFace::PositiveX) const;
			void onDeviceLost() override;
			//! テクスチャユニット番号を指定してBind
			void setActiveID(GLuint n);

			static bool IsMipmap(MipState level);
			bool isMipmap() const;
			//! 内容をファイルに保存 (主にデバッグ用)
			void save(const std::string& path, CubeFace face=CubeFace::PositiveX);

			bool isCubemap() const;
			bool operator == (const IGLTexture& t) const;
			spn::ByteBuff readData(GLInFmt internalFmt, GLTypeFmt elem, int level=0, CubeFace face=CubeFace::PositiveX) const;
			spn::ByteBuff readRect(GLInFmt internalFmt, GLTypeFmt elem, const spn::Rect& rect, CubeFace face=CubeFace::PositiveX) const;
			/*! \param[in] uniform変数の番号
				\param[in] index idで示されるuniform変数配列のインデックス(デフォルト=0)
				\param[in] hRes 自身のリソースハンドル */
			void getDrawToken(draw::TokenDst& dst, GLint id, int index, int actID);
	};
	namespace draw {
		// 連番テクスチャはUniformID + Indexとして設定
		class Texture : public IGLTexture, public Uniform<Texture> {
			public:
				Texture(HRes hRes, GLint id, int index, int baseActID, const IGLTexture& t);
				virtual ~Texture();
				void exec() override;
		};
		class TextureA : public Uniform<TextureA> {
			private:
				using TexA = std::vector<Texture>;
				TexA	_texA;
			public:
				TextureA(GLint id, const HRes* hRes, const IGLTexture** tp, int baseActID, int nTex);
				void exec() override;
		};
	}

	template <class T>
	struct IOPArray {
		virtual int getNPacked() const = 0;
		virtual const T& getPacked(int n) const = 0;
		virtual ~IOPArray() {}
		virtual uint32_t getID() const = 0;
		virtual bool operator == (const IOPArray& p) const = 0;
	};
	template <class T, int N>
	class OPArray : public IOPArray<T> {
		spn::Optional<T>	_packed[N];

		void _init(spn::Optional<T>* /*dst*/) {}
		template <class TA, class... Ts>
		void _init(spn::Optional<T>* dst, TA&& ta, Ts&&... ts) {
			*dst++ = std::forward<TA>(ta);
			_init(dst, std::forward<Ts>(ts)...);
		}
		public:
			template <class... Ts>
			OPArray(Ts&&... ts) {
				static_assert(sizeof...(Ts)==N, "invalid number of argument(s)");
				_init(_packed, std::forward<Ts>(ts)...);
			}
			int getNPacked() const override { return N; }
			const T& getPacked(int n) const override {
				Assert(Trap, n<=N)
				return *_packed[n];
			}
			bool operator == (const IOPArray<T>& p) const override {
				if(getID()==p.getID() && getNPacked()==p.getNPacked()) {
					auto& p2 = reinterpret_cast<const OPArray&>(p);
					for(int i=0 ; i<N ; i++) {
						if(_packed[i] != p2._packed[i])
							return false;
					}
					return true;
				}
				return false;
			}
			uint32_t getID() const override { return spn::MakeChunk('P','a','c','k'); }
	};
	using SPPackURI = std::shared_ptr<IOPArray<spn::URI>>;
	//! ユーザー定義の空テクスチャ
	/*! DeviceLost時の復元は任意
		内部バッファはDeviceLost用であり、DeviceがActiveな時はnone
		フォーマット変換は全てOpenGLにさせる
		書き込み不可の時は最初の一度だけ書き込める */
	class Texture_Mem : public IGLTexture {
		using OPBuff = spn::Optional<spn::ByteBuff>;
		using OPFormat = spn::Optional<GLTypeFmt>;
		OPBuff		_buff;			//!< DeviceLost時用のバッファ
		OPFormat	_typeFormat;	//!< _buffに格納されているデータの形式(Type)

		//bool		_bStream;		//!< 頻繁に書き換えられるか(の、ヒント)
		bool		_bRestore;
		//! テクスチャフォーマットから必要なサイズを計算してバッファを用意する
		const GLFormatDesc& _prepareBuffer();
		public:
			Texture_Mem(bool bCube, GLInSizedFmt fmt, const spn::Size& sz, bool bStream, bool bRestore);
			void onDeviceReset() override;
			void onDeviceLost() override;
			//! テクスチャ全部書き換え = バッファも置き換え
			/*! \param[in] fmt テクスチャのフォーマット
				\param[in] srcFmt 入力フォーマット(Type)
				\param[in] bRestore trueなら内部バッファにコピーを持っておいてDeviceLostに備える
				\param[in] face Cubemapにおける面 */
			void writeData(spn::AB_Byte buff, GLTypeFmt srcFmt, CubeFace face=CubeFace::PositiveX);
			//! 部分的に書き込み
			/*! \param[in] ofsX 書き込み先オフセット X
				\param[in] ofsY 書き込み先オフセット Y
				\param[in] srcFmt 入力フォーマット(Type)
				\param[in] face Cubemapにおける面 */
			void writeRect(spn::AB_Byte buff, const spn::Rect& rect, GLTypeFmt srcFmt, CubeFace face=CubeFace::PositiveX);
	};
	//! 画像ファイルから2Dテクスチャを読む
	/*! DeviceReset時:
		再度ファイルから読み出す */
	class Texture_StaticURI : public IGLTexture {
		spn::URI			_uri;
		OPInCompressedFmt	_opFmt;
		public:
			static std::pair<spn::Size, GLInCompressedFmt> LoadTexture(IGLTexture& tex, HRW hRW, CubeFace face);
			Texture_StaticURI(Texture_StaticURI&& t);
			Texture_StaticURI(const spn::URI& uri, MipState miplevel, OPInCompressedFmt fmt);
			void onDeviceReset() override;
	};
	//! 連番または6つの画像ファイルからCubeテクスチャを読む
	class Texture_StaticCubeURI : public IGLTexture {
		SPPackURI			_uri;
		OPInCompressedFmt	_opFmt;
		public:
			Texture_StaticCubeURI(Texture_StaticCubeURI&& t);
			Texture_StaticCubeURI(const spn::URI& uri, MipState miplevel, OPInCompressedFmt fmt);
			Texture_StaticCubeURI(const spn::URI& uri0, const spn::URI& uri1, const spn::URI& uri2,
				const spn::URI& uri3, const spn::URI& uri4, const spn::URI& uri5, MipState miplevel, OPInCompressedFmt fmt);
			void onDeviceReset() override;
	};

	//! デバッグ用テクスチャ模様生成インタフェース
	struct ITDGen {
		virtual ~ITDGen() {}
		virtual uint32_t getFormat() const = 0;
		virtual bool isSingle() const = 0;
		virtual spn::ByteBuff generate(const spn::Size& size, CubeFace face=CubeFace::PositiveX) const = 0;
	};
	using UPTDGen = std::unique_ptr<ITDGen>;
	#define DEF_DEBUGGEN \
		uint32_t getFormat() const override; \
		bool isSingle() const override; \
		spn::ByteBuff generate(const spn::Size& size, CubeFace face) const override;

	//! 2色チェッカー
	class TDChecker : public ITDGen {
		spn::Vec4	_col[2];
		int			_nDivW, _nDivH;
		public:
			TDChecker(const spn::Vec4& col0, const spn::Vec4& col1, int nDivW, int nDivH);
			DEF_DEBUGGEN
	};
	//! カラーチェッカー
	/*! 準モンテカルロで色を決定 */
	class TDCChecker : public ITDGen {
		// int			_nDivW, _nDivH;
		public:
			TDCChecker(int nDivW, int nDivH);
			DEF_DEBUGGEN
	};
	//! ベタ地と1テクセル枠
	class TDBorder : public ITDGen {
		public:
			TDBorder(const spn::Vec4& col, const spn::Vec4& bcol);
			DEF_DEBUGGEN
	};

	//! デバッグ用のテクスチャ
	/*! DeviceLost時:
		再度生成し直す */
	class Texture_Debug : public IGLTexture {
		// デバッグ用なので他との共有を考えず、UniquePtrとする
		UPTDGen		_gen;
		public:
			Texture_Debug(ITDGen* gen, const spn::Size& size, bool bCube, MipState miplevel);
			void onDeviceReset() override;
	};

	//! OpenGL: RenderBufferObjectインタフェース
	/*! Use/EndインタフェースはユーザーがRenderbufferに直接何かする事は無いが内部的に使用 */
	class GLRBuffer : public IGLResource {
		public:
			//! DeviceLost時の挙動
			enum OnLost {
				NONE,		//!< 何もしない(領域はゴミデータになる)
				CLEAR,		//!< 単色でクリア
				RESTORE,	//!< 事前に保存しておいた内容で復元
				NUM_ONLOST
			};
			friend class RUser<GLRBuffer>;
			void use_begin() const;
			void use_end() const;
			// OpenGL ES2.0だとglDrawPixelsが使えない
		private:
			using F_LOST = std::function<void (GLFBufferTmp&,GLRBuffer&)>;
			const static F_LOST cs_onLost[NUM_ONLOST],
								cs_onReset[NUM_ONLOST];
			GLuint		_idRbo;
			OnLost		_behLost;

			using Res = boost::variant<boost::blank, spn::Vec4, spn::ByteBuff>;
			Res				_restoreInfo;
			GLTypeFmt		_buffFmt;
			GLFormatV		_fmt;
			spn::Size		_size;
			//! 現在指定されているサイズとフォーマットでRenderbuffer領域を確保 (内部用)
			void allocate();

		public:
			GLRBuffer(int w, int h, GLInRenderFmt fmt);
			~GLRBuffer();
			void onDeviceReset() override;
			void onDeviceLost() override;

			RUser<GLRBuffer> use() const;

			void setOnLost(OnLost beh, const spn::Vec4* color=nullptr);
			GLuint getBufferID() const;
			const spn::Size& getSize() const;
			const GLFormatV& getFormat() const;
			const std::string& getResourceName() const override;
	};

	class GLFBufferCore {
		public:
			friend class RUser<GLFBufferCore>;
			void use_begin() const;
			void use_end() const;
			static thread_local spn::Size s_fbSize;

			struct Att {
				enum Id {
					COLOR0,
					#ifndef USE_OPENGLES2
						COLOR1,
						COLOR2,
						COLOR3,
					#endif
					DEPTH,
					STENCIL,
					NUM_ATTACHMENT,
					DEPTH_STENCIL = 0xffff
				};
			};
			static FBInfo GetCurrentInfo(Att::Id att);
			static GLenum _AttIDtoGL(Att::Id att);
			void _attachRenderbuffer(Att::Id aId, GLuint rb);
			void _attachCubeTexture(Att::Id aId, GLuint faceFlag, GLuint tb);
			void _attachTexture(Att::Id aId, GLuint tb);
			using TexRes = std::pair<HLTex, CubeFace>;
			struct RawTex : spn::Wrapper<GLuint> {
				using Wrapper::Wrapper;
			};
			struct RawRb : spn::Wrapper<GLuint> {
				using Wrapper::Wrapper;
			};
			// attachは受け付けるがハンドルを格納するだけであり、実際OpenGLにセットされるのはDTh
		protected:
			// 内部がTextureかRenderBufferなので、それらを格納できる型を定義
			using Res = boost::variant<boost::blank, RawTex, RawRb, TexRes, HLRb>;
			GLuint	_idFbo;
			static void _SetCurrentFBSize(const spn::Size& s);

		public:
			static const spn::Size& GetCurrentFBSize();
			GLFBufferCore(GLuint id);
			GLuint getBufferID() const;

			RUser<GLFBufferCore> use() const;
	};

	namespace draw {
		// 毎回GLでAttachする
		class FrameBuff : public GLFBufferCore, public TokenR<FrameBuff> {
			struct Visitor;
			struct Pair {
				bool	bTex;
				Res		handle;
				GLuint	idRes,		// 0番は無効
						faceFlag;
			} _ent[Att::NUM_ATTACHMENT];
			const spn::Size	_size;	//!< Colo0バッファのサイズ

			public:
				// from GLFBufferTmp
				FrameBuff(GLuint idFb, const spn::Size& s);
				// from GLFBuffer
				FrameBuff(HRes hRes, GLuint idFb, const Res (&att)[Att::NUM_ATTACHMENT]);

				void exec() override;
		};
	}
	//! 非ハンドル管理で一時的にFramebufferを使いたい時のヘルパークラス (内部用)
	class GLFBufferTmp : public GLFBufferCore {
		private:
			static void _Attach(GLenum flag, GLuint rb);
			const spn::Size	_size;
		public:
			GLFBufferTmp(GLuint idFb, const spn::Size& s);
			void attachRBuffer(Att::Id att, GLuint rb);
			void attachTexture(Att::Id att, GLuint id);
			void attachCubeTexture(Att::Id att, GLuint id, GLuint face);
			void use_end() const;

			void getDrawToken(draw::TokenDst& dst) const;
			RUser<GLFBufferTmp> use() const;
	};
	using Size_OP = spn::Optional<spn::Size>;
	//! OpenGL: FrameBufferObjectインタフェース
	class GLFBuffer : public GLFBufferCore, public IGLResource {
		// GLuintは内部処理用 = RenderbufferのID
		Res	_attachment[Att::NUM_ATTACHMENT];
		template <class T>
		void _attachIt(Att::Id att, const T& arg);

		public:
			static Size_OP GetAttachmentSize(const Res (&att)[Att::NUM_ATTACHMENT], Att::Id id);
			static void LuaExport(LuaState& lsc);
			GLFBuffer();
			~GLFBuffer();
			void attachRBuffer(Att::Id att, HRb hRb);
			void attachTexture(Att::Id att, HTex hTex);
			void attachTextureFace(Att::Id att, HTex hTex, CubeFace face);
			void attachRawRBuffer(Att::Id att, GLuint idRb);
			void attachRawTexture(Att::Id att, GLuint idTex);
			void attachOther(Att::Id attDst, Att::Id attSrc, HFb hFb);
			void detach(Att::Id att);

			void onDeviceReset() override;
			void onDeviceLost() override;
			void getDrawToken(draw::TokenDst& dst) const;
			const Res& getAttachment(Att::Id att) const;
			HTex getAttachmentAsTexture(Att::Id id) const;
			HRb getAttachmentAsRBuffer(Att::Id id) const;
			Size_OP getAttachmentSize(Att::Id att) const;
			const std::string& getResourceName() const override;
	};
}
DEF_LUAIMPORT(rs::GLRes)
DEF_LUAIMPORT(rs::GLRBuffer)
DEF_LUAIMPORT(rs::GLFBuffer)
DEF_LUAIMPORT(rs::IGLTexture)
