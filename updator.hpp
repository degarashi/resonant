#pragma once
#include "spinner/resmgr.hpp"
#include "resmap.hpp"
#include <boost/variant.hpp>
#include <list>
#include "clock.hpp"

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
	class Object;
	using UPObject = std::unique_ptr<Object>;
	class ObjMgr : public spn::ResMgrA<UPObject, ObjMgr> {};
	DEF_HANDLE(ObjMgr, Gbj, UPObject)

	#define mgr_upd (::rs::UpdMgr::_ref())
	class UpdChild;
	using UPUpdCh = std::unique_ptr<UpdChild>;
	class UpdMgr : public spn::ResMgrA<UPUpdCh, UpdMgr> {};
	DEF_HANDLE(UpdMgr, Upd, UPUpdCh)

	class GVec;
	class GMap;
	using Variant = boost::variant<boost::blank, int32_t, float, GMessageStr, GVec*, GMap*,
									HGbj, HUpd>;
	class GVec : public std::vector<Variant> {
		using std::vector<Variant>::vector;
	};
	class GMap : public std::unordered_map<std::string, Variant> {
		using std::unordered_map<std::string, Variant>::unordered_map;
	};

	class GMessage {
		using MsgMap = std::unordered_map<GMessageStr, GMessageID>;
		static MsgMap		_msgMap;
		static GMessageID	_msgIDCur;

		public:
			//! 遅延メッセージエントリ
			struct Packet {
				Timepoint	tmSend;
				GMessageID	msgID;
				Variant		arg;

				Packet(const Packet& pk) = default;
				Packet(GMessageID id, const Variant& args): msgID(id), arg(args) {}
				Packet(GMessageID id, Variant&& args): msgID(id), arg(args) {}
				Packet(Packet&& p) { swap(p); }
				template <class R, class P>
				Packet(std::chrono::duration<R,P> delay, GMessageID id, const Variant& args): Packet(id, args) {
					tmSend = Clock::now() + delay;
				}
				template <class C, class D>
				Packet(std::chrono::time_point<C,D> when, GMessageID id, const Variant& args): Packet(id, args) {
					tmSend = when;
				}
				void swap(Packet& p) noexcept {
					std::swap(tmSend, p.tmSend);
					std::swap(msgID, p.msgID);
					std::swap(arg, p.arg);
				}
			};
			using Queue = std::list<Packet>;
			//! メッセージIDの登録。既に同じ物が登録されている場合はそれを返す
			static GMessageID getMsgID(const GMessageStr& msg);
	};

	//! GameObj - UpdBase 共通基底
	class GOBase {
		bool	_bDestroy = false;

		public:
			bool isDead() const { return _bDestroy; }
			virtual void destroy() { _bDestroy = true; }
			bool onUpdateUpd();
			virtual Variant recvMsg(const GMessageStr& msg, const Variant& arg) { return boost::blank(); }
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
			//! 描画セッティング等をチェックして適切なタグを出力
			virtual void getDTag(DTagCollect& dst) const {}
			//! 描画タグに記載がしてある類の描画セッティングをする
			virtual void prepareDraw() const {}
			//! オブジェクト描画用メソッド
			virtual void onDraw() const {}

			virtual Form getForm() const { return Form::Invalid; }
			virtual void onHitEnter(HGbj hGbj) {}		//!< 初回衝突
			virtual void onHit(HGbj hGbj, int n) {}		//!< 2フレーム目以降
			//! 衝突が終わった最初のフレーム
			/*! 既にオブジェクトが存在しない可能性がある為弱ハンドルを渡す */
			virtual void onHitExit(WGbj whGbj, int n) {}

			virtual ObjTypeID getTypeID() const = 0;					//!< オブジェクトの識別IDを取得
			// ---------- Scene用メソッド ----------
			virtual void onDraw() {}
			virtual void onDown(ObjTypeID prevID, const Variant& arg) {}
			virtual void onPause() {}
			virtual void onStop() {}
			virtual void onResume() {}
			virtual void onReStart() {}
	};

	ObjTypeID GenerateObjTypeID();
	template <class T>
	class ObjectIDT : public Object {
		public:
			const static ObjTypeID ID;
			ObjTypeID getTypeID() const override { return ID; }
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

	#define rep_upd (::rs::UpdRep::_ref())
	class UpdRep : public spn::Singleton<UpdRep>, public SHandleMap<GroupID, HUpd>, public IDMap<GroupName, GroupID> {};
	#define rep_gobj (::rs::ObjRep::_ref())
	class ObjRep : public spn::Singleton<ObjRep>, public WHandleMap<ObjID, WGbj>, public IDMap<ObjName, ObjID> {};

	//! UpdBase(Child)の集合体
	class UpdChild {
		UpdList		_child;			//!< このクラスがOwnershipを持つポインタリスト
		int			_nGroup = 0;	//!< 子要素にグループが幾つあるか
		GroupID		_id;

		public:
			UpdChild(GroupID id = UpdRep::InvalidID);
			GroupID getID() const;
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
			void addObj(Priority prio, Object* obj);
			void addObj(Priority prio, HGbj hGbj);
			//! オブジェクト又はグループを削除(メモリ解放)
			void remObj(UpdBase* upd);
			GroupID getGroupID() const;
			//! 特定の優先度範囲のオブジェクトを処理
			void proc(Priority prioBegin, Priority prioEnd, const IUpdProc* p) override;
			void proc(const IUpdProc* p) override;
			//! グループ内のオブジェクト全てに配信
			Variant recvMsg(const std::string& msg, const Variant& arg) override;
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
			Variant recvMsg(const std::string& msg, const Variant& arg) override;
	};

	//! オブジェクト基底
	/*! UpdatableなオブジェクトやDrawableなオブジェクトはこれから派生して作る
		Sceneと共用 */
	template <class T>
	class ObjectT : public ObjectIDT<T> {
		protected:
			using StateID = uint32_t;
			constexpr static int InvalidStateID = ~0;
			class State {
				StateID _stateID;

				public:
					State(StateID id): _stateID(id) {}
					virtual ~State() {}
					StateID getID() const { return _stateID; }

					virtual void onUpdate(T& self) {}
					virtual Variant recvMsg(T& self, const std::string& msg, Variant arg) { return Variant(); }
					virtual void onEnter(T& self, StateID prevID) {}
					virtual void onExit(T& self, StateID nextID) {}
					virtual void onHitEnter(T& self, HGbj hGbj) {}
					virtual void onHit(T& self, HGbj hGbj, int n) {}
					virtual void onHitExit(T& self, WGbj whGbj, int n) {}
					// --------- Scene用メソッド ---------
					virtual void onDraw(T& self) {}
					virtual void onDown(T& self, ObjTypeID prevID, const Variant& arg) {}
					virtual void onPause(T& self) {}
					virtual void onStop(T& self) {}
					virtual void onResume(T& self) {}
					virtual void onReStart(T& self) {}
			};
			using FPState = FlagPtr<State>;

			static State _nullState;
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

			ObjectT(Priority prio=0): _state(FPState(&_nullState, false)) {}
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
					StateID prevID = InvalidStateID;
					// 現在のステートのonExitを呼ぶ
					if(_state.get()) {
						StateID nextID = (_nextState.get()) ? _nextState->getID() : InvalidStateID;
						_state->onExit(getRef(), nextID);
						prevID = _state->getID();
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
			void onDraw() {
				_state->onDraw(getRef());
				AssertP(Trap, !_bSwState)
			}
			//! 上の層のシーンから抜ける時に戻り値を渡す (Scene用)
			void onDown(ObjTypeID prevID, const Variant& arg) override final {
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
			Variant recvMsg(const std::string& msg, const Variant& arg) override final {
				Variant ret(_state->recvMsg(getRef(), msg, arg));
				_doSwitchState();
				return std::move(ret);
			}
	};
	template <class T>
	typename ObjectT<T>::State ObjectT<T>::_nullState(0);
}
