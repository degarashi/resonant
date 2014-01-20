#pragma once
#define BOOST_PP_VARIADICS 1
#include <boost/variant.hpp>
#include "spinner/abstbuff.hpp"
#include "spinner/resmgr.hpp"
#include "spinner/dir.hpp"
#include "spinner/size.hpp"
#include "glformat.hpp"
#include "spinner/error.hpp"
#include "sdlwrap.hpp"

namespace rs {
	//! Tech:Pass の組み合わせを表す
	struct GL16ID {
		union {
			uint16_t	value;
			uint8_t		id[2];
		};
		GL16ID() = default;
		GL16ID(int id0, int id1): id{static_cast<uint8_t>(id0),static_cast<uint8_t>(id1)} {}
		bool operator < (const GL16ID& t) const { return value < t.value; }
		bool operator == (const GL16ID& t) const { return value == t.value; }
		operator uint_fast16_t() const { return value; }
	};
	//! ある(Tech:Pass)から別の(Tech:Pass)への遷移を表す
	struct GLDiffID {
		struct Pair { GL16ID fromID, toID; };
		union {
			uint32_t	value;
			Pair		id;
		};
		GLDiffID() = default;
		GLDiffID(GL16ID id0, GL16ID id1): id{id0, id1} {}
		bool operator < (const GLDiffID& t) const { return value < t.value; }
		bool operator == (const GLDiffID& t) const { return value == t.value; }
	};
}
namespace std {
	template <> struct hash<rs::GL16ID> {
		size_t operator() (const rs::GL16ID& id) const {
			return id.value;
		}
	};
	template <> struct hash<rs::GLDiffID> {
		size_t operator() (const rs::GLDiffID& id) const {
			return id.value;
		}
	};
}
namespace rs {
	template <class T>
	class RUser {
		const T& _t;
		public:
			RUser(const T& t): _t(t) {
				_t.use_begin();
			}
			~RUser() {
				_t.use_end();
			}
	};

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

	// ------------------ GLリソース管理 ------------------
	class GLFBufferTmp;
	#define mgr_gl (::rs::GLRes::_ref())
	//! OpenGL関連のリソースマネージャ
	class GLRes : public spn::ResMgrN<UPResource, GLRes, std::allocator, std::string> {
		using base_type = spn::ResMgrN<UPResource, GLRes, std::allocator, std::string>;
		UPFBuffer						_upFb;
		std::unique_ptr<GLFBufferTmp>	_tmpFb;
		//! 空のテクスチャ (何もテクスチャをセットしない事を示す)
		/*! デバッグで色を変えたりしてチェックできる */
		std::unique_ptr<AnotherLHandle<UPTexture>>	_hlEmptyTex;
		//! DeviceLost/Resetの状態管理
		bool	_bInit;
		//! デストラクタ内の時はtrue
		bool	_bInDtor;

		//! 既にデバイスがアクティブだったらonDeviceResetを呼ぶ
		template <class LHDL>
		void initHandle(LHDL& lh) {
			if(_bInit)
				lh.ref()->onDeviceReset();
		}

		public:
			GLRes();
			~GLRes();
			bool deviceStatus() const;
			bool isInDtor() const;
			void onDeviceLost();
			void onDeviceReset();

			//! ベースクラスのacquireメソッドを隠す為のダミー
			void acquire();

			// ------------ Texture ------------
			using HLTex = AnotherLHandle<UPTexture>;
			//! ファイルからテクスチャを読み込む
			/*! 圧縮テクスチャはファイルヘッダで判定
				\param[in] fmt OpenGLの内部フォーマット(not ファイルのフォーマット)<br>
								指定しなければファイルから推定 */
			HLTex loadTexture(const spn::URI& uri, OPInCompressedFmt fmt=spn::none);
			//! 連番ファイルからキューブテクスチャを作成
			HLTex loadCubeTexture(const spn::URI& uri, OPInCompressedFmt fmt=spn::none);
			//! 個別のファイルからキューブテクスチャを作成
			/*! 画像サイズとフォーマットは全て一致していなければならない */
			HLTex loadCubeTexture(const spn::URI& uri0, const spn::URI& uri1, const spn::URI& uri2,
								  const spn::URI& uri3, const spn::URI& uri4, const spn::URI& uri5, OPInCompressedFmt fmt);
			//! 空のテクスチャを作成
			/*! 領域だけ確保 */
			HLTex createTexture(const spn::Size& size, GLInSizedFmt fmt, bool bStream, bool bRestore);
			/*! 用意したデータで初期化 */
			HLTex createTexture(const spn::Size& size, GLInSizedFmt fmt, bool bStream, bool bRestore, GLTypeFmt srcFmt, spn::AB_Byte data);
			//! 共通のデータで初期化
			HLTex createCubeTexture(const spn::Size& size, GLInSizedFmt fmt, bool bRestore, bool bStream, spn::AB_Byte data);
			//! 個別のデータで初期化
			HLTex createCubeTexture(const spn::Size& size, GLInSizedFmt fmt, bool bRestore, bool bStream,
									spn::AB_Byte data0, spn::AB_Byte data1, spn::AB_Byte data2,
									spn::AB_Byte data3, spn::AB_Byte data4, spn::AB_Byte data5);

