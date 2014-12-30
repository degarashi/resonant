#pragma once
#include "sdlwrap.hpp"
#include "luaw.hpp"

namespace rs {
	#define mgr_lsys (rs::LSysFunc::_ref())
	class LSysFunc : public spn::Singleton<LSysFunc> {
		class LoadThread : public Thread<void (LSysFunc&)> {
			protected:
				void run(LSysFunc& self) override;
			public:
				LoadThread();
		};
		struct Block {
			using Pair = std::pair<std::string, spn::URI>;
			using URIVec = std::vector<Pair>;
			URIVec		uriVec;
			int			current;		//!< 現在読み込み中のリソース
			SPLCTable	result;

			Block();
			Block(Block&& blk);
		};
		// 問い合わせID : 非同期ロードクラス
		using ASync = std::unordered_map<UInt_MainT, Block>;

		UInt_MainT		_serialCur,		//!< 次に生成されるタスクの通し番号
						_procCur;		//!< 現在処理中のタスク番号(processing時)
		float			_progress;		//!< 進捗状況(0-1)
		enum class State {
			Idle = 0x00,			//!< 次のタスク待ち
			Processing = 0x01		//!< タスク処理中
		} _state;

		LoadThread		_thread;		//!< 読み込み処理を行うスレッド
		ASync			_async;			//!< 非同期タスクキュー
		bool			_bLoop;
		Mutex			_mutex;			//!< CondV, またはクラス全体の同期
		CondV			_cond;			//!< 非同期タスクがアイドルになった時に起こす
		public:
			LSysFunc();
			~LSysFunc();
			//! 拡張子で型を判別してリソース読み込み
			spn::LHandle loadResource(const std::string& urisrc);
			//! Luaのテーブルからリソースを一括読み込み
			LCTable loadResources(LValueG tbl);
			//! 非同期で読み込み
			UInt_MainT loadResourcesASync(LValueG tbl);
			float queryProgress(UInt_MainT id);
			//! 非同期読み込みの結果を取得
			/*! 一度取得すると消去される
				\retval LCTable
				\retval nil (該当IDがない場合 or 既に取得されている場合) */
			LCValue getResult(UInt_MainT id);
			//! キューに溜まってるタスク数
			int getNTask();
			//! (主にデバッグ用途)
			void sleep(UInt_MainT ms) const;
	};
	DEF_LUAIMPORT(LSysFunc)
}

