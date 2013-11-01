#pragma once
#define BOOST_PP_VARIADICS 1
#include <boost/variant.hpp>
#include "spinner/abstbuff.hpp"
#include "spinner/resmgr.hpp"
#include "spinner/dir.hpp"
#include "spinner/size.hpp"
#include "glformat.hpp"

#define SDLEC_Base(act, ...)	EChk_base<act, SDLError>(__FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define SDLEC_Base0(act)		EChk_base<act, SDLError>(__FILE__, __PRETTY_FUNCTION__, __LINE__);
#define SDLEC(act, ...)			SDLEC_Base(AAct_##act<std::runtime_error>, __VA_ARGS__)
#define SDLEC_Chk(act)			SDLEC_Base0(AAct_##act<std::runtime_error>)
#ifdef DEBUG
	#define SDLEC_P(act, ...)	SDLEC(act, __VA_ARGS__)
#else
	#define SDLEC_P(act, ...)	EChk_pass(__VA_ARGS__)
#endif

// OpenGLに関するアサート集
#define GLEC_Base(act, ...)		EChk_base<GLError>(act, __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define GLEC_Base0(act)			EChk_base<GLError>(act, __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define GLEC(act, ...)			GLEC_Base(AAct_##act<GLE_Error>(), __VA_ARGS__)
#define GLECProg(act, id, ...)	GLEC_Base(AAct_##act<GLE_ProgramError, GLuint>(id), __VA_ARGS__)
#define GLECSh(act, id, ...)	GLEC_Base(AAct_##act<GLE_ShaderError, GLuint>(id), __VA_ARGS__)
#define GLECParam(act, name, ...)	GLEC_Base(AAct_##act<GLE_ParamNotFound, const char*>(name), __VA_ARGS__)
#define GLECArg(act, shname, argname, ...)	GLEC_Base(AAct_##act<GLE_InvalidArgument, const char*, const char*>(shname, argname), __VA_ARGS__)
#define GLECLog(act, ...)		GLEC_Base(AAct_##act<GLE_LogicalError>(), __VA_ARGS__)

#define GLEC_Chk(act)			GLEC_Base0(AAct_##act<GLE_Error>())
#ifdef DEBUG
	#define GLEC_P(act, ...)	GLEC(act, __VA_ARGS__)
#else
	#define GLEC_P(act, ...)	EChk_pass(__VA_ARGS__)
#endif

namespace rs {
	using PathStr = spn::Dir::StrType;
	//! Tech:Pass の組み合わせを表す
	struct GL16ID {
		union {
			uint16_t	value;
			uint8_t		id[2];
		};
		GL16ID() {}
		GL16ID(int id0, int id1): id{static_cast<uint8_t>(id0),static_cast<uint8_t>(id1)} {}
		bool operator < (const GL16ID& t) const { return value < t.value; }
		bool operator == (const GL16ID& t) const { return value == t.value; }
		operator uint_fast16_t() const { return value; }
	};
	//! ある(Tech:Pass)から別の(Tech:Pass)への遷移を表す
	struct GLDiffID {
		union {
			uint32_t		value;
			struct {
				GL16ID		fromID, toID;
			};
		};
		GLDiffID() {}
		GLDiffID(GL16ID id0, GL16ID id1): fromID(id0), toID(id1) {}
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
	//! OpenGLに関する全般的なエラー
	struct GLE_Error : std::runtime_error {
		using runtime_error::runtime_error;
	};
	using GLGetIV = decltype(glGetShaderiv);
	using GLInfoFunc = decltype(glGetShaderInfoLog);
	//! GLSLコンパイル関連のエラー基底
	struct GLE_ShProgBase : GLE_Error {
		GLE_ShProgBase(GLGetIV ivF, GLInfoFunc infoF, const std::string& aux, GLuint id);
	};
	//! GLSLシェーダーコンパイルエラー
	struct GLE_ShaderError : GLE_ShProgBase {
		GLE_ShaderError(GLuint id);
	};
	//! GLSLプログラムリンクエラー
	struct GLE_ProgramError : GLE_ShProgBase {
		GLE_ProgramError(GLuint id);
	};
	//! GLSL変数が見つからないエラー
	struct GLE_ParamNotFound : GLE_Error {
		GLE_ParamNotFound(const std::string& name);
	};
	//! GLSLユーザー変数の型エラー
	struct GLE_InvalidArgument : GLE_Error {
		GLE_InvalidArgument(const std::string& shname, const std::string& argname);
	};
	//! GLXファイルの論理的な記述ミス
	struct GLE_LogicalError : GLE_Error {
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
	//! OpenGLエラーIDとその詳細メッセージ
	struct GLError {
		const static std::pair<GLenum, const char*> ErrorList[];

