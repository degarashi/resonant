#pragma once
#include "glhead.hpp"
#include "glresource.hpp"
#include "glx_parse.hpp"
#include "spinner/matrix.hpp"
#include "spinner/adaptstream.hpp"
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <boost/lexical_cast.hpp>
#include "glx_id.hpp"

namespace rs {
	//! OpenGLの値設定関数代理クラス
	struct ValueSettingR;
	using VSFunc = void (*)(const ValueSettingR&);
	using VBFunc = decltype(&IGL::glEnable);
	struct ValueSettingR {
		ValueSetting::ValueT 	value[4];
		VSFunc					func;

		const static VSFunc cs_func[];

		explicit ValueSettingR(const ValueSetting& s);
		void action() const;
		template <class GF, class T0>
		void action(GF gf, T0) const {
			(GL.*gf)(boost::get<T0>(value[0])); }
		template <class GF, class T0, class T1>
		void action(GF gf, T0,T1) const {
			(GL.*gf)(boost::get<T0>(value[0]), boost::get<T1>(value[1])); }
		template <class GF, class T0, class T1, class T2>
		void action(GF gf, T0,T1,T2) const {
			(GL.*gf)(boost::get<T0>(value[0]), boost::get<T1>(value[1]), boost::get<T2>(value[2])); }
		template <class GF, class T0, class T1, class T2, class T3>
		void action(GF gf, T0,T1,T2,T3) const {
			(GL.*gf)(boost::get<T0>(value[0]), boost::get<T1>(value[1]), boost::get<T2>(value[2]), boost::get<T3>(value[3])); }

		bool operator == (const ValueSettingR& s) const;
	};
	//! OpenGLのBool値設定クラス
	struct BoolSettingR {
		GLenum		flag;
		VBFunc		func;

		const static VBFunc cs_func[2];

		explicit BoolSettingR(const BoolSetting& s);
		void action() const;
		bool operator == (const BoolSettingR& s) const;
	};

	struct VData {
		const static int MAX_STREAM = 4;
		using BuffA = const spn::Optional<draw::Buffer> (&)[MAX_STREAM];
		using AttrA = GLint[static_cast<int>(VSem::NUM_SEMANTIC)];

		BuffA	buff;
		const AttrA&	attrID;

		VData(BuffA b, const AttrA& at): buff(b), attrID(at) {}
	};
	//! 頂点宣言
	class VDecl {
		private:
			friend class boost::serialization::access;
			template <class Ar>
			void serialize(Ar& ar, const unsigned int) {
				ar	& BOOST_SERIALIZATION_NVP(_vdInfo);
				if(typename Ar::is_loading())
					_init();
			}
		public:
			struct VDInfo {
				friend class boost::serialization::access;
				template <class Ar>
				void serialize(Ar& ar, const unsigned int) {
					ar	& BOOST_SERIALIZATION_NVP(streamID)
						& BOOST_SERIALIZATION_NVP(offset)
						& BOOST_SERIALIZATION_NVP(elemFlag)
						& BOOST_SERIALIZATION_NVP(bNormalize)
						& BOOST_SERIALIZATION_NVP(elemSize)
						& BOOST_SERIALIZATION_NVP(semID);
				}

				GLuint	streamID,		//!< 便宜上の)ストリームID
						offset,			//!< バイトオフセット
						elemFlag,		//!< OpenGLの要素フラグ
						bNormalize,		//!< OpenGLが正規化するか(bool)
						elemSize,		//!< 要素数
						semID;			//!< 頂点セマンティクスID

				bool operator == (const VDInfo& v) const;
				bool operator != (const VDInfo& v) const;
			};
			using VDInfoV = std::vector<VDInfo>;
		private:
			using Func = std::function<void (GLuint, const VData::AttrA&)>;
			using FuncL = std::vector<Func>;
			FuncL		_func;						//!< ストリーム毎のサイズを1次元配列で格納 = 0番から並べる
			int			_nEnt[VData::MAX_STREAM+1];	//!< 各ストリームの先頭インデックス
			VDInfoV		_vdInfo;

			static VDInfoV _ToVector(std::initializer_list<VDInfo>& il);
			void _init();

