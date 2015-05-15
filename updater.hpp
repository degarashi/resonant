#pragma once
#include "spinner/resmgr.hpp"
#include "resmap.hpp"
#include <list>
#include "clock.hpp"
#include "luaw.hpp"
#include "handle.hpp"

namespace rs {
	using Priority = uint32_t;
	using GroupName = std::string;
	using GroupTypeId = uint32_t;		//!< Object種別Id
	using ObjName = std::string;
	using ObjTypeId = uint32_t;			//!< Object種別Id

	using GMessageStr = std::string;
	using GMessageId = uint32_t;
	using InterfaceId = uint32_t;

	// スクリプトからメッセージ文字列を受け取ったらGMessage::GetMsgId()
	class GMessage {
		private:
			using MsgMap = std::unordered_map<GMessageStr, GMessageId>;
			MsgMap		_msgMap;
			GMessageId	_msgIdCur;

		public:
			//! 遅延メッセージエントリ
			struct Packet {
				Timepoint	tmSend;
				GMessageId	msgId;
				LCValue		arg;

				Packet(const Packet& pk) = default;
				Packet(Packet&& p) = default;
				Packet(GMessageId id, const LCValue& args);
				Packet(GMessageId id, LCValue&& args);
				Packet(Duration delay, GMessageId id, const LCValue& args);
				Packet(Timepoint when, GMessageId id, const LCValue& args);
				void swap(Packet& p) noexcept;
			};
			using Queue = std::list<Packet>;

			static GMessage& Ref();
			//! メッセージIdの登録。同じメッセージを登録するとエラー
			static GMessageId RegMsgId(const GMessageStr& msg);
			//! メッセージIdの取得。存在しないメッセージの時はnoneを返す
			static spn::Optional<GMessageId> GetMsgId(const GMessageStr& msg);

			GMessage();
	};

	// ---- Objectの固有Idを生成 ----
	extern const ObjTypeId InvalidObjId;
	namespace detail {
		template <class Tag>
		struct ObjectIdT {
			static ObjTypeId GenerateObjTypeId() {
				static ObjTypeId s_id(0);
				return s_id++;
			}
		};
	}
	// 型Tはdetail::ObjectIdTにて新しいIdを生成する為に使用
	template <class T, class Tag>
	struct ObjectIdT {
		const static ObjTypeId Id;
	};
	template <class T, class Tag>
	const ObjTypeId ObjectIdT<T, Tag>::Id(detail::ObjectIdT<Tag>::GenerateObjTypeId());

	namespace idtag {
		struct Object {};
		struct Group {};
		struct DrawGroup {};
	}

	class UpdGroup;
	using UpdProc = std::function<void (HObj)>;
	using CBFindGroup = std::function<bool (HGroup)>;
	class ObjMgr;
	//! ゲームオブジェクト基底インタフェース
	class Object {
		private:
			bool		_bDestroy;
			virtual void initState();
			virtual void _doSwitchState();
		protected:
			friend class ObjMgr;
			void _initState();
		public:
			Object();
			virtual ~Object() {}
			virtual Priority getPriority() const;

			bool isDead() const;
			bool onUpdateUpd();
			virtual bool isNode() const = 0;
			//! オブジェクトの識別IDを取得
			virtual ObjTypeId getTypeId() const = 0;

			virtual void* getInterface(InterfaceId id);
			const void* getInterface(InterfaceId id) const;

			//! UpdGroupに登録された時に呼ばれる
			virtual void onConnected(HGroup hGroup);
			//! UpdGroupから削除される時に呼ばれる
			virtual void onDisconnected(HGroup hGroup);
			//! 各Objが実装するアップデート処理
			virtual void onUpdate();

			virtual void destroy();
			virtual const std::string& getName() const;

			virtual void enumGroup(CBFindGroup cb, GroupTypeId id, int depth) const;
			// ---- Message ----
			virtual LCValue recvMsg(GMessageId id, const LCValue& arg=LCValue());
			//! 特定の優先度範囲のオブジェクトを処理
			virtual void proc(UpdProc p, bool bRecursive,
								Priority prioBegin=std::numeric_limits<Priority>::lowest(),
								Priority prioEnd=std::numeric_limits<Priority>::max());

			// ---------- Object/Scene用メソッド ----------
			virtual void onDraw() const;
			// ---------- Scene用メソッド ----------
			virtual void onDown(ObjTypeId prevId, const LCValue& arg);
			virtual void onPause();
			virtual void onStop();
			virtual void onResume();
			virtual void onReStart();
	};

