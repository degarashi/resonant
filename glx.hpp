#pragma once
#define BOOST_PP_VARIADICS 1
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
	using VBFunc = decltype(&glEnable);
	struct ValueSettingR {
		ValueSetting::ValueT 	value[4];
		VSFunc					func;

		const static VSFunc cs_func[];

		static void StencilFuncFront(int func, int ref, int mask);
		static void StencilFuncBack(int func, int ref, int mask);
		static void StencilOpFront(int sfail, int dpfail, int dppass);
		static void StencilOpBack(int sfail, int dpfail, int dppass);
		static void StencilMaskFront(int mask);
		static void StencilMaskBack(int mask);

		explicit ValueSettingR(const ValueSetting& s);
		void action() const;
		template <class GF, class T0>
		void action(GF gf, T0) const { GLEC_P(Trap, gf, boost::get<T0>(value[0])); }
		template <class GF, class T0, class T1>
		void action(GF gf, T0,T1) const { GLEC_P(Trap, gf, boost::get<T0>(value[0]), boost::get<T1>(value[1])); }
		template <class GF, class T0, class T1, class T2>
		void action(GF gf, T0,T1,T2) const { GLEC_P(Trap, gf, boost::get<T0>(value[0]), boost::get<T1>(value[1]), boost::get<T2>(value[2])); }
		template <class GF, class T0, class T1, class T2, class T3>
		void action(GF gf, T0,T1,T2,T3) const { GLEC_P(Trap, gf, boost::get<T0>(value[0]), boost::get<T1>(value[1]), boost::get<T2>(value[2]), boost::get<T3>(value[3])); }

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
		using HLBuffA = HLVb[MAX_STREAM];
		using AttrA = GLint[static_cast<int>(VSem::NUM_SEMANTIC)];

		const HLBuffA&	hlBuff;
		const AttrA&	attrID;

		VData(const HLBuffA& b, const AttrA& at): hlBuff(b), attrID(at) {}
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

	using UniVal = boost::variant<bool, int, float, spn::Vec3, spn::Vec4,
					spn::Mat32, spn::Mat33, spn::Mat43, spn::Mat44, HLTex>;
	using UniMapStr = std::unordered_map<std::string, UniVal>;
	using UniMapID = std::unordered_map<GLint, UniVal>;
	using UniEntryMap = std::unordered_set<std::string>;

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

		private:
			HLProg			_prog;
			// --- 関連情報(ゼロから構築する場合の設定項目) ---
			//! Attribute: 頂点セマンティクスに対する頂点ID
			/*! 無効なセマンティクスは負数 */
			GLint			_vAttrID[static_cast<int>(VSem::NUM_SEMANTIC)];
			//! Setting: Uniformデフォルト値(texture, vector, float, bool)設定を含む。GLDeviceの設定クラスリスト
			SettingList		_setting;

			UniEntryMap		_noDefValue;	//!< Uniform非デフォルト値エントリリスト (主にユーザーの入力チェック用)
			UniMapID		_defValue;		//!< Uniformデフォルト値と対応するID
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
			void ts_onDeviceReset();

			bool findSetting(const Setting& s) const;
			void swap(TPStructR& tp) noexcept;

			const UniMapID& getUniformDefault() const;
			const UniEntryMap& getUniformEntries() const;

			const HLProg& getProgram() const;
			//! OpenGLに設定を適用
			void applySetting() const;
			//! 頂点ポインタを設定 (GLXから呼ぶ)
			void setVertex(const SPVDecl& vdecl, const HLVb (&stream)[VData::MAX_STREAM]) const;
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

	//! GLXエフェクト管理クラス
	class GLEffect : public IGLResource {
		public:
			using TexIndex = std::unordered_map<GLint, GLint>;
		private:
			using UseArray = std::vector<std::string>;
			//! アクティブなBoolSetting, ValueSettingを適用
			void _applyShaderSetting() const;

			using TechMap = std::unordered_map<GL16ID, TPStructR>;
	// TODO: 差分キャッシュの実装 (set差分 + vAttrID)
	//		using DiffCache = std::unordered_map<GLDiffID, int>;
			using TechName = std::vector<std::vector<std::string>>;

			GLXStruct		_result;		//!< 元になった構造体 (Effectファイル解析結果)
			TechMap			_techMap;		//!< ゼロから設定を構築する場合の情報や頂点セマンティクス
	//		DiffCache		_diffCache;		//!< セッティング差分を格納
			TechName		_techName;		//!< Tech名とPass名のセット

			// --------------- 現在アクティブな設定 ---------------
			SPVDecl			_spVDecl;
			HLVb			_vBuffer[VData::MAX_STREAM];
			HLIb			_iBuffer;
			using TPID = boost::optional<int>;
			TPID			_idTech,		//!< 移行予定のTechID
							_idTechCur,		//!< 現在OpenGLにセットされているTechID
							_idPass,		//!< 移行予定のPassID
							_idPassCur;		//!< 現在OpenGLにセットされているPassID
			bool			_bDefaultParam;	//!< Tech切替時、trueならデフォルト値読み込み
			UniMapID		_uniMapID,		//!< 設定待ちのエントリ
							_uniMapIDTmp;	//!< 設定し終わったエントリ
			using TPRef = boost::optional<const TPStructR&>;
			TPRef			_tps;			//!< 現在使用中のTech
			TexIndex		_texIndex;		//!< [AttrID : TextureIndex]

			enum REFLAG {
				REFL_PROGRAM = 0x01,		//!< シェーダーのUse宣言
				REFL_UNIFORM = 0x02,		//!< キャッシュしたUniform値の設定
				REFL_VSTREAM = 0x04,		//!< VStreamの設定
				REFL_ISTREAM = 0x08,		//!< IStreamの設定
				REFL_ALL = 0x0f
			};
			uint32_t		_rflg = REFL_ALL;
			bool			_bInit = false;		//!< deviceLost/Resetの状態区別

			// ----- リフレッシュ関数 -----
			void _refreshProgram();
			void _refreshUniform();
			void _refreshVStream();
			void _refreshIStream();

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

			//! Uniform変数設定 (Tech/Passで指定された名前とセマンティクスのすり合わせを行う)
			GLint getUniformID(const std::string& name);
			template <class T>
			void setUniform(T&& v, GLint id) {
				if(id < 0)
					return;
				_rflg |= REFL_UNIFORM;
				_uniMapID.insert(std::make_pair(id, std::forward<T>(v)));
			}

			//! 現在セットされているUniform変数の保存用
			const UniMapID& getUniformMap();
			//! セーブしておいたUniform変数群を復元
			void inputParams(const UniMapStr& u);
			void inputParams(const UniMapID& u);
			//! 実際にOpenGLへ各種設定を適用
			void applySetting();

			//! 頂点宣言
			/*! \param[in] decl 頂点定義クラスのポインタ(定数を前提) */
			void setVDecl(const SPVDecl& decl);
			void setVStream(HVb vb, int n);
			void setIStream(HIb ib);
			//! Tech指定
			/*!	Techを切り替えるとUniformがデフォルトにリセットされる
				切替時に引数としてUniMapを渡すとそれで初期化 */
			void setTechnique(int techID, bool bDefault);
			int getTechID(const std::string& tech) const;
			int getCurTechID() const;
			//! Pass指定
			void setPass(int passID);
			int getPassID(const std::string& pass) const;
			int getCurPassID() const;

			//! IStreamを使用して描画
			void drawIndexed(GLenum mode, GLsizei count, GLuint offset=0);
			//! IStreamを使わず描画
			void draw(GLenum mode, GLint first, GLsizei count);
	};
}
