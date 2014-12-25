#pragma once
#include "spinner/resmgr.hpp"
#include "resmap.hpp"
#include <boost/variant.hpp>
#include <list>
#include "clock.hpp"
#include "luaw.hpp"
#include "handle.hpp"

namespace rs {
	using Priority = uint32_t;
	using GroupName = std::string;
	using GroupID = uint32_t;			//!< UpdateGroup名前ID(ユーザー任意)
	using ObjTypeID = uint32_t;			//!< Object種別ID
	using ObjName = std::string;
	using ObjID = uint32_t;				//!< Object名前ID(ユーザー任意)

	using DrawName = std::string;
	class DTagCollect;
	using GMessageID = uint32_t;
	using GMessageStr = std::string;

	#define mgr_gobj (::rs::ObjMgr::_ref())
	//! アクティブゲームオブジェクトの管理
	class ObjMgr : public spn::ResMgrA<UPObject, ObjMgr> {
		using base = spn::ResMgrA<UPObject, ObjMgr>;
		using LHdl = typename base::LHdl;

		template <class T>
		static void NormalDeleter(void* p) {
			delete reinterpret_cast<T*>(p);
		}
		// Alignmentが8byte以上かどうかで分岐して対応した関数でメモリ確保 & 解放を行う
		template <class T, class... Ts>
		LHdl _makeObj(std::false_type, Ts&&... ts) {
			return base::acquire(std::move(UPObject(new T(std::forward<Ts>(ts)...), &NormalDeleter<T>)));
		}
		template <class T, class... Ts>
		LHdl _makeObj(std::true_type, Ts&&... ts) {
			return base::acquire(std::move(T::NewUF_Args(std::forward<Ts>(ts)...)));
		}

		public:
			// デフォルトのリソース作成関数は無効化
			void acquire() = delete;
			void emplace() = delete;
			// オブジェクトの追加にはこっちを使う
			template <class T, class... Ts>
			LHdl makeObj(Ts&&... ar) {
				return _makeObj<T>(typename spn::NType<alignof(T), 8>::great(), std::forward<Ts>(ar)...);
			}
	};

	#define mgr_upd (::rs::UpdMgr::_ref())
	//! アップデートグループの管理
	class UpdMgr : public spn::ResMgrA<UPUpdCh, UpdMgr> {};

	// スクリプトからメッセージ文字列を受け取ったらGMessage::GetMsgID()
	class GMessage {
		using MsgMap = std::unordered_map<GMessageStr, GMessageID>;
		MsgMap		_msgMap;
		GMessageID	_msgIDCur = 0;

		public:
			//! 遅延メッセージエントリ
			struct Packet {
				Timepoint	tmSend;
				GMessageID	msgID;
				LCValue		arg;

				Packet(const Packet& pk) = default;
				Packet(GMessageID id, const LCValue& args): msgID(id), arg(args) {}
				Packet(GMessageID id, LCValue&& args): msgID(id), arg(std::move(args)) {}
				Packet(Packet&& p) { swap(p); }
				Packet(Duration delay, GMessageID id, const LCValue& args): Packet(id, args) {
					tmSend = Clock::now() + delay;
				}
				Packet(Timepoint when, GMessageID id, const LCValue& args): Packet(id, args) {
					tmSend = when;
				}
				void swap(Packet& p) noexcept {
					std::swap(tmSend, p.tmSend);
					std::swap(msgID, p.msgID);
					std::swap(arg, p.arg);
				}
			};
			using Queue = std::list<Packet>;

			static GMessage& Ref();
			//! メッセージIDの登録。同じメッセージを登録するとエラー
			static GMessageID RegMsgID(const GMessageStr& msg);
			//! メッセージIDの取得。存在しないメッセージの時はnoneを返す
			static spn::Optional<GMessageID> GetMsgID(const GMessageStr& msg);
	};

	//! GameObj - UpdBase 共通基底
	class GOBase {
		bool	_bDestroy = false;