		static std::string s_tmp;
		static const char* ErrorDesc();
		static const char* GetAPIName();
		//! エラー値を出力しなくなるまでループする
		static void ResetError();
	};
	class GLFBufferTmp;
	#define mgr_gl rs::GLRes::_ref()
	//! OpenGL関連のリソースマネージャ
	class GLRes : public spn::ResMgrN<UPResource, GLRes> {
		using base_type = spn::ResMgrN<UPResource, GLRes>;
		UPFBuffer						_upFb;
		std::unique_ptr<GLFBufferTmp>	_tmpFb;
		//! 空のテクスチャ (何もテクスチャをセットしない事を示す)
		/*! デバッグで色を変えたりしてチェックできる */
		std::unique_ptr<AnotherLHandle<UPTexture>>	_hlEmptyTex;
		//! DeviceLost/Resetの状態管理
		bool	_bInit;

		template <class LHDL>
		void initHandle(LHDL& lh) {
			if(_bInit)
				lh.ref()->onDeviceReset();
		}

		public:
			GLRes();
			~GLRes();
			bool deviceStatus() const;
			void onDeviceLost();
			void onDeviceReset();

			//! ベースクラスのacquireメソッドを隠す為のダミー
			void acquire();

			// ------------ Texture ------------
			//! ファイルからテクスチャを読み込む
			AnotherLHandle<UPTexture> loadTexture(const PathStr& path, bool bCube=false);
			//! 空のテクスチャを作成
			/*! 中身はゴミデータ */
			AnotherLHandle<UPTexture> createTexture(const spn::Size& size, GLInSizedFmt fmt, bool bRestore=false);
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
			AnotherLHandle<UPEffect> loadEffect(const PathStr& path);
			//! 頂点バッファの確保
			AnotherLHandle<UPVBuffer> makeVBuffer(GLuint dtype);
			//! インデックスバッファの確保
			AnotherLHandle<UPIBuffer> makeIBuffer(GLuint dtype);

			AnotherSHandle<UPTexture> getEmptyTexture() const;
			LHdl _common(const PathStr& key, std::function<UPResource()> cb);
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

			State	_mipLevel;
			GLuint	_texFlag;	//!< TEXTURE_2D or TEXTURE_CUBE_MAP
			GLInSizedFmt	_format;
			float	_coeff;

			bool _onDeviceReset();
			IGLTexture(GLInSizedFmt fmt, bool bCube);

			static void Use(IGLTexture& t);
			static void End(IGLTexture& t);

		protected:
			Inner1& setFilter(State miplevel, bool bLinearMag, bool bLinearMin);
			Inner1& setAnisotropicCoeff(float coeff);
			Inner1& setUVWrap(GLuint s, GLuint t);

		public:
			~IGLTexture();
			const spn::Size& getSize() const;
			GLint getTextureID() const;
			void onDeviceLost() override;
			void setActiveID(GLuint n);	//!< テクスチャユニット番号を指定してBind

			static bool IsMipmap(State level);
			bool isMipmap() const;
			bool isCubemap() const;

			//! 内容をファイルに保存 (主にデバッグ用)
			bool save(const PathStr& path);
	};
	DEF_GLRESOURCE_INNER(IGLTexture, (setFilter)(setAnisotropicCoeff)(setUVWrap))

	template <class T, int N>
	struct Pack {
		T	val[N];
		Pack() = default;
		Pack(std::initializer_list<T> il) {
			T* pVal = val;
			for(auto& a : il)
				*pVal++ = a;
		}
		Pack(const T& t) {
			for(auto& a : val)
				a = t;
		}

		bool operator == (const Pack& p) const {
			for(int i=0 ; i<6 ; i++) {
				if(val[i] != p.val[i])
					return false;
			}
			return true;
		}
	};

	//! ファイルから生成したテクスチャ
	/*! DeviceReset時:
		再度ファイルから読み出す */
	class TexFile : public IGLTexture {
		using QS6 = Pack<PathStr, 6>;
		boost::variant<PathStr, QS6>	_fPath;

