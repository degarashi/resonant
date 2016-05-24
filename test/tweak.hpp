#pragma once
#include "../util/screenrect.hpp"
#include "../util/textdraw.hpp"
#include "../updater.hpp"
#include "spinner/structure/treenode.hpp"

struct FVec4 {
	int			nValue;
	spn::Vec4	value;

	FVec4() = default;
	FVec4(int nv, float iv);

	std::ostream& write(std::ostream& s) const;
	static FVec4 LoadFromLua(const rs::LValueS& v);
	rs::LCValue	toLCValue() const;
};
class Tweak : public rs::DrawableObjT<Tweak> {
	private:
		using Vec2 = spn::Vec2;
		using Vec3 = spn::Vec3;
		using Vec4 = spn::Vec4;
		using LValueS = rs::LValueS;
		using LValueG = rs::LValueG;
		using LCValue = rs::LCValue;
		using DegF = spn::DegF;
		using RectF = spn::RectF;
		using Name = std::string;
		using HAct = rs::HAct;
		using SHandle = spn::SHandle;
		class Value;
		using Value_UP = std::unique_ptr<Value>;

	public:
		//! 要素ごとの基本色
		struct Color {
			enum type {
				Linear,
				Log,
				Dir2D,
				Dir3D,
				Node,
				Cursor,
				_Num
			} c;
			const rs::util::ColorA& get() const;
		};
		class Drawer;
		//! Tweakツリーインタフェース
		struct IBase {
			virtual void set(const LValueS& v, bool bStep);
			virtual void increment(float inc, int index);
			virtual int draw[[noreturn]](const Vec2& offset,  const Vec2& unit, Drawer& d) const;
			virtual rs::LCValue get() const;
		};
		//! Tweakノード
		//! 複数の子ノードを持てる
		class INode : public spn::TreeNode<INode>, public IBase {
			protected:
				Name			_name;
				rs::HLText		_text;
			public:
				INode(rs::CCoreID cid, const Name& name);
				virtual bool expand(bool b) = 0;
				virtual bool isExpanded() const = 0;
				virtual bool isNode() const = 0;
				virtual void setPointer(Value_UP v) = 0;
				virtual std::ostream& write(std::ostream& s) const = 0;
				const Name& getName() const;
		};
		class Drawer {
			private:
				rs::util::TextHUD		_text;			//!< 矩形描画クラス
				rs::util::WindowRect	_rect;			//!< テキスト描画クラス
				rs::IEffect&			_effect;		//!< 描画インタフェース
				RectF					_range;			//!< 仮想空間上での表示範囲
				const INode* const		_cursor;		//!< カーソル位置にある要素
				Vec2					_offset,		//!< 描画オフセット
										_cursorAt;		//!< カーソル描画オフセット