			// ------------ Shader ------------
			//! 文字列からシェーダーを作成
			AnotherLHandle<UPShader> makeShader(GLuint flag, const std::string& src);

			using HSh = AnotherSHandle<UPShader>;
			using HLProg = AnotherLHandle<UPProg>;
			//! 複数のシェーダーからプログラムを作成 (vertex, geometry, pixel)
			HLProg makeProgram(HSh vsh, HSh gsh, HSh psh);
			//! 複数のシェーダーからプログラムを作成 (vertex, pixel)
			HLProg makeProgram(HSh vsh, HSh psh);

			// ------------ Buffer ------------
			//! ファイルからエフェクトの読み込み
			AnotherLHandle<UPEffect> loadEffect(const spn::URI& uri);
			//! 頂点バッファの確保
			AnotherLHandle<UPVBuffer> makeVBuffer(GLuint dtype);
			//! インデックスバッファの確保
			AnotherLHandle<UPIBuffer> makeIBuffer(GLuint dtype);

			AnotherSHandle<UPTexture> getEmptyTexture() const;
			LHdl _common(const std::string& key, std::function<UPResource()> cb);
			GLFBufferTmp& getTmpFramebuffer() const;
	};
	DEF_HANDLE(GLRes, Res, UPResource)
	DEF_HANDLE(GLRes, Tex, UPTexture)
	DEF_HANDLE(GLRes, Vb, UPVBuffer)
	DEF_HANDLE(GLRes, Ib, UPIBuffer)
	DEF_HANDLE(GLRes, Buff, UPBuffer)
	DEF_HANDLE(GLRes, Prog, UPProg)
	DEF_HANDLE(GLRes, Sh, UPShader)
	DEF_HANDLE(GLRes, Fx, UPEffect)
	DEF_HANDLE(GLRes, Fb, UPFBuffer)
	DEF_HANDLE(GLRes, Rb, UPRBuffer)

	using Priority = uint32_t;
	using Priority64 = uint64_t;
	using PreFunc = std::function<void ()>;
	namespace draw {
		// とりあえず描画ソートの事は考えない
		struct Token {
			HLRes	_hlRes;
			Token(HRes hRes=HRes()): _hlRes(hRes) {}

			virtual void exec() = 0;
		};
		using SPToken = std::shared_ptr<Token>;
		using TokenL = std::vector<SPToken>;

		struct Uniform : Token {
			GLint	idUnif;
			Uniform(HRes hRes, GLint id);
		};
		struct Unif_Int : Uniform {
			int		iValue;

			Unif_Int(GLint id, bool b);
			Unif_Int(GLint id, int iv);
			void exec() override;
		};
		struct Unif_Float : Uniform {
			float	fValue;

			Unif_Float(GLint id, float v);
			void exec() override;
		};
		struct Unif_Vec3 : Uniform {
			spn::Vec3	vValue;

			Unif_Vec3(GLint id, const spn::Vec3& v);
			void exec() override;
		};
		struct Unif_Vec4 : Uniform {
			spn::Vec4	vValue;

			Unif_Vec4(GLint id, const spn::Vec4& v);
			void exec() override;
		};
		struct Unif_Mat33 : Uniform {
			spn::Mat33	mValue;

			Unif_Mat33(GLint id, const spn::Mat33& m);
			void exec() override;
		};
		struct Unif_Mat44 : Uniform {
			spn::Mat44	mValue;