	//! アクティブゲームオブジェクトの管理
	class ObjMgr : public spn::ResMgrA<ObjectUP, ObjMgr> {
		using base = spn::ResMgrA<ObjectUP, ObjMgr>;
		using LHdl = typename base::LHdl;

		template <class T>
		static void NormalDeleter(void* p) {
			delete reinterpret_cast<T*>(p);
		}
		template <class P>
		LHdl _callInitialize(P&& p) {
			p->_initState();
			return base::acquire(std::move(p));
		}
		// Alignmentが8byte以上かどうかで分岐して対応した関数でメモリ確保 & 解放を行う
		template <class T, class... Ts>
		LHdl _makeObj(std::false_type, Ts&&... ts) {
			return _callInitialize(std::unique_ptr<T, void(*)(void*)>(new T(std::forward<Ts>(ts)...), &NormalDeleter<T>));
		}
		template <class T, class... Ts>
		LHdl _makeObj(std::true_type, Ts&&... ts) {
			return _callInitialize(spn::AAllocator<T>::NewUF(std::forward<Ts>(ts)...));
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
			template <class T, class... Ts>
			HLGroup makeGroup(Ts&&... ar) {
				LHdl lh = makeObj<T>(std::forward<Ts>(ar)...);
				return CastToGroup(lh.get());
			}
			template <class T, class... Ts>
			HLDObj makeDrawable(Ts&&... ar) {
				LHdl lh = makeObj<T>(std::forward<Ts>(ar)...);
				return Cast<DrawableUP>(lh.get());
			}
			template <class T, class... Ts>
			HLDGroup makeDrawGroup(Ts&&... ar) {
				LHdl lh = makeObj<T>(std::forward<Ts>(ar)...);
				return Cast<DrawGroupUP>(lh.get());
			}
			static HGroup CastToGroup(HObj hObj) {
				return Cast<GroupUP>(hObj);
			}
	};
	#define rs_mgr_obj (::rs::ObjMgr::_ref())
	/*! スクリプト対応のために型とではなく文字列とIDを関連付け
		オブジェクトグループとIDの関連付け */
	class UpdRep : public spn::Singleton<UpdRep>, public SHandleMap<std::string, HGroup> {};
	#define rs_rep_group (::rs::UpdRep::_ref())
	//! オブジェクトハンドルとIDの関連付け
	class ObjRep : public spn::Singleton<ObjRep>, public WHandleMap<std::string, WObj> {};
	#define rs_rep_obj (::rs::ObjRep::_ref())

	//! Objectのグループ管理
	class UpdGroup : public Object, public spn::EnableFromThis<HGroup> {
		private:
			static thread_local bool tls_bUpdateRoot;
			using UGVec = std::vector<UpdGroup*>;
			static UGVec	s_ug;

			using ObjV = std::vector<HLObj>;
			using ObjVH = std::vector<HObj>;
			using GroupVH = std::vector<HGroup>;

			Priority	_priority;
			ObjV		_objV;
			ObjVH		_remObj;	//!< 削除予定のオブジェクト
			GroupVH		_groupV;	//!< Idでグループ検索する時用
			int			_nParent;

			//! オブジェクト又はグループを実際に削除
			/*! onUpdate内で暗黙的に呼ばれる */
			void _doRemove();
		public:
			static void SetAsUpdateRoot();
			UpdGroup(Priority p=0);
			~UpdGroup();
			Priority getPriority() const override;

			//! オブジェクト又はグループを追加
			void addObj(HObj hObj);
			//! オブジェクト又はグループを削除
			int remObj(const ObjVH& ar);
			void remObj(HObj hObj);

			bool isNode() const override;
			void onConnected(HGroup hGroup) override;
			void onDisconnected(HGroup hGroup) override;
			void onUpdate() override;
			void onDraw() const override;
			void enumGroup(CBFindGroup cb, GroupTypeId id, int depth) const override;
			const std::string& getName() const override;
			//! グループ内のオブジェクト全てに配信
			LCValue recvMsg(GMessageId msg, const LCValue& arg) override;
			void proc(UpdProc p, bool bRecursive, Priority prioBegin, Priority prioEnd) override;