		public:
			bool isDead() const { return _bDestroy; }
			virtual void destroy() { _bDestroy = true; }
			bool onUpdateUpd();
			virtual LCValue recvMsg(GMessageID /*id*/, const LCValue& /*arg*/=LCValue()) { return LCValue(); }
			//! 各Objが実装するアップデート処理
			virtual void onUpdate() = 0;
			virtual void onDestroy() {}
			virtual ~GOBase() {}
	};

	//! ゲームオブジェクト基底インタフェース
	class Object : public GOBase {
		enum class Form {
			Invalid,
			Circle
		};
		public:
			virtual const char* getLuaName() const { return "Object"; }
			//! 描画セッティング等をチェックして適切なタグを出力
			virtual void getDTag(DTagCollect& /*dst*/) const {}
			//! 描画タグに記載がしてある類の描画セッティングをする
			virtual void prepareDraw() const {}
			//! オブジェクト描画用メソッド
			virtual void onDraw() const {}

			virtual Form getForm() const { return Form::Invalid; }
			virtual void onHitEnter(HGbj /*hGbj*/) {}		//!< 初回衝突
			virtual void onHit(HGbj /*hGbj*/, int /*n*/) {}		//!< 2フレーム目以降
			//! 衝突が終わった最初のフレーム
			/*! 既にオブジェクトが存在しない可能性がある為弱ハンドルを渡す */
			virtual void onHitExit(WGbj /*whGbj*/, int /*n*/) {}

			virtual ObjTypeID getTypeID() const = 0;					//!< オブジェクトの識別IDを取得
			// ---------- Scene用メソッド ----------
			virtual void onDraw() {}
			virtual void onDown(ObjTypeID /*prevID*/, const LCValue& /*arg*/) {}
			virtual void onPause() {}
			virtual void onStop() {}
			virtual void onResume() {}
			virtual void onReStart() {}
	};

	ObjTypeID GenerateObjTypeID();
	const static ObjTypeID InvalidObjID(~0);
	template <class T>
	struct ObjectIDT {
		const static ObjTypeID ID;
	};
	template <class T>
	const ObjTypeID ObjectIDT<T>::ID(GenerateObjTypeID());

	class UpdBase;
	class UpdChild;
	class UpdGroup;
	struct IUpdProc {
		virtual void updateProc(UpdBase* u) const = 0;
	};

	//! UpdateTask基底インタフェース
	class UpdBase : public GOBase {
		Priority		_priority = 0;
		UpdChild*		_parent = nullptr;

		public:
			UpdBase();
			UpdBase(Priority prio);
			UpdBase(const UpdBase&) = delete;
			UpdBase(UpdBase&& u);

			Priority getPriority() const;
			UpdChild* getParent() const;
			void setParent(UpdChild* p);
			UpdBase& operator = (const UpdBase&) = delete;
			UpdBase& operator = (UpdBase&& u);

			virtual void proc(Priority prioBegin, Priority prioEnd, const IUpdProc* p) = 0;
			virtual void proc(const IUpdProc* p) = 0;
			virtual bool isNode() const = 0;
			virtual UpdGroup* findGroup(GroupID id) const = 0;
			virtual UpdBase* _clone() const = 0;
	};
	using UpdArray = std::vector<UpdBase*>;
	using UpdList = std::list<UpdBase*>;

	// スクリプト対応のために型とではなく文字列とIDを関連付け
	// オブジェクトグループとIDの関連付け
	#define rep_upd (::rs::UpdRep::_ref())
	class UpdRep : public spn::Singleton<UpdRep>, public SHandleMap<std::string, HUpd> {};
	// オブジェクトハンドルとIDの関連付け
	#define rep_gobj (::rs::ObjRep::_ref())
	class ObjRep : public spn::Singleton<ObjRep>, public WHandleMap<std::string, WGbj> {};

	//! UpdBase(Child)の集合体
	class UpdChild {
		UpdList		_child;			//!< このクラスがOwnershipを持つポインタリスト
		int			_nGroup = 0;	//!< 子要素にグループが幾つあるか
		std::string	_name;