		public:
			VDecl();
			VDecl(const VDInfoV& vl);
			//! 入力: {streamID, offset, GLFlag, bNoramalize, semantics}
			VDecl(std::initializer_list<VDInfo> il);
			//! OpenGLへ頂点位置を設定
			void apply(const VData& vdata) const;
			bool operator == (const VDecl& vd) const;
			bool operator != (const VDecl& vd) const;
	};

	using UniIDSet = std::unordered_set<GLint>;
	//! [UniformID -> Token]
	using UniMap = std::unordered_map<GLint, draw::SPToken>;

	// OpenGLのレンダリング設定
	using Setting = boost::variant<BoolSettingR, ValueSettingR>;
	using SettingList = std::vector<Setting>;
	//! Tech | Pass の分だけ作成
	class TPStructR {
		public:
			using MacroMap = std::map<std::string, std::string>;
			using AttrL = std::vector<const AttrEntry*>;
			using VaryL = std::vector<const VaryEntry*>;
			using ConstL = std::vector<const ConstEntry*>;
			using UnifL = std::vector<const UnifEntry*>;
			using VAttrID = const GLint (&)[static_cast<int>(VSem::NUM_SEMANTIC)];

		private:
			HLProg			_prog;
			// --- 関連情報(ゼロから構築する場合の設定項目) ---
			//! Attribute: 頂点セマンティクスに対する頂点ID
			/*! 無効なセマンティクスは負数 */
			GLint			_vAttrID[static_cast<int>(VSem::NUM_SEMANTIC)];

			//! Setting: Uniformデフォルト値(texture, vector, float, bool)設定を含む。GLDeviceの設定クラスリスト
			SettingList		_setting;

			UniIDSet		_noDefValue;	//!< Uniform非デフォルト値エントリIDセット (主にユーザーの入力チェック用)
			UniMap			_defaultValue;	//!< Uniformデフォルト値と対応するID
			bool			_bInit = false;	//!< lost/resetのチェック用 (Debug)

			// ----------- GLXStructから読んだデータ群 -----------
			AttrL			_attrL;
			VaryL			_varyL;
			ConstL			_constL;
			UnifL			_unifL;

		public:
			TPStructR();
			TPStructR(TPStructR&& tp);
			//! エフェクトファイルのパース結果を読み取る
			TPStructR(const GLXStruct& gs, int tech, int pass);

			//! OpenGL関連のリソースを解放
			/*! GLResourceの物とは別。GLEffectから呼ぶ */
			void ts_onDeviceLost();
			void ts_onDeviceReset(const GLEffect& glx);

			bool findSetting(const Setting& s) const;
			void swap(TPStructR& tp) noexcept;

			const UniMap& getUniformDefault() const;
			const UniIDSet& getUniformEntries() const;
			VAttrID getVAttrID() const;

			const HLProg& getProgram() const;
			//! OpenGLに設定を適用
			void applySetting() const;
			//! 設定差分を求める
			static SettingList CalcDiff(const TPStructR& from, const TPStructR& to);
	};
	//! 引数の型チェックと同時に出力
	struct ArgChecker : boost::static_visitor<> {
		enum TARGET {
			BOOLEAN,
			SCALAR,
			VECTOR,
			NONE
		};
		const static int N_TARGET = 4;
		TARGET _target[N_TARGET];
		const ArgItem* _arg[N_TARGET];
		const std::string& _shName;
		std::ostream& _ost;
		int _cursor = 0;

		ArgChecker(std::ostream& ost, const std::string& shName, const std::vector<ArgItem>& args);
		static TARGET Detect(int type);
		void _checkAndSet(TARGET tgt);
		void operator()(const std::vector<float>& v);
		void operator()(float v);
		void operator()(bool b);
		void finalizeCheck();
	};

	namespace Bit {
		template <class T0, class T1>
		inline bool ChClear(T0& t, const T1& tbit) {
			bool bRet = t & tbit;
			t &= ~tbit;
			return bRet;
		}
	}
	namespace draw {
		struct ITag {
			virtual ~ITag() {}
			virtual void exec() = 0;
		};
		using UPTag = std::unique_ptr<ITag>;

