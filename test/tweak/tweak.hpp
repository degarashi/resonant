#pragma once
#include "../luaw.hpp"
#include "../updater.hpp"
#include "test/tweak/value.hpp"
#include "test/tweak/node.hpp"
#include "stext.hpp"

namespace tweak {
	using Vec3 = spn::Vec3;
	using Vec4 = spn::Vec4;
	using DegF = spn::DegF;
	using RectF = spn::RectF;
	using HAct = rs::HAct;
	using SHandle = spn::SHandle;
	class Tweak : public rs::DrawableObjT<Tweak> {
		private:
			using DefineV = std::vector<Define_SP>;
			// クラス名 -> 変数定義配列
			using DefineM = std::unordered_map<Name, DefineV>;
			// Luaでのクラス名 -> 調整可能なパラメータ配列
			DefineM			_define;

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
			STextPack			_stext;
			// --- draw parameter ---
			struct {
				Vec2			offset;
				spn::SizeF		size;
				float			tsize,
								indent;
			} _dparam;
			//! Luaから値定義とデフォルト値を読み取る
			static DefineV _LoadDefine(const LValueS& d, rs::CCoreID cid);
			//! オブジェクト名を元に_loadDefine()を呼ぶ
			const DefineV& _getDefineV(const Name& objname, lua_State* ls);
			//! オブジェクト名と変数名を元に値定義を取得
			const Define_SP& _getDefine(const Name& objname, const Name& entname, lua_State* ls);
			std::pair<INode::SP,int> _remove(const INode::SP& sp, const bool delNode);
			static void _Save(const INode::SP& ent, rs::HRW rw);
			struct St_Base : StateT<St_Base> {
				void onDraw(const Tweak& self, rs::IEffect& e) const override;
			};
			//! カーソル移動ステート
			struct St_Cursor : StateT<St_Cursor, St_Base> {
				St_Cursor();
				void onUpdate(Tweak& self) override;
			};
			//! 値改変ステート
			struct St_Value : StateT<St_Value, St_Base> {
				St_Value();
				void onUpdate(Tweak& self) override;
			};
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
			void setSize(const spn::SizeF& s);
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
}
DEF_LUAIMPORT(::tweak::Tweak)