			const ObjV& getList() const;
			ObjV& getList();
			//! 子要素を全て削除
			void clear();
	};
	template <class T, class Base>
	class GroupT : public Base, public ObjectIdT<T, idtag::Group> {
		using IdT = ObjectIdT<T, idtag::Group>;
		public:
			using Base::Base;
			ObjTypeId getTypeId() const override {
				return IdT::Id;
			}
	};
	#define DefineGroupT(name, base)	class name : public ::rs::GroupT<name, base> { \
		using ::rs::GroupT<name, base>::GroupT; };

	//! UpdGroupにフレームカウンタやアイドル機能をつけたクラス
	/*! 中身は別のUpdGroupを使用 */
	class UpdTask : public Object {
		private:
			int			_idleCount,		//!< 再起動までの待ち時間
						_accum;			//!< 累積フレーム数
			HLGroup		_hlGroup;		//!< 参照しているグループハンドル

		public:
			UpdTask(Priority p, HGroup hGroup);

			// ---- アダプタ関数 ----
			bool isNode() const override;
			void onConnected(HGroup hGroup) override;
			void onDisconnected(HGroup hGroup) override;
			void enumGroup(CBFindGroup cb, GroupTypeId id, int depth) const override;
			LCValue recvMsg(GMessageId msg, const LCValue& arg) override;
			void proc(UpdProc p, bool bRecursive, Priority prioBegin, Priority prioEnd) override;

			const std::string& getName() const override;

			void onUpdate() override;
			void setIdle(int nFrame);
			int getAccum() const;
			// コピー禁止
			UpdTask& operator = (const UpdTask&) = delete;
	};

	//! Tech:Pass の組み合わせを表す
	using GL16Id = std::array<uint8_t, 2>;
	struct DrawTag {
		using TexAr = std::array<HTex, 4>;
		using VBuffAr = std::array<HVb, 4>;

		GL16Id		idTechPass;
		VBuffAr		idVBuffer;
		HIb			idIBuffer;
		TexAr		idTex;
		float		zOffset;
	};
	class DrawableObj : public Object {
		protected:
			DrawTag		_dtag;
		public:
			const DrawTag& getDTag() const;
	};
	using DLObjP = std::pair<const DrawTag*, HLDObj>;
	using DLObjV = std::vector<DLObjP>;

	struct DSort;
	using DSortSP = std::shared_ptr<DSort>;
	using DSortV = std::vector<DSortSP>;

	class GLEffect;
	// ---- Draw sort algorithms ----
	struct DSort {
		virtual bool compare(const DrawTag& d0, const DrawTag& d1) const = 0;
		virtual void apply(const DrawTag& d, GLEffect& glx);
		static void DoSort(const DSortV& alg, int cursor, typename DLObjV::iterator itr0, typename DLObjV::iterator itr1);
	};
	//! 描画ソート: Z距離の昇順
	struct DSort_Z_Asc : DSort {
		bool compare(const DrawTag& d0, const DrawTag& d1) const override;
	};
	//! 描画ソート: Z距離の降順
	struct DSort_Z_Desc : DSort {
		bool compare(const DrawTag& d0, const DrawTag& d1) const override;
	};
	//! 描画ソート: Tech&Pass Id
	struct DSort_TechPass : DSort {
		bool compare(const DrawTag& d0, const DrawTag& d1) const override;
		void apply(const DrawTag& d, GLEffect& glx) override;
	};
	namespace detail {
		class DSort_UniformPairBase {
			private:
				GLEffect*	_pFx = nullptr;
			protected:
				//! UniformIdがまだ取得されて無ければ or 前回と違うEffectの時にIdを更新
				void _refreshUniformId(GLEffect& glx, const std::string* name, int* id, size_t length);
		};
		template <size_t N>
		class DSort_UniformPair : public DSort_UniformPairBase {
			private:
				using ArStr = std::array<std::string, N>;
				using ArId = std::array<int, N>;
				//! Index -> Uniform名の対応表
				ArStr	_strUniform;
				ArId	_unifId;

				void _init(int) {}
				template <class T, class... Ts>
				void _init(int cursor, T&& t, Ts&&... ts) {
					Assert(Trap, cursor < countof(_strUniform))
					_strUniform[cursor] = std::forward<T>(t);
					_init(++cursor, std::forward<Ts>(ts)...);
				}
			protected:
				constexpr static int length = N;
				const ArId& _getUniformId(GLEffect& glx) {
					_refreshUniformId(glx, _strUniform.data(), _unifId.data(), N);
					return _unifId;
				}
				template <class... Ts>
				DSort_UniformPair(Ts&&... ts) {
					_init(0, std::forward<Ts>(ts)...);
				}
		};
	}
	//! 描画ソート: Texture
	class DSort_Texture :
		public DSort,
		public detail::DSort_UniformPair<std::tuple_size<DrawTag::TexAr>::value>
	{
		using base_t = detail::DSort_UniformPair<std::tuple_size<DrawTag::TexAr>::value>;
		public:
			using base_t::base_t;
			bool compare(const DrawTag& d0, const DrawTag& d1) const override;
			void apply(const DrawTag& d, GLEffect& glx) override;
	};