		class UserFunc : public Token {
			public:
				using Func = std::function<void ()>;
			private:
				Func	_func;
			public:
				UserFunc(const Func& f);
				void exec() override;
		};
		//MEMO: ソースの改変を経ている為、少し実装が回りくどいと思われる
		class VStream {
			private:
				friend class RUser<VStream>;
				void use_begin() const;
				void use_end() const;
			public:
				using OPBuffer = spn::Optional<Buffer>;
				using OPVAttrId = spn::Optional<TPStructR::VAttrID>;
				// vertex stream
				SPVDecl		spVDecl;
				OPBuffer	vbuff[VData::MAX_STREAM];
				OPVAttrId	vAttrId;
				// index stream
				OPBuffer	ibuff;

				RUser<VStream> use();
		};
		class Tag_DrawBase : public ITag {
			private:
				TokenV		_token;
				VStream		_vstream;
			protected:
				Tag_DrawBase(TokenV&& t, VStream&& vs);
				RUser<VStream> use();
		};
		//! Draw token (without index)
		class Tag_Draw : public Tag_DrawBase {
			private:
				GLenum		_mode;
				GLint		_first;
				GLsizei		_count;
			public:
				Tag_Draw(TokenV&& t, VStream&& vs, GLenum mode, GLint first, GLsizei count);
				void exec() override;
		};
		//! Draw token (with index)
		class Tag_DrawI : public Tag_DrawBase {
			private:
				GLenum		_mode;
				GLsizei		_count;
				GLenum		_sizeF;
				GLuint		_offset;
			public:
				/*! \param[in] mode 描画モードフラグ(OpenGL)
					\param[in] count 描画に使用される要素数
					\param[in] sizeF 1要素のサイズを表すフラグ
					\param[in] offset オフセットバイト数 */
				Tag_DrawI(TokenV&& t, VStream&& vs, GLenum mode, GLsizei count, GLenum sizeF, GLuint offset);
				void exec() override;
		};

		/*! PreFuncとして(TPStructR::applySettingsを追加)
			[Program, FrameBuff, RenderBuff] */
		class Task {
			constexpr static int NUM_ENTRY = 3;
			//! 描画エントリのリングバッファ
			using UPTagV = std::vector<UPTag>;
			UPTagV	_entry[NUM_ENTRY];
			//! 読み書きカーソル位置
			int			_curWrite, _curRead;
			Mutex		_mutex;
			CondV		_cond;

			UPTagV& refWriteEnt();
			UPTagV& refReadEnt();
			public:
				Task();
				// -------------- from MainThread --------------
				void pushTag(UPTag tag);
				void beginTask();
				void endTask();
				void clear();
				// -------------- from DrawThread --------------
				void execTask();
		};
	}

	//! GLXエフェクト管理クラス
	class GLEffect : public IGLResource {
		public:
			struct tagConstant {};
			using GlxId = rs::IdMgr_Glx<tagConstant>;
			//! Uniform & TechPass 定数にIdを割り当てるクラス
			static GlxId	s_myId;

			//! [UniformID -> TextureActiveIndex]
			using TexIndex = std::unordered_map<GLint, GLint>;
			//! [(TechID|PassID) -> ProgramClass]
			using TechMap = std::unordered_map<GL16Id, TPStructR>;
			//! Tech名とPass名のセット
			using TechName = std::vector<std::vector<std::string>>;
			using TPRef = spn::Optional<const TPStructR&>;

		private:
			GLXStruct		_result;			//!< 元になった構造体 (Effectファイル解析結果)
			TechMap			_techMap;			//!< ゼロから設定を構築する場合の情報や頂点セマンティクス
			TechName		_techName;
			bool			_bInit = false;		//!< deviceLost/Resetの状態区別

			draw::Task		_task;
			struct Current {
				class Vertex {
					private:
						SPVDecl			_spVDecl;
						HLVb			_vbuff[VData::MAX_STREAM];
						mutable bool	_bChanged;
					public:
						Vertex();
						void setVDecl(const SPVDecl& v);
						void setVBuffer(HVb hVb, int n);
						void reset();
						void extractData(draw::VStream& dst, TPStructR::VAttrID vAttrId) const;
				} vertex;
				class Index {
					private:
						HLIb			_ibuff;
						mutable bool	_bChanged;
					public:
						Index();
						void setIBuffer(HIb hIb);
						HIb getIBuffer() const;
						void reset();
						void extractData(draw::VStream& dst) const;
				} index;