		public:
			UpdChild(const std::string& name = std::string());
			const std::string& getName() const;
			void addObj(UpdBase* upd);
			//! このリストから外し、メモリも開放する UpdGroupから呼ぶ
			void remObjs(const UpdArray& ar);
			UpdGroup* findGroup(GroupID id) const;
			const UpdList& getList() const;
			UpdList& getList();
			void clear();
	};

	//! UpdChildにフレームカウンタやアイドル機能をつけたクラス
	class UpdGroup : public UpdBase {
		int			_idleCount = 0,		//!< 再起動までの待ち時間
					_accum = 0;			//!< 累積フレーム数
		HLUpd		_child;				//!< 参照しているグループハンドル

		//! 削除予定のオブジェクト
		UpdArray	_remObj;

		using UGVec = std::vector<UpdGroup*>;
		static UGVec	s_ug;

		//! オブジェクト又はグループを実際に削除
		void _doRemove();

		public:
			//! 子グループを新規作成(名無し) for タスクルート
			UpdGroup(Priority prio);
			//! 子グループを流用 for 他グループ流用
			UpdGroup(Priority prio, HUpd hUpd);

			UpdGroup* findGroup(GroupID id) const override;
			void onUpdate() override;
			//! 子要素を全て削除
			void clear();
			//! オブジェクト又はグループを追加
			void addObj(UpdBase* upd);
			//! オブジェクトをラップして追加
			void addObj(Priority prio, HGbj hGbj);
			//! オブジェクト又はグループを削除(メモリ解放)
			void remObj(UpdBase* upd);
			const std::string& getGroupName() const;
			//! 特定の優先度範囲のオブジェクトを処理
			void proc(Priority prioBegin, Priority prioEnd, const IUpdProc* p) override;
			void proc(const IUpdProc* p) override;
			//! グループ内のオブジェクト全てに配信
			LCValue recvMsg(GMessageID msg, const LCValue& arg) override;
			//! 名前を新たに付加してグループ複製
			/*! 下層のグループは複製されず参照カウントを加算 */
			HLUpd clone() const;
			UpdBase* _clone() const override;
			HUpd getChild() const;
			bool isNode() const override;
			void setIdle(int nFrame);
			int getAccum() const;
			// コピー禁止
			UpdGroup& operator = (const UpdGroup& u) = delete;
	};

	//! HGbjをタスクツリーに登録する為のプロキシ
	/*! タスクツリーに登録するオブジェクトは全てハンドル管理 */
	class UpdProxy : public UpdBase {
		HLGbj	_hlGbj;
		public:
			UpdProxy(Priority prio, HGbj hGbj);

			bool isNode() const override;
			void proc(Priority prioBegin, Priority prioEnd, const IUpdProc* p) override;
			void proc(const IUpdProc* p) override;
			UpdGroup* findGroup(GroupID id) const override;
			UpdBase* _clone() const override;
			// ----------- 以下はGobjのアダプタメソッド -----------
			void onUpdate() override;
			void onDestroy() override;
			LCValue recvMsg(GMessageID msg, const LCValue& arg) override;
	};

	//! オブジェクト基底
	/*! UpdatableなオブジェクトやDrawableなオブジェクトはこれから派生して作る
		Sceneと共用 */
	template <class T>
	class ObjectT : public Object, public ObjectIDT<T> {
		protected:
			struct State {
				virtual ~State() {}
				virtual ObjTypeID getStateID() const = 0;
				virtual void onUpdate(T& /*self*/) {}
				virtual LCValue recvMsg(T& /*self*/, GMessageID /*msg*/, const LCValue& /*arg*/) { return LCValue(); }
				virtual void onEnter(T& /*self*/, ObjTypeID /*prevID*/) {}
				virtual void onExit(T& /*self*/, ObjTypeID /*nextID*/) {}
				virtual void onHitEnter(T& /*self*/, HGbj /*hGbj*/) {}
				virtual void onHit(T& /*self*/, HGbj /*hGbj*/, int /*n*/) {}
				virtual void onHitExit(T& /*self*/, WGbj /*whGbj*/, int /*n*/) {}
				// --------- Scene用メソッド ---------
				virtual void onDraw(T& /*self*/) {}
				virtual void onDown(T& /*self*/, ObjTypeID /*prevID*/, const LCValue& /*arg*/) {}
				virtual void onPause(T& /*self*/) {}
				virtual void onStop(T& /*self*/) {}
				virtual void onResume(T& /*self*/) {}
				virtual void onReStart(T& /*self*/) {}
			};
			template <class ST>
			struct StateT : State, ObjectIDT<ST> {
				ObjTypeID getStateID() const override { return ObjectIDT<ST>::ID; }
			};
			using FPState = FlagPtr<State>;