	//! 描画ソート: Vertex&Index Buffer
	struct DSort_Buffer : DSort {
		constexpr static int length = std::tuple_size<DrawTag::VBuffAr>::value;

		bool compare(const DrawTag& d0, const DrawTag& d1) const override;
		void apply(const DrawTag& d, GLEffect& glx) override;
	};
	extern const DSortSP	cs_dsort_z_asc,
							cs_dsort_z_desc,
							cs_dsort_techpass,
							cs_dsort_texture,
							cs_dsort_buffer;

	class DrawGroup : public Object {
		private:
			const DSortV	_dsort;		//!< ソートアルゴリズム優先度リスト
			const bool		_bSort;		//!< 毎フレームソートするか
			DLObjV			_dobj;

			void _doDrawSort();
		public:
			// 描画ソート方式を指定
			DrawGroup(const DSortV& ds=DSortV{}, bool bSort=false);

			void addObj(HDObj hObj);
			void remObj(HDObj hObj);
			void onUpdate() override;

			bool isNode() const override;
			void onDraw() const override;
			const std::string& getName() const override;
	};

	namespace detail {
		//! オブジェクト基底
		/*! UpdatableなオブジェクトやDrawableなオブジェクトはこれから派生して作る
			Sceneと共用 */
		template <class T, class Base, Priority P>
		class ObjectT : public Base, public ::rs::ObjectIdT<T, ::rs::idtag::Object> {
			using ThisT = ObjectT<T,Base,P>;
			using IdT = ::rs::ObjectIdT<T, ::rs::idtag::Object>;
			protected:
				struct State {
					virtual ~State() {}
					virtual ObjTypeId getStateId() const = 0;
					virtual void onUpdate(T& /*self*/) {}
					virtual LCValue recvMsg(T& /*self*/, GMessageId /*msg*/, const LCValue& /*arg*/) { return LCValue(); }
					virtual void onEnter(T& /*self*/, ObjTypeId /*prevId*/) {}
					virtual void onExit(T& /*self*/, ObjTypeId /*nextId*/) {}
					virtual void onHitEnter(T& /*self*/, HObj /*hObj*/) {}
					virtual void onHit(T& /*self*/, HObj /*hObj*/, int /*n*/) {}
					virtual void onHitExit(T& /*self*/, WObj /*whObj*/, int /*n*/) {}
					virtual void onConnected(T& /*self*/, HGroup /*hGroup*/) {}
					virtual void onDisconnected(T& /*self*/, HGroup /*hGroup*/) {}
					// --------- Scene用メソッド ---------
					virtual void onDraw(const T& /*self*/) const {}
					virtual void onDown(T& /*self*/, ObjTypeId /*prevId*/, const LCValue& /*arg*/) {}
					virtual void onPause(T& /*self*/) {}
					virtual void onStop(T& /*self*/) {}
					virtual void onResume(T& /*self*/) {}
					virtual void onReStart(T& /*self*/) {}
				};
				struct tagObjectState {};
				template <class ST, class D=State>
				struct StateT : D {
					StateT() = default;
					StateT(const D& d): D(d) {}
					StateT(D&& d): D(std::move(d)) {}
					using IdT = ::rs::ObjectIdT<ST, tagObjectState>;
					const static IdT	s_idt;
					ObjTypeId getStateId() const override { return GetStateId(); }
					static ObjTypeId GetStateId() { return IdT::Id; }
				};
				using FPState = FlagPtr<State>;

				static StateT<void> _nullState;
				static FPState _GetNullState() {
					return FPState(&_nullState, false);
				}

			private:
				bool	_bSwState = false;
				FPState	_state, _nextState;
				//! 遅延メッセージリストの先端
				GMessage::Packet* _delayMsg;