			Unif_Mat44(GLint id, const spn::Mat44& m);
			void exec() override;
		};
	}
	struct IPreFunc {
		virtual void addPreFunc(PreFunc pf) = 0;
	};
	//! OpenGL関連のリソース
	/*! Android用にデバイスロスト対応 */
	struct IGLResource {
		virtual void onDeviceLost() {}
		virtual void onDeviceReset() {}
		virtual ~IGLResource() {}
	};

	//! GLシェーダークラス
	class GLShader : public IGLResource {
		GLuint				_idSh,
							_flag;
		const std::string	_source;
		void _initShader();

		public:
			//! 空シェーダーの初期化
			GLShader();
			GLShader(GLuint flag, const std::string& src);
			~GLShader() override;

			bool isEmpty() const;
			int getShaderID() const;
			void onDeviceLost() override;
			void onDeviceReset() override;
	};

	class GLBufferCore {
		protected:
			GLuint		_buffType,			//!< VERTEX_BUFFERなど
						_drawType,			//!< STATIC_DRAWなどのフラグ
						_stride,			//!< 要素1つのバイトサイズ
						_idBuff;			//!< OpenGLバッファID
		public:
			GLBufferCore(GLuint flag, GLuint dtype);
			GLBufferCore(const GLBufferCore&) = default;

			RUser<GLBufferCore> use() const;
			void use_begin() const;
			void use_end() const;

			GLuint getBuffID() const;
			GLuint getBuffType() const;
			GLuint getStride() const;
	};
	namespace draw {
		class Buffer : public GLBufferCore, public Token {
			public:
				Buffer(const GLBufferCore& core, HRes hRes);
				Buffer(Buffer&& b);
				void operator = (const Buffer&) = delete;

				void exec() override;
		};

		class Program : public Token {
			GLuint		_idProg;
			public:
				Program(HProg hProg);
				Program(Program&& p);

				void exec() override;
		};
	}

	//! OpenGLバッファクラス
	class GLBuffer : public IGLResource, public GLBufferCore {
		PreFunc			_preFunc;
		spn::ByteBuff	_buff;			//!< 再構築の際に必要となるデータ実体

		public:
			using GLBufferCore::GLBufferCore;
			~GLBuffer() override;

			// 全域を書き換え
			void initData(const void* src, size_t nElem, GLuint stride);
			void initData(spn::ByteBuff&& buff, GLuint stride);
			// 部分的に書き換え
			void updateData(const void* src, size_t nElem, GLuint offset);

			void onDeviceLost() override;
			void onDeviceReset() override;
			draw::Buffer getDrawToken(IPreFunc& pf, HRes hRes) const;
	};

	//! 頂点バッファ
	class GLVBuffer : public GLBuffer {
		public:
			GLVBuffer(GLuint dtype);
	};
	//! インデックスバッファ
	class GLIBuffer : public GLBuffer {
		public:
			GLIBuffer(GLuint dtype);
			void initData(const GLubyte* src, size_t nElem);
			void initData(const GLushort* src, size_t nElem);
			void initData(spn::ByteBuff&& buff);
			void initData(const spn::U16Buff& buff);

			void updateData(const GLushort* src, size_t nElem, GLuint offset);
			void updateData(const GLubyte* src, size_t nElem, GLuint offset);
	};

	//! GLSLプログラムクラス
	class GLProgram : public IGLResource {
		HLSh		_shader[ShType::NUM_SHTYPE];
		GLuint		_idProg;

		void _initProgram();

		public:
			GLProgram(HSh vsh, HSh psh);
			GLProgram(HSh vsh, HSh gsh, HSh psh);
			~GLProgram() override;
			void onDeviceLost() override;
			void onDeviceReset() override;
			draw::Program getDrawToken(IPreFunc& pf, HRes hRes) const;
			const HLSh& getShader(ShType type) const;
			int getUniformID(const std::string& name) const;
			int getUniformIDNc(const std::string& name) const;
			int getAttribID(const std::string& name) const;
			int getAttribIDNc(const std::string& name) const;
			GLuint getProgramID() const;
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