			static StateT<void> _nullState;
			static FPState getNullState() {
				return useState(&_nullState);
			}

		private:
			bool	_bSwState = false;
			FPState	_state, _nextState;
			//! 遅延メッセージリストの先端
			GMessage::Packet* _delayMsg;

		protected:
			// ステートをNewするためのヘルパー関数
			template <class ST, class... Args>
			void setStateNew(Args... args) {
				setState(FPState(new ST(args...), true));
			}
			// ステートを再利用するヘルパー関数
			template <class ST>
			void setStateUse(ST* st) {
				setState(FPState(st, false));
			}
			void setNullState() {
				setStateUse(&_nullState);
			}
			ObjTypeID getTypeID() const override { return ObjectIDT<T>::ID; }

			ObjectT(Priority /*prio*/=0): _state(FPState(&_nullState, false)) {}
			T& getRef() { return *reinterpret_cast<T*>(this); }
			void setState(FPState&& st) {
				// もし既に有効なステートがセットされていたら無視 | nullステートは常に適用
				if(!st.get() || !_bSwState) {
					// 初回のステート設定だったら即適用
					_bSwState = true;
					_nextState.swap(st);
					if(_state == &_nullState)
						_doSwitchState();
				}
			}
			void _doSwitchState() {
				if(_bSwState) {
					_bSwState = false;
					ObjTypeID prevID = InvalidObjID;
					// 現在のステートのonExitを呼ぶ
					if(_state.get()) {
						ObjTypeID nextID = (_nextState.get()) ? _nextState->getStateID() : InvalidObjID;
						_state->onExit(getRef(), nextID);
						prevID = _state->getStateID();
					}
					_state.swap(_nextState);
					_nextState.reset();
					// 次のステートのonEnterを呼ぶ
					if(_state.get())
						_state->onEnter(getRef(), prevID);
				}
			}
			State* getState() {
				return _state.get();
			}
		public:
			void destroy() override {
				Object::destroy();
				// nullステートに移行
				setNullState();
			}
			//! 毎フレームの描画 (Scene用)
			void onDraw() override {
				_state->onDraw(getRef());
				AssertP(Trap, !_bSwState)
			}
			//! 上の層のシーンから抜ける時に戻り値を渡す (Scene用)
			void onDown(ObjTypeID prevID, const LCValue& arg) override final {
				_state->onDown(getRef(), prevID, arg);
				_doSwitchState();
			}
			// ----------- 以下はStateのアダプタメソッド -----------
			void onUpdate() override {
				_state->onUpdate(getRef());
				_doSwitchState();
			}
			void onHitEnter(HGbj hGbj) override final {
				_state->onHitEnter(getRef(), hGbj);
				_doSwitchState();
			}
			void onHit(HGbj hGbj, int n) override final {
				_state->onHit(getRef(), hGbj, n);
				_doSwitchState();
			}
			void onHitExit(WGbj wGbj, int n) override final {
				_state->onHitExit(getRef(), wGbj, n);
				_doSwitchState();
			}
			LCValue recvMsg(GMessageID msg, const LCValue& arg) override final {
				LCValue ret(_state->recvMsg(getRef(), msg, arg));
				_doSwitchState();
				return std::move(ret);
			}
	};
	template <class T>
	typename ObjectT<T>::template StateT<void> ObjectT<T>::_nullState;
}