				// Tech, Pass何れかを変更したらDraw変数をクリア
				// passをセットしたタイミングでProgramを検索し、tpsにセット
				OPGLint				tech,
									pass;
				bool				bDefaultParam;	//!< Tech切替時、trueならデフォルト値読み込み
				TPRef				tps;			//!< 現在使用中のTech
				UniMap				uniMap;			//!< 現在設定中のUniform
				TexIndex			texIndex;		//!< [UniformID : TextureIndex]
				draw::TokenV		tokenV;

				void reset();
				void _clean_drawvalue();
				void setTech(GLint idTech, bool bDefault);
				void setPass(GLint idPass, TechMap& tmap);
				void _outputDrawCall(draw::VStream& vs);
				//! DrawCallに関連するAPI呼び出しTokenを出力
				/*! Vertex,Index BufferやUniform変数など */
				draw::UPTag outputDrawCall(GLenum mode, GLint first, GLsizei count);
				draw::UPTag outputDrawCallIndexed(GLenum mode, GLsizei count, GLenum sizeF, GLuint offset);
			} _current;

			using UnifIdV = std::vector<GLint>;
			using UnifIdM = std::unordered_map<GL16Id, UnifIdV>;
			using IdPair = std::pair<int,int>;
			using TechIdV = std::vector<IdPair>;

			struct {
				const StrV*			src = nullptr;
				UnifIdM				result;		// [Tech|Pass]->[size(src)]
				const UnifIdV*		resultCur;	// current tech-pass entry
			} _unifId;

			struct {
				const StrPairV*		src = nullptr;
				TechIdV				result;
			} _techId;

			OPGLint _getPassId(int techId, const std::string& pass) const;
			OPGLint _getUnifId(IdValue id) const;
			IdPair _getTechPassId(IdValue id) const;

			/*! 引数はコンパイラで静的に確保される定数を想定しているのでポインタで受け取る
				動的にリストを削除したりはサポートしない */
			void _setConstantUniformList(const StrV* src);
			void _setConstantTechPassList(const StrPairV* src);
		public:
			//! Effectファイル(gfx)を読み込む
			/*! フォーマットの解析まではするがGLリソースの確保はしない */
			GLEffect(spn::AdaptStream& s);
			void onDeviceLost() override;
			void onDeviceReset() override;
			//! GLEffectで発生する例外基底
			struct EC_Base : std::runtime_error {
				using std::runtime_error::runtime_error;
			};
			//! 該当するTechが無い
			struct EC_TechNotFound : EC_Base { using EC_Base::EC_Base; };
			//! 範囲外のPass番号を指定
			struct EC_PassOutOfRange : EC_Base { using EC_Base::EC_Base; };
			//! 頂点Attributeにデータがセットされてない
			struct EC_EmptyAttribute : EC_Base { using EC_Base::EC_Base; };
			//! Uniformにデータがセットされてない
			struct EC_EmptyUniform : EC_Base { using EC_Base::EC_Base; };
			//! Macroにデータがセットされてない
			struct EC_EmptyMacro : EC_Base { using EC_Base::EC_Base; };
			//! GLXファイルの文法エラー
			struct EC_GLXGrammar : EC_Base { using EC_Base::EC_Base; };
			//! 該当するGLXファイルが見つからない
			struct EC_FileNotFound : EC_Base {
				EC_FileNotFound(const std::string& fPath);
			};
			//! システムセマンティクス(2D)
			//! システムセマンティクス(3D)
			//! システムセマンティクス(Both)

			//! Technique
			OPGLint getTechId(const std::string& tech) const;
			OPGLint getPassId(const std::string& pass) const;
			OPGLint getPassId(const std::string& tech, const std::string& pass) const;
			OPGLint getCurTechId() const;
			OPGLint getCurPassId() const;
			//! TechID, PassIDに該当するProgramハンドルを返す
			/*! \param[in] techId (-1 = currentTechId)
				\param[in] passId (-1 = currentPassId)
				\return 該当があればそのハンドル、なければ無効なハンドル */
			HLProg getProgram(int techId=-1, int passId=-1) const;
			//! Tech切替時に初期値をセットするか
			void setTechnique(int id, bool bReset);
			//! Pass指定
			void setPass(int id);
			//! 頂点宣言
			/*! \param[in] decl 頂点定義クラスのポインタ(定数を前提) */
			void setVDecl(const SPVDecl& decl);
			void setVStream(HVb vb, int n);
			void setIStream(HIb ib);
			//! Uniform変数設定 (Tech/Passで指定された名前とセマンティクスのすり合わせを行う)
			OPGLint getUniformID(const std::string& name) const;