	//! OpenGLテクスチャインタフェース
	/*!	フィルターはNEARESTとLINEARしか無いからboolで管理 */
	class IGLTexture : public IGLResource {
		public:
			enum State {
				NoMipmap,
				MipmapNear,
				MipmapLinear
			};
		protected:
			GLuint		_idTex;
			int			_iLinearMag,	//!< Linearの場合は1, Nearestは0
						_iLinearMin,
						_iWrapS,
						_iWrapT;
			GLuint		_actID;		//!< セットしようとしているActiveTextureID (for Use())
			//! [mipLevel][Nearest / Linear]
			const static GLuint cs_Filter[3][2];
			State				_mipLevel;
			GLuint				_texFlag,	//!< TEXTURE_2D or TEXTURE_CUBE_MAP
								_faceFlag;	//!< TEXTURE_2D or TEXTURE_CUBE_MAP_POSITIVE_X
			float				_coeff;
			spn::Size			_size;
			OPInCompressedFmt	_format;	//!< 値が無効 = 不定
			bool				_bReset;
			PreFunc				_preFunc;

			bool _onDeviceReset();
			void _reallocate();
			IGLTexture(OPInCompressedFmt fmt, const spn::Size& sz, bool bCube);
			IGLTexture(IGLTexture& t);

		public:
			IGLTexture(IGLTexture&& t);
			virtual ~IGLTexture();

			void setFilter(State miplevel, bool bLinearMag, bool bLinearMin);
			void setAnisotropicCoeff(float coeff);
			void setUVWrap(GLuint s, GLuint t);

			RUser<IGLTexture> use() const;
			void use_begin() const;
			void use_end() const;

			const spn::Size& getSize() const;
			GLint getTextureID() const;
			const GLInCompressedFmt& getFormat() const;
			GLenum getTexFlag() const;
			GLenum getFaceFlag() const;
			void onDeviceLost() override;
			//! テクスチャユニット番号を指定してBind
			void setActiveID(GLuint n);

			static bool IsMipmap(State level);
			bool isMipmap() const;
			//! 内容をファイルに保存 (主にデバッグ用)
			void save(const std::string& path);

			bool isCubemap() const;
			bool operator == (const IGLTexture& t) const;
			draw::SPToken getDrawToken(IPreFunc& pf, GLint id, HRes hRes) const;
	};
	namespace draw {
		class Texture : public IGLTexture, public Uniform {
			public:
				Texture(HRes hRes, GLint id, IGLTexture& t);
				Texture(const Texture&) = delete;
				virtual ~Texture();

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

		void _init(spn::Optional<T>* dst) {}
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

		bool		_bStream;		//!< 頻繁に書き換えられるか(の、ヒント)
		bool		_bRestore;
		//! テクスチャフォーマットから必要なサイズを計算してバッファを用意する
		const GLFormatDesc& _prepareBuffer();
		public:
			Texture_Mem(bool bCube, GLInSizedFmt fmt, const spn::Size& sz, bool bStream, bool bRestore);
			Texture_Mem(bool bCube, GLInSizedFmt fmt, const spn::Size& sz, bool bStream, bool bRestore, GLTypeFmt srcFmt, spn::AB_Byte buff);
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
			void writeRect(spn::AB_Byte buff, int width, int ofsX, int ofsY, GLTypeFmt srcFmt, CubeFace face=CubeFace::PositiveX);
	};
	//! 画像ファイルから2Dテクスチャを読む
	/*! DeviceReset時:
		再度ファイルから読み出す */
	class Texture_StaticURI : public IGLTexture {
		spn::URI			_uri;
		OPInCompressedFmt	_opFmt;
		public:
			static spn::Size LoadTexture(IGLTexture& tex, HRW hRW, CubeFace face);
			Texture_StaticURI(Texture_StaticURI&& t);
			Texture_StaticURI(const spn::URI& uri, OPInCompressedFmt fmt);
			void onDeviceReset() override;
	};
	//! 連番または6つの画像ファイルからCubeテクスチャを読む
	class Texture_StaticCubeURI : public IGLTexture {
		SPPackURI			_uri;
		OPInCompressedFmt	_opFmt;
		public:
			Texture_StaticCubeURI(Texture_StaticCubeURI&& t);
			Texture_StaticCubeURI(const spn::URI& uri, OPInCompressedFmt fmt);
			Texture_StaticCubeURI(const spn::URI& uri0, const spn::URI& uri1, const spn::URI& uri2,
				const spn::URI& uri3, const spn::URI& uri4, const spn::URI& uri5, OPInCompressedFmt fmt);
			void onDeviceReset() override;
	};

