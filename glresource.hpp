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
	#define GLRESOURCE_INNER \
		public: static struct tagUse{} TagUse; class Inner0; class Inner1; \
						Inner0 use(); \
						void use(tagUse); \
						Inner0 operator -> (); \
						Inner0 operator * ();
	#define DEF_GLRESOURCE_INNER_USING(z,data,elem) using data::elem;
	#define DEF_GLRESOURCE_INNER(base, seq) \
				class base::Inner0 { \
					base::Inner1& _base; \
					public: Inner0(base& tmp): _base(reinterpret_cast<Inner1&>(tmp)) { base::Use(tmp); } \
					~Inner0() { base::End(reinterpret_cast<base&>(_base)); } \
					base::Inner1* operator -> () { return &_base; } }; \
				class base::Inner1 : private base { \
					Inner1() = delete; \
					friend class base; \
					static Inner1& Cast(base* b) { return reinterpret_cast<Inner1&>(*b); } \
					public: BOOST_PP_SEQ_FOR_EACH(DEF_GLRESOURCE_INNER_USING, base, seq) \
					static void end() {} };
	#define DEF_GLRESOURCE_CPP(base) \
		void base::use(tagUse) { Use(*this); } \
		base::Inner0 base::use() { return *this; } \
		base::Inner0 base::operator -> () { return *this; } \
		base::Inner0 base::operator * () { return *this; }

	//! OpenGL関連のリソース
	/*! Android用にデバイスロスト対応 */
	struct IGLResource {
		virtual void onDeviceLost() {}
		virtual void onDeviceReset() {}
		virtual ~IGLResource() {}
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
	//! GLシェーダークラス
	class GLShader : public IGLResource {
		GLuint	_idSh,
				_flag;
		const std::string _source;

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
	//! OpenGLバッファクラス
	class GLBuffer : public IGLResource {
		GLRESOURCE_INNER
		private:
			GLuint		_buffType,			//!< VERTEX_BUFFERなど
						_drawType,			//!< STATIC_DRAWなどのフラグ
						_stride,			//!< 要素1つのバイトサイズ
						_idBuff;			//!< OpenGLバッファID
			spn::ByteBuff	_buff;			//!< 再構築の際に必要となるデータ実体

		protected:
			// 全域を書き換え
			Inner1& initData(const void* src, size_t nElem, GLuint stride);
			Inner1& initData(spn::ByteBuff&& buff, GLuint stride);
			// 部分的に書き換え
			Inner1& updateData(const void* src, size_t nElem, GLuint offset);
			static void Use(GLBuffer& b);
			static void End(GLBuffer& b);

		public:
			GLBuffer(GLuint flag, GLuint dtype);
			~GLBuffer() override;
			void onDeviceLost() override;
			void onDeviceReset() override;

			GLuint getBuffID() const;
			GLuint getBuffType() const;
			GLuint getStride() const;
	};
	DEF_GLRESOURCE_INNER(GLBuffer, (initData)(updateData))
	//! 頂点バッファ
	class GLVBuffer : public GLBuffer {
		public:
			GLVBuffer(GLuint dtype);
	};
	//! インデックスバッファ
	class GLIBuffer : public GLBuffer {
		GLRESOURCE_INNER
		protected:
			Inner1& initData(const GLubyte* src, size_t nElem);
			Inner1& initData(const GLushort* src, size_t nElem);
			Inner1& initData(spn::ByteBuff&& buff);
			Inner1& initData(const spn::U16Buff& buff);

			Inner1& updateData(const GLushort* src, size_t nElem, GLuint offset);
			Inner1& updateData(const GLubyte* src, size_t nElem, GLuint offset);

		public:
			GLIBuffer(GLuint dtype);
	};
	DEF_GLRESOURCE_INNER(GLIBuffer, (initData)(updateData))
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

	DEF_HANDLE(GLRes, Tex, UPTexture)
	DEF_HANDLE(GLRes, Vb, UPVBuffer)
	DEF_HANDLE(GLRes, Ib, UPIBuffer)
	DEF_HANDLE(GLRes, Buff, UPBuffer)
	DEF_HANDLE(GLRes, Prog, UPProg)
	DEF_HANDLE(GLRes, Sh, UPShader)
	DEF_HANDLE(GLRes, Fx, UPEffect)
	DEF_HANDLE(GLRes, Res, UPResource)
	DEF_HANDLE(GLRes, Fb, UPFBuffer)
	DEF_HANDLE(GLRes, Rb, UPRBuffer)

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
		GLRESOURCE_INNER
		public:
			enum State {
				NoMipmap,
				MipmapNear,
				MipmapLinear
			};
		protected:
			GLuint	_idTex;
			int		_iLinearMag,	//!< Linearの場合は1, Nearestは0
					_iLinearMin,
					_iWrapS,
					_iWrapT;
			spn::Size	_size;
			GLuint		_actID;		//!< セットしようとしているActiveTextureID (for Use())

			//! [mipLevel][Nearest / Linear]
			const static GLuint cs_Filter[3][2];

			State				_mipLevel;
			GLuint				_texFlag,	//!< TEXTURE_2D or TEXTURE_CUBE_MAP
								_faceFlag;	//!< TEXTURE_2D or TEXTURE_CUBE_MAP_POSITIVE_X
			OPInCompressedFmt	_format;	//!< 値が無効 = 不定
			float				_coeff;

			bool _onDeviceReset();
			IGLTexture(OPInCompressedFmt fmt, const spn::Size& sz, bool bCube);

			static void Use(IGLTexture& t);
			static void End(IGLTexture& t);

		protected:
			Inner1& setFilter(State miplevel, bool bLinearMag, bool bLinearMin);
			Inner1& setAnisotropicCoeff(float coeff);
			Inner1& setUVWrap(GLuint s, GLuint t);

		public:
			IGLTexture(IGLTexture&& t);
			virtual ~IGLTexture();
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
	};
	DEF_GLRESOURCE_INNER(IGLTexture, (setFilter)(setAnisotropicCoeff)(setUVWrap))

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

	//! 一時的にFramebufferを使いたい時のヘルパークラス
	class GLFBufferTmp {
		GLRESOURCE_INNER
		private:
			GLuint _idFb;
			static void Use(GLFBufferTmp& tmp);
			static void End(GLFBufferTmp& tmp);

			Inner1& _attach(GLenum flag, GLuint rb);

		protected:
			Inner1& attachColor(int n, GLuint rb);
			Inner1& attachDepth(GLuint rb);
			Inner1& attachStencil(GLuint rb);
			Inner1& attachDS(GLuint rb);

		public:
			GLFBufferTmp(GLuint idFb);
	};
	DEF_GLRESOURCE_INNER(GLFBufferTmp, (attachColor)(attachDepth)(attachStencil)(attachDS))

	//! OpenGL: RenderBufferObjectインタフェース
	/*! Use/EndインタフェースはユーザーがRenderbufferに直接何かする事は無いが内部的に使用 */
	class GLRBuffer : public IGLResource {
		GLRESOURCE_INNER
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
			static void Use(GLRBuffer& rb);
			static void End(GLRBuffer& rb);
			//! 現在指定されているサイズとフォーマットでRenderbuffer領域を確保 (内部用)
			Inner1& allocate();

		public:
			GLRBuffer(int w, int h, GLInRenderFmt fmt);
			~GLRBuffer();
			void onDeviceReset() override;
			void onDeviceLost() override;
			void setOnLost(OnLost beh, const spn::Vec4* color=nullptr);
			GLuint getBufferID() const;
			int getWidth() const;
			int getHeight() const;
			const GLFormatV& getFormat() const;
	};
	DEF_GLRESOURCE_INNER(GLRBuffer, NOTHING)

	//! OpenGL: FrameBufferObjectインタフェース
	class GLFBuffer : public IGLResource {
		GLRESOURCE_INNER
		public:
			// 今はOpenGL ES2 しか考えてないのでCOLOR_ATTACHMENTは0番Only
			enum AttID {
				COLOR0,
				DEPTH,
				STENCIL,
				NUM_ATTACHMENT
			};
			static GLenum _AttIDtoGL(AttID att);

		private:
			GLuint	_idFbo;
			// 内部がTextureかRenderBufferなので、それらを格納できる型を定義
			// GLuintは内部処理用 = RenderbufferのID
			using Res = boost::variant<boost::none_t, HLTex, HLRb>;
			Res	_attachment[NUM_ATTACHMENT] = {boost::none, boost::none, boost::none};

			static void Use(GLFBuffer& fb);
			static void End(GLFBuffer& fb);

		protected:
			Inner1& attach(AttID att, HRb hRb);
			Inner1& attach(AttID att, HTex hTex);
			Inner1& detach(AttID att);

		public:
			GLFBuffer();
			~GLFBuffer();
			void onDeviceReset() override;
			void onDeviceLost() override;
			const Res& getAttachment(AttID att) const;
			GLuint getBufferID() const;
	};
	DEF_GLRESOURCE_INNER(GLFBuffer, (attach)(detach))
}