			public:
				/*!
					\param[in] range		仮想空間上での表示範囲
					\param[in] offset		実際の描画オフセット(左上からの)
					\param[in] cur			カーソル位置にある要素
					\param[in] e			描画インタフェース
				*/
				Drawer(const RectF& range, const Vec2& offset,
						const INode::SP& cur, rs::IEffect& e);
				//! 描画処理中に出現したカーソル項目の位置
				const Vec2& getCursorAt() const;
				//! 描画の範囲内かの判定
				/*!
					\param p カーソル位置記憶用
					\param r 描画判定対象の矩形
					\return 描画の必要があるならtrue
				*/
				bool checkDraw(const INode* p, const RectF& r);
				//! 任意の色で矩形描画
				/*!
					\param rect			描画対象の矩形
					\param bWireframe	trueならワイヤフレームで描画
					\param color		矩形色
				*/
				void drawRect(const RectF& rect, bool bWireframe, Color color);
				//! 塗りつぶし矩形とワイヤー矩形の両方描画
				void drawRectBoth(const RectF& rect, Color color);
				void drawText(const Vec2& offset, rs::HText hText, Color color);
				//! 縦位置中央に補正したテキスト描画
				void drawTextVCenter(const RectF& rect, rs::HText hText, Color color);
		};
	private:
		//! 調整可能な定数値
		class Value : public IBase {
			protected:
				rs::CCoreID			_cid;
				mutable bool		_bRefl;
			private:
				mutable rs::HLText	_valueText;
			protected:
				virtual rs::HLText _getValueText() const = 0;
			public:
				Value(rs::CCoreID cid);
				rs::HText getValueText() const;
				virtual Value_UP clone() const = 0;
				virtual std::ostream& write(std::ostream& s) const = 0;
		};
		//! Valueクラスのcloneメソッドをテンプレートにて定義
		//! \sa Value
		template <class T>
		class ValueT : public Value {
			public:
				using Value::Value;
				virtual Value_UP clone() const {
					return std::make_unique<T>(static_cast<const T&>(*this));
				}
		};
		//! 線形の増減値による定数調整
		class LinearValue : public ValueT<LinearValue> {
			protected:
				using base_t = ValueT<LinearValue>;
				FVec4		_value,
							_step;
				rs::HLText _getValueText() const override;
			public:
				LinearValue(rs::CCoreID cid, int n);
				void set(const LValueS& v, bool bStep) override;
				void increment(float inc, int index) override;
				int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
				rs::LCValue get() const override;
				std::ostream& write(std::ostream& s) const override;
		};
		//! 級数的な定数調整
		class LogValue : public LinearValue {
			public:
				using LinearValue::LinearValue;
				void increment(float inc, int index) override;
				int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
				Value_UP clone() const override;
		};
		//! 2D方向ベクトル定数
		class Dir2D : public ValueT<Dir2D> {
			private:
				DegF		_angle,
							_step;
				Vec2 _getRaw() const;
			protected:
				rs::HLText _getValueText() const override;
			public:
				Dir2D(rs::CCoreID cid);
				void set(const LValueS& v, bool bStep) override;
				void increment(float inc, int index) override;
				int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
				rs::LCValue get() const override;
				std::ostream& write(std::ostream& s) const override;
		};
		//! 3D方向ベクトル(Yaw, Pitch)
		class Dir3D : public ValueT<Dir3D> {
			private:
				DegF		_angle[2],
							_step[2];
				Vec3 _getRaw() const;
			protected:
				rs::HLText _getValueText() const override;
			public:
				Dir3D(rs::CCoreID cid);
				void set(const LValueS& v, bool bStep) override;
				void increment(float inc, int index) override;
				int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
				rs::LCValue get() const override;
				std::ostream& write(std::ostream& s) const override;
		};
		// 定数パラメータごとに1つ定義
		struct Define {
			Name		name;		// 変数名
			Value_UP	defvalue;	// デフォルト値(値操作の方式)
			LValueG		apply;		// 値をどのように適用するか
		};
		using Define_SP = std::shared_ptr<Define>;
		using DefineV = std::vector<Define_SP>;
		// クラス名 -> 変数定義配列
		using DefineM = std::unordered_map<Name, DefineV>;
		// Luaでのクラス名 -> 調整可能なパラメータ配列
		DefineM			_define;
	private:
		class Node : public INode {
			private:
				bool		_expanded;
				rs::HLText	_mark[2];
			public:
				Node(rs::CCoreID cid, const Name& name);
				bool expand(bool b) override;
				bool isExpanded() const override;
				bool isNode() const override;
				void setPointer(Value_UP v) override;
				std::ostream& write(std::ostream& s) const override;
				int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
		};
		class Entry : public INode {
			private:
				spn::WHandle		_target;
				Value_UP			_value;
				Define_SP			_def;
				void _applyValue();
			public:
				Entry(rs::CCoreID cid, const Name& name, spn::WHandle target, const Define_SP& def);
				bool expand(bool b) override;
				bool isExpanded() const override;
				bool isNode() const override;
				void increment(float inc, int index) override;
				int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
				void set(const LValueS& v, bool bStep) override;
				void setPointer(Value_UP v) override;
				std::ostream& write(std::ostream& s) const override;
				rs::LCValue get() const override;
		};
		using Entry_SP = std::shared_ptr<Entry>;
		static Entry_SP _MakeEntry(const LValueS& param);
		using EntryV = std::vector<Entry_SP>;
		//! Tweak対象オブジェクトに関連付けられた変数
		struct ObjInfo {
			std::string	resourceName;	//!< 定数定義が読み込まれた時のファイルパス(リソース名)
			INode::SP	entry;			//!< 関連付けられたエントリ
		};
		//! Tweak対象オブジェクト -> 各種変数
		using Obj2Info = std::unordered_map<SHandle, ObjInfo>;
		Obj2Info			_obj2info;
		INode::SP			_root,
							_cursor;
		rs::CCoreID			_cid;