	//! デバッグ用テクスチャ模様生成インタフェース
	struct ITDGen {
		virtual GLenum getFormat() const = 0;
		virtual bool isSingle() const = 0;
		virtual spn::ByteBuff generate(const spn::Size& size, CubeFace face=CubeFace::PositiveX) const = 0;
	};
	using UPTDGen = std::unique_ptr<ITDGen>;
	#define DEF_DEBUGGEN \
		GLenum getFormat() const override; \
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
		int			_nDivW, _nDivH;
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
			Texture_Debug(ITDGen* gen, const spn::Size& size, bool bCube);
			void onDeviceReset() override;
	};

	//! 非ハンドル管理で一時的にFramebufferを使いたい時のヘルパークラス (内部用)
	class GLFBufferTmp {
		GLuint _idFb;

		static void _Attach(GLenum flag, GLuint rb);
		public:
			GLFBufferTmp(GLuint idFb);
			void attachColor(int n, GLuint rb);
			void attachDepth(GLuint rb);
			void attachStencil(GLuint rb);
			void attachDS(GLuint rb);
			GLuint getBufferID() const;

			RUser<GLFBufferTmp> use() const;
			void use_begin() const;
			void use_end() const;
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
			// OpenGL ES2.0だとglDrawPixelsが使えない
		private:
			using F_LOST = std::function<void (GLFBufferTmp&,GLRBuffer&)>;
			const static F_LOST cs_onLost[NUM_ONLOST],
								cs_onReset[NUM_ONLOST];
			GLuint		_idRbo;
			OnLost		_behLost;

			using Res = boost::variant<boost::none_t, spn::Vec4, spn::ByteBuff>;
			Res				_restoreInfo;
			GLTypeFmt		_buffFmt;
			GLFormatV		_fmt;
			int				_width,
							_height;
			//! 現在指定されているサイズとフォーマットでRenderbuffer領域を確保 (内部用)
			void allocate();

		public:
			GLRBuffer(int w, int h, GLInRenderFmt fmt);
			~GLRBuffer();
			void onDeviceReset() override;
			void onDeviceLost() override;

			RUser<GLRBuffer> use() const;
			void use_begin() const;
			void use_end() const;

			void setOnLost(OnLost beh, const spn::Vec4* color=nullptr);
			GLuint getBufferID() const;
			int getWidth() const;
			int getHeight() const;
			const GLFormatV& getFormat() const;
	};

	class GLFBufferCore {
		public:
			// 今はOpenGL ES2 しか考えてないのでCOLOR_ATTACHMENTは0番Only
			enum AttID {
				COLOR0,
				DEPTH,
				STENCIL,
				NUM_ATTACHMENT
			};
			static GLenum _AttIDtoGL(AttID att);
			// attachは受け付けるがハンドルを格納するだけであり、実際OpenGLにセットされるのはDTh
		protected:
			// 内部がTextureかRenderBufferなので、それらを格納できる型を定義
			using Res = boost::variant<boost::none_t, HLTex, HLRb>;
			GLuint	_idFbo;

		public:
			GLFBufferCore(GLuint id);
			GLuint getBufferID() const;

			RUser<GLFBufferCore> use() const;
			void use_begin() const;
			void use_end() const;
	};
	namespace draw {
		// 毎回GLでAttachする
		class FrameBuff : public GLFBufferCore, public Token {
			struct Visitor;
			struct Pair {
				bool	bTex;
				GLuint	idRes;		// 0番は無効
				Res		handle;
			} _ent[NUM_ATTACHMENT];

			public:
				FrameBuff(HRes hRes, GLuint idFb, const Res (&att)[AttID::NUM_ATTACHMENT]);
				FrameBuff(FrameBuff&& f);

				void exec() override;
		};
	}
	//! OpenGL: FrameBufferObjectインタフェース
	class GLFBuffer : public GLFBufferCore, public IGLResource {
		// GLuintは内部処理用 = RenderbufferのID
		Res	_attachment[AttID::NUM_ATTACHMENT] = {boost::none, boost::none, boost::none};

		public:
			GLFBuffer();
			~GLFBuffer();
			void attach(AttID att, HRb hRb);
			void attach(AttID att, HTex hTex);
			void detach(AttID att);

			void onDeviceReset() override;
			void onDeviceLost() override;
			draw::FrameBuff getDrawToken(IPreFunc& pf, HRes hRes) const;
			const Res& getAttachment(AttID att) const;
	};
}