			protected:
				ObjectT(): _state(FPState(&_nullState, false)) {}

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
					setState(T::_GetNullState());
				}
				bool isNode() const override {
					return false;
				}
				ObjTypeId getTypeId() const override {
					return IdT::Id;
				}
				T& getRef() { return *reinterpret_cast<T*>(this); }
				const T& getRef() const { return *reinterpret_cast<const T*>(this); }
				void setState(FPState&& st) {
					// もし既に有効なステートがセットされていたら無視 | nullステートは常に適用
					if(!st.get() || !_bSwState) {
						_bSwState = true;
						_nextState.swap(st);
					} else {
						// ステートを2度以上セットするのはロジックが何処かおかしいと思われる
						Assert(Warn, false, "state set twice")
					}
				}
				//! 前後をdoSwitchStateで挟む
				/*! not void バージョン */
				template <class CB>
				auto _callWithSwitchState(CB&& cb, std::false_type) {
					// 前後をdoSwitchStateで挟む
					_doSwitchState();
					auto ret = std::forward<CB>(cb)();
					_doSwitchState();
					return std::move(ret);
				}
				//! 前後をdoSwitchStateで挟む
				/*! void バージョン */
				template <class CB>
				void _callWithSwitchState(CB&& cb, std::true_type) {
					_doSwitchState();
					std::forward<CB>(cb)();
					_doSwitchState();
				}
				template <class CB>
				auto _callWithSwitchState(CB&& cb) {
					// DebugBuild & NullState時に警告を出す
					AssertP(Warn, _state.get() != &_nullState, "null state detected")
					using Rt = typename std::is_same<void, decltype(std::forward<CB>(cb)())>::type;
					return _callWithSwitchState(std::forward<CB>(cb), Rt());
				}
				void _doSwitchState() override {
					if(_bSwState) {
						_bSwState = false;
						ObjTypeId prevId = InvalidObjId;
						// 現在のステートのonExitを呼ぶ
						if(_state.get()) {
							ObjTypeId nextId = (_nextState.get()) ? _nextState->getStateId() : InvalidObjId;
							_state->onExit(getRef(), nextId);
							prevId = _state->getStateId();
						}
						_state.swap(_nextState);
						_nextState.reset();
						// 次のステートのonEnterを呼ぶ
						if(_state.get())
							_state->onEnter(getRef(), prevId);
					}
				}
				State* getState() {
					return _state.get();
				}
			public:
				Priority getPriority() const override {
					return P;
				}
				ObjTypeId getStateId() const {
					return _state->getStateId();
				}
				void destroy() override {
					Base::destroy();
					// nullステートに移行
					setNullState();
				}
				//! 毎フレームの描画 (Scene用)
				void onDraw() const override {
					// ステート遷移はナシ
					_state->onDraw(getRef());
				}
				//! 上の層のシーンから抜ける時に戻り値を渡す (Scene用)
				void onDown(ObjTypeId prevId, const LCValue& arg) override final {
					_callWithSwitchState([&](){ _state->onDown(getRef(), prevId, arg); });
				}
				// ----------- 以下はStateのアダプタメソッド -----------
				void onUpdate() override {
					_callWithSwitchState([&](){ return _state->onUpdate(getRef()); });
				}
				LCValue recvMsg(GMessageId msg, const LCValue& arg) override final {
					return _callWithSwitchState([&](){ return _state->recvMsg(getRef(), msg, arg); });
				}
				//! Updaterノードツリーに追加された時に呼ばれる
				/*! DrawGroupに登録された時は呼ばれない( */
				void onConnected(HGroup hGroup) override {
					return _callWithSwitchState([&](){ return _state->onConnected(getRef(), hGroup); });
				}
				void onDisconnected(HGroup hGroup) override {
					return _callWithSwitchState([&](){ return _state->onDisconnected(getRef(), hGroup); });
				}
		};
		template <class T, class Base, Priority P>
		typename ObjectT<T, Base, P>::template StateT<void> ObjectT<T, Base, P>::_nullState;
	}
	template <class T, class Base, Priority P>
	template <class ST, class D>
	const ObjectIdT<ST,typename detail::ObjectT<T,Base,P>::tagObjectState> detail::ObjectT<T,Base,P>::StateT<ST,D>::s_idt;

	// Priority値をテンプレート指定
	template <class T, Priority P>
	class ObjectT : public detail::ObjectT<T, Object, P> {
		using detail::ObjectT<T, Object, P>::ObjectT;
	};
	// PriorityはUpdateObjと兼用の場合に使われる
	template <class T, Priority P=0>
	class DrawableObjT : public detail::ObjectT<T, DrawableObj, P> {
		using detail::ObjectT<T, DrawableObj, P>::ObjectT;
	};
}