		rs::HLAct			_haX,		//!< カーソル操作: 左右
							_haY,		//!< カーソル操作: 上下
							_haSw,		//!< 操作モード切り替え
							_haCont,
							_haInc[4];	//!< 値操作: (X,Y,Z,W)
		// --- draw parameter ---
		Vec2				_offset;
		float				_tsize;
		float				_indent;

		//! Luaから値定義とデフォルト値を読み取る
		DefineV _loadDefine(const LValueS& d);
		//! オブジェクト名を元に_loadDefine()を呼ぶ
		const DefineV& _getDefineV(const Name& objname, lua_State* ls);
		//! オブジェクト名と変数名を元に値定義を取得
		const Define_SP& _getDefine(const Name& objname, const Name& entname, lua_State* ls);
		std::pair<INode::SP,int> _remove(const INode::SP& sp, const bool delNode);
		static void _Save(const INode::SP& ent, rs::HRW rw);
		struct St_Base;
		struct St_Cursor;	//!< カーソル移動ステート
		struct St_Value;	//!< 値改変ステート

	public:
		Tweak(const std::string& rootname, int tsize);

		// ---- input setting ----
		void setCursorAxis(HAct hX, HAct hY);
		void setIncrementAxis(HAct hC, HAct hX, HAct hY, HAct hZ, HAct hW);
		void setSwitchButton(HAct hSw);
		// ---- manupulate ----
		INode::SP makeGroup(const Name& name);
		INode::SP makeEntry(spn::SHandle obj, const Name& ent, lua_State* ls);
		void setEntryDefault(const INode::SP& sp, spn::SHandle obj, lua_State* ls);
		void setEntryFromTable(const INode::SP& sp, spn::SHandle obj, const LValueS& tbl);
		void setEntryFromFile(const INode::SP& sp, spn::SHandle obj, const std::string& file, const rs::SPLua& ls);
		//! カーソルの直後にノードを追加
		void insertNext(const INode::SP& sp);
		void insertChild(const INode::SP& sp);
		//! カーソル位置のエントリ値設定
		void setValue(const LValueS& v);
		//! カーソルが指すエントリの値操作
		void increment(float inc, int index);
		//! カーソルが指すエントリを削除
		int remove(bool delNode);
		//! オブジェクトに関連付けられたエントリを全て削除
		void removeObj(SHandle obj, bool delNode);
		// ---- draw ----
		void setFontSize(int tsize);
		//! 描画を開始するオフセット
		void setOffset(const Vec2& ofs);
		void setIndent(float w);
		// ---- cursor move ----
		//! 項目を展開
		bool expand();
		//! 項目を閉じる
		bool fold();
		//! 1階層上がる
		bool up();
		//! 下の階層へ移動
		bool down();
		//! 次のノード
		bool next();
		//! 前のノード
		bool prev();
		//! ポインタ指定によるカーソル移動
		void setCursor(const INode::SP& s);
		void saveAll();
		void save(SHandle obj, const spn::Optional<std::string>& path);
		const INode::SP& getCursor() const;
		const INode::SP& getRoot() const;
};
DEF_LUAIMPORT(Tweak)