			void setTechPassId(IdValue id);
			//! 定数値を使ったUniform変数設定。Uniform値が存在しなくてもエラーにならない
			template <class T>
			void setUniform_try(IdValue id, T&& t, bool bT=false) {
				if(auto idv = _getUnifId(id))
					setUniform(*idv, std::forward<T>(t), bT);
			}
			//! 定数値を使ったUniform変数設定
			template <class T>
			void setUniform(IdValue id, T&& t, bool bT=false) {
				auto idv = _getUnifId(id);
				// 定数値に対応するUniform変数が見つからない時は警告を出す
				Assert(Warn, idv, "Uniform-ConstantId: %1% not found", id.value)
				setUniform(*idv, std::forward<T>(t), bT);
			}
			//! 単体Uniform変数セット
			template <class T, class = typename std::enable_if< !std::is_pointer<T>::value >::type>
			void setUniform(GLint id, const T& t, bool bT=false) {
				setUniform(id, &t, 1, bT); }
			//! 配列Uniform変数セット
			template <class T>
			void setUniform(GLint id, const T* t, int n, bool bT=false) {
				_makeUniformToken(_current.uniMap, id, t, n, bT); }
			void _makeUniformToken(UniMap& dstToken, GLint id, const bool* b, int n, bool) const;
			void _makeUniformToken(UniMap& dstToken, GLint id, const float* fv, int n, bool) const;
			void _makeUniformToken(UniMap& dstToken, GLint id, const double* fv, int n, bool) const;
			void _makeUniformToken(UniMap& dstToken, GLint id, const int* iv, int n, bool) const;
			template <int DN, bool A>
			void _makeUniformToken(UniMap& dstToken, GLint id, const spn::VecT<DN,A>* v, int n, bool) const {
				dstToken.emplace(id, std::make_shared<draw::Unif_Vec<float, DN>>(id, v, n)); }
			template <int DM, int DN, bool A>
			void _makeUniformToken(UniMap& dstToken, GLint id, const spn::MatT<DM,DN,A>* m, int n, bool bT) const {
				constexpr int DIM = spn::TValue<DM,DN>::great;
				std::vector<spn::MatT<DIM,DIM,false>> tm(n);
				for(int i=0 ; i<n ; i++)
					m[i].convert(tm[i]);
				_makeUniformToken(dstToken, id, tm.data(), n, bT);
			}
			template <int DN, bool A>
			void _makeUniformToken(UniMap& dstToken, GLint id, const spn::MatT<DN,DN,A>* m, int n, bool bT) const {
				dstToken.emplace(id, std::make_shared<draw::Unif_Mat<float, DN>>(id, m, n, bT)); }
			void _makeUniformToken(UniMap& dstToken, GLint id, const HTex* hTex, int n, bool) const;
			void _makeUniformToken(UniMap& dstToken, GLint id, const HLTex* hlTex, int n, bool) const;

			//! IStreamを使用して描画
			/*! \param[in] mode 描画モードフラグ(OpenGL)
				\param[in] count 描画に使用される要素数
				\param[in] offsetElem オフセット要素数 */
			virtual void drawIndexed(GLenum mode, GLsizei count, GLuint offsetElem=0);
			//! IStreamを使わず描画
			/*! \param[in] mode 描画モードフラグ(OpenGL)
				\param[in] first 描画を開始する要素オフセット
				\param[in] count 描画に使用される要素数 */
			virtual void draw(GLenum mode, GLint first, GLsizei count);

			// ---- from MainThread ----
			//! バッファを2つともクリア(主にスリープ時)
			void clearTask();
			//! バッファを切り替えて古いバッファをクリア
			/*! まだDrawThreadが描画を終えてない場合はブロック */
			void beginTask();
			//! OpenGLコマンドをFlushする
			void endTask();
			// ---- from DrawThread ----
			void execTask();
	};
}
