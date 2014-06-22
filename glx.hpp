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
		public:
			struct VDInfo {
				GLuint	streamID,		//!< 便宜上の)ストリームID
						offset,			//!< バイトオフセット
						elemFlag,		//!< OpenGLの要素フラグ
						bNormalize,		//!< OpenGLが正規化するか(bool)
						elemSize,		//!< 要素数
						semID;			//!< 頂点セマンティクスID
			};
		private:
			using Func = std::function<void (GLuint, const VData::AttrA&)>;
			using FuncL = std::vector<Func>;
			FuncL	_func;						//!< ストリーム毎のサイズを1次元配列で格納 = 0番から並べる
			int		_nEnt[VData::MAX_STREAM+1];	//!< 各ストリームの先頭インデックス

		public:
			VDecl();
			//! 入力: {streamID, offset, GLFlag, bNoramalize, semantics}
			VDecl(std::initializer_list<VDInfo> il);
			//! OpenGLへ頂点位置を設定
			void apply(const VData& vdata) const;
	};

	using UniIDSet = std::unordered_set<GLint>;
	//! [UniformID -> Token]
	using UniMap = std::unordered_map<GLint, draw::SPToken>;

	using PreFuncL = std::vector<PreFunc>;
	// OpenGLのレンダリング設定
	using Setting = boost::variant<BoolSettingR, ValueSettingR>;
	using SettingList = std::vector<Setting>;
	//! Tech | Pass の分だけ作成
	//MEMO: デフォルトテクスチャのPreFuncはどっかで事前に呼んで置かないとGPUに画素が転送されない
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
			PreFuncL		_preFuncL;
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
			const PreFuncL& getPreFunc() const;
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
		/*!	描画スレッドが処理するタスク基底
			PreFunc = 描画がスキップされた時も実行される処理 */
		class Tag : public IPreFunc {
			PreFuncL		_funcL;
			protected:
				Priority64	_priority;
			public:
				virtual ~Tag() {}
				virtual void exec();
				virtual void cancel();
				void clearTags();
				void addPreFunc(PreFunc pf) override;
		};
		using UPTag = std::unique_ptr<Tag>;

		//! 描画ソートで使う優先度値
		struct Prio {
			Priority	userP, sysP;
			float		camDist;
			int			texID, vertID, indexID;

			Priority64 makeID(float minD, float maxD) const;
		};
		//! DrawToken: DrawCall
		struct DrawCall : Token {
			GLenum	_mode;
			GLint	_first;
			GLsizei	_count;

			DrawCall(GLenum mode, GLint first, GLsizei count);
			void exec() override;
		};
		//! DrawToken: DrawCall(Indexed)
		struct DrawCallI : Token {
			GLenum	_mode;
			GLsizei	_count;
			GLenum	_sizeF;
			GLuint	_offset;

			/*! \param[in] mode 描画モードフラグ(OpenGL)
				\param[in] count 描画に使用される要素数
				\param[in] sizeF 1要素のサイズを表すフラグ
				\param[in] offset オフセットバイト数 */
			DrawCallI(GLenum mode, GLsizei count, GLenum sizeF, GLuint offset);
			void exec() override;
		};
		//! DrawTag (Texture, Uniformなど雑多なオペレーション)
		class NormalTag : public Tag {
			friend class ::rs::GLEffect;
			TokenL		_tokenL;
			public:
				// ---- from DrawThread ----
				void exec() override;
		};
		//! ProgramとUniformの初期値をセットするTag
		/*! PreFuncとして(TPStructR::applySettingsを追加)
			[Program, VStream, IStream, FrameBuff, RenderBuff] */
		class InitTag : public Tag {
			friend class ::rs::GLEffect;
			using OPProg = spn::Optional<Program>;
			using OPBuff = spn::Optional<Buffer>;
			using OPVAttrID = spn::Optional<TPStructR::VAttrID>;

			OPProg		_opProgram;
			SPVDecl		_spVDecl;
			OPVAttrID	_opVAttrID;
			OPBuff		_opVb[VData::MAX_STREAM];
			OPBuff		_opIb;

			using OPFb = spn::Optional<FrameBuff>;
			OPFb		_opFb;

			public:
				// ---- from DrawThread ----
				void exec() override;
		};
		// 今のところDrawTagはInitTagとNormalTagしかない

		class Task {
			constexpr static int NUM_ENTRY = 3;
			//! 描画エントリのリングバッファ
			using UPTagL = std::vector<UPTag>;
			UPTagL	_entry[NUM_ENTRY];
			//! 読み書きカーソル位置
			int			_curWrite, _curRead;
			Mutex		_mutex;
			CondV		_cond;

			UPTagL& refWriteEnt();
			UPTagL& refReadEnt();
			public:
				Task();
				// -------------- from MainThread --------------
				void pushTag(Tag* tag);
				void beginTask();
				void endTask();
				void clear();
				// -------------- from DrawThread --------------
				void execTask(bool bSkip);
		};
	}

	//! GLXエフェクト管理クラス
	class GLEffect : public IGLResource {
		public:
			//! [UniformID -> TextureActiveIndex]
			using TexIndex = std::unordered_map<GLint, GLint>;
			//! [(TechID|PassID) -> ProgramClass]
			using TechMap = std::unordered_map<GL16ID, TPStructR>;
			//! Tech名とPass名のセット
			using TechName = std::vector<std::vector<std::string>>;
			using TPRef = spn::Optional<const TPStructR&>;

		private:
			GLXStruct		_result;			//!< 元になった構造体 (Effectファイル解析結果)
			TechMap			_techMap;			//!< ゼロから設定を構築する場合の情報や頂点セマンティクス
			TechName		_techName;
			bool			_bInit = false;		//!< deviceLost/Resetの状態区別
			bool			_bDefaultParam;		//!< Tech切替時、trueならデフォルト値読み込み

			draw::Task		_task;
			Priority		_sysP;
			struct {
				// passをセットしたタイミングでProgramを検索し、InitTagにセット
				using OPProg = spn::Optional<GLuint>;
				using OPInitTag = spn::Optional<draw::InitTag>;
				using OPNormalTag = spn::Optional<draw::NormalTag>;

				OPGLint			tech,
								pass;
				TPRef			tps;		//!< 現在使用中のTech
				UniMap			uniMap;		//!< 現在設定中のUniform
				TexIndex		texIndex;	//!< [UniformID : TextureIndex]

				bool			bInit;
				draw::InitTag	init;
				bool			bNormal;
				draw::NormalTag	normal;

				draw::Prio		prio;
			} _current;

			void _exportInitTag();
			bool _exportUniform();
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
			OPGLint getTechID(const std::string& tech) const;
			OPGLint getPassID(const std::string& pass) const;
			OPGLint getCurTechID() const;
			OPGLint getCurPassID() const;
			//! TechID, PassIDに該当するProgramハンドルを返す
			/*! \param[in] techID (-1 = currentTechID)
				\param[in] passID (-1 = currentPassID)
				\return 該当があればそのハンドル、なければ無効なハンドル */
			HLProg getProgram(int techID=-1, int passID=-1) const;
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

			//! 単体Uniform変数セット
			template <class T, class = std::enable_if_t< !std::is_pointer<T>::value >>
			void setUniform(GLint id, const T& t, bool bT=false) {
				setUniform(id, &t, 1, bT); }
			//! 配列Uniform変数セット
			template <class T>
			void setUniform(GLint id, const T* t, int n, bool bT=false) {
				_makeUniformToken(_current.uniMap, _current.normal, id, t, n, bT); }
			void _makeUniformToken(UniMap& dstToken, IPreFunc& pf, GLint id, const bool* b, int n, bool) const;
			void _makeUniformToken(UniMap& dstToken, IPreFunc& pf, GLint id, const float* fv, int n, bool) const;
			void _makeUniformToken(UniMap& dstToken, IPreFunc& pf, GLint id, const double* fv, int n, bool) const;
			void _makeUniformToken(UniMap& dstToken, IPreFunc& pf, GLint id, const int* iv, int n, bool) const;
			template <int DN, bool A>
			void _makeUniformToken(UniMap& dstToken, IPreFunc& /*pf*/, GLint id, const spn::VecT<DN,A>* v, int n, bool) const {
				dstToken.emplace(id, std::make_shared<draw::Unif_Vec<float, DN>>(id, v, n)); }
			template <int DM, int DN, bool A>
			void _makeUniformToken(UniMap& dstToken, IPreFunc& pf, GLint id, const spn::MatT<DM,DN,A>* m, int n, bool bT) const {
				constexpr int DIM = spn::TValue<DM,DN>::great;
				std::vector<spn::MatT<DIM,DIM,false>> tm(n);
				for(int i=0 ; i<n ; i++)
					m[i].convert(tm[i]);
				_makeUniformToken(dstToken, pf, id, tm.data(), n, bT);
			}
			template <int DN, bool A>
			void _makeUniformToken(UniMap& dstToken, IPreFunc& /*pf*/, GLint id, const spn::MatT<DN,DN,A>* m, int n, bool bT) const {
				dstToken.emplace(id, std::make_shared<draw::Unif_Mat<float, DN>>(id, m, n, bT)); }
			void _makeUniformToken(UniMap& dstToken, IPreFunc& pf, GLint id, const HTex* hTex, int n, bool) const;
			void _makeUniformToken(UniMap& dstToken, IPreFunc& pf, GLint id, const HLTex* hlTex, int n, bool) const;

			void setUserPriority(Priority p);
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
			void execTask(bool bSkip);
	};
}