		public:
			//! Cube時: 連番ファイル名から作成
			TexFile(const PathStr& path, bool bCube);
			TexFile(const PathStr& path0, const PathStr& path1, const PathStr& path2,
										const PathStr& path3, const PathStr& path4, const PathStr& path5);
			void onDeviceReset() override;
			bool operator == (const TexFile& t) const;
	};
#ifdef QT5
	//! ユーザー定義のユニークテクスチャ (QImage由来)
	/*! DeviceReset時:
		QImage使用時: 一旦バッファにコピーして後で復元
		空テクスチャ時: 何もしない	*/
	class TexUser : public IGLTexture {
		using QI6 = Pack<QImage, 6>;
		boost::variant<QImage, QI6>		_image;

		public:
			//! QtのImageクラスからテクスチャを生成
			TexUser(const QImage& img);
			TexUser(const QImage& img0, const QImage& img1, const QImage& img2,
					const QImage& img3, const QImage& img4, const QImage& img5);
			void onDeviceReset() override;
			bool operator == (const TexUser& t) const;
	};
#endif
	//! ユーザー定義の空テクスチャ
	/*! テクスチャへの書き込みなどサポート
		DeviceLost時の復元は任意
		内部バッファはDeviceLost用であり、DeviceがActiveな時はnone
		フォーマット変換は全てOpenGLにさせる */
	class TexEmpty : public IGLTexture {
		using OPBuff = boost::optional<spn::ByteBuff>;
		using OPFormat = boost::optional<GLTypeFmt>;

		bool		_bRestore;
		OPBuff		_buff;			//!< DeviceLost時用のバッファ
		OPFormat	_typeFormat;	//!< _buffに格納されているデータの形式(Type)
		//! テクスチャフォーマットから必要なサイズを計算してバッファを用意する
		void _prepareBuffer();

		public:
			TexEmpty(const spn::Size& size, GLInSizedFmt fmt, bool bRestore=false);
			void onDeviceLost() override;
			void onDeviceReset() override;
			bool operator == (const TexEmpty& t) const;

			//! テクスチャ全部書き換え = バッファも置き換え
			/*! \param[in] fmt テクスチャのフォーマット
				\param[in] srcFmt 入力フォーマット(Type)
				\param[in] bRestore trueなら内部バッファにコピーを持っておいてDeviceLostに備える */
			void writeData(GLInSizedFmt fmt, spn::AB_Byte buff, int width, GLTypeFmt srcFmt, bool bRestore);
			//! 部分的に書き込み
			/*! \param[in] ofsX 書き込み先オフセット X
				\param[in] ofsY 書き込み先オフセット Y
				\param[in] srcFmt 入力フォーマット(Type) */
			void writeRect(spn::AB_Byte buff, int width, int ofsX, int ofsY, GLTypeFmt srcFmt);
	};

	//! デバッグ用テクスチャ模様生成インタフェース
	class ITDGen {
		protected:
			using UPByte = std::unique_ptr<GLubyte>;
			UPByte	_buff;
			int		_width, _height;

			ITDGen(int w, int h);

		public:
			const GLubyte* getPtr() const;
			int getWidth() const;
			int getHeight() const;
	};
	//! 2色チェッカー
	class TDChecker : public ITDGen {
		public:
			TDChecker(const spn::Vec4& col0, const spn::Vec4& col1, int nDivW, int nDivH, int w, int h);
	};
	//! カラーチェッカー
	/*! 準モンテカルロで色を決定 */
	class TDCChecker : public ITDGen {
		public:
			TDCChecker(int nDivW, int nDivH, int w, int h);
	};
	//! ベタ地と1テクセル枠
	class TDBorder : public ITDGen {
		public:
			TDBorder(const spn::Vec4& col, const spn::Vec4& bcol, int w, int h);
	};

	//! デバッグ用のチェッカーテクスチャ
	/*! DeviceLost時:
		再度生成し直す */
	class TexDebug : public IGLTexture {
		// デバッグ用なので他との共有を考えず、UniquePtrとする
		std::unique_ptr<ITDGen>	_upGen;
		using ITD6 = Pack<ITDGen*, 6>;
		boost::variant<ITDGen*, ITD6>	_gen;

		public:
			TexDebug(ITDGen* gen, bool bCube);
			void onDeviceReset() override;
			bool operator == (const TexDebug& t) const;
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
