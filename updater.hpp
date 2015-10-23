#pragma once
#include "spinner/resmgr.hpp"
#include "resmap.hpp"
#include <list>
#include "clock.hpp"
#include "luaw.hpp"
#include "handle.hpp"
#include "glx_id.hpp"
#include "drawtag.hpp"

namespace rs {
	using Priority = uint32_t;
	constexpr static Priority DefaultPriority = std::numeric_limits<Priority>::max() / 2;
	using GroupName = std::string;
	using GroupTypeId = uint32_t;		//!< Object種別Id
	using ObjName = std::string;
	using ObjTypeId = uint32_t;			//!< Object種別Id

	using GMessageStr = std::string;
	using InterfaceId = uint32_t;

	class GMessage {
		public:
			//! 遅延メッセージエントリ
			struct Packet {
				Timepoint	tmSend;
				GMessageStr	msgStr;
				LCValue		arg;

				Packet(const Packet& pk) = default;
				Packet(Packet&& p) = default;
				Packet(const GMessageStr& msg, const LCValue& args);
				Packet(const GMessageStr& msg, LCValue&& args);
				Packet(Duration delay, const GMessageStr& msg, const LCValue& args);
				Packet(Timepoint when, const GMessageStr& msg, const LCValue& args);
				void swap(Packet& p) noexcept;
			};
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

	struct IEffect;
	class UpdGroup;
	using UpdProc = std::function<void (HObj)>;
	using CBFindGroup = std::function<bool (HGroup)>;
	class ObjMgr;
	//! ゲームオブジェクト基底インタフェース
	class Object : public spn::EnableFromThis<HObj> {
		private:
			bool _bDestroy;
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
			virtual void onUpdate(bool bFirst);

			virtual void destroy();
			virtual const std::string& getName() const;

			virtual void enumGroup(CBFindGroup cb, GroupTypeId id, int depth) const;
			// ---- Message ----
			virtual LCValue recvMsg(const GMessageStr& msg, const LCValue& arg=LCValue());
			virtual LCValue recvMsgLua(const GMessageStr& msg, const LCValue& arg=LuaNil());
			//! 特定の優先度範囲のオブジェクトを処理
			virtual void proc(UpdProc p, bool bRecursive,
								Priority prioBegin=std::numeric_limits<Priority>::lowest(),
								Priority prioEnd=std::numeric_limits<Priority>::max());

			// ---------- Object/Scene用メソッド ----------
			virtual void onDraw(IEffect& e) const;
			// ---------- Scene用メソッド ----------
			virtual void onDown(ObjTypeId prevId, const LCValue& arg);
			virtual void onPause();
			virtual void onStop();
			virtual void onResume();
			virtual void onReStart();
	};
	class IScene : public Object {
		public:
			virtual HGroup getUpdGroup() const;
			virtual HDGroup getDrawGroup() const;
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
		decltype(auto) _makeHandle(P&& p) {
			auto* ptr = p.get();
			return std::make_pair(base::acquire(std::move(p)), ptr);
		}
		// Alignmentが8byte以上かどうかで分岐して対応した関数でメモリ確保 & 解放を行う
		template <class T, class... Ts>
		decltype(auto) _makeObj(std::false_type, Ts&&... ts) {
			return _makeHandle(std::unique_ptr<T, void(*)(void*)>(new T(std::forward<Ts>(ts)...), &NormalDeleter<T>));
		}
		template <class T, class... Ts>
		decltype(auto) _makeObj(std::true_type, Ts&&... ts) {
			return _makeHandle(spn::AAllocator<T>::NewUF(std::forward<Ts>(ts)...));
		}
		SPLua _lua;

		public:
			void setLua(const SPLua& ls);
			const SPLua& getLua() const;
			// デフォルトのリソース作成関数は無効化
			void acquire() = delete;
			void emplace() = delete;
			// アラインメントによってスマートポインタ型を分けるので
			// UpdGroupに登録するオブジェクトの作成にはこの関数を使う
			template <class T, class... Ts>
			std::pair<LHdl, T*> makeObj(Ts&&... ar) {
				return _makeObj<T>(typename spn::NType<alignof(T), 8>::great(),
									std::forward<Ts>(ar)...);
			}
			template <class T, class... Ts>
			std::pair<HLGroup,T*> makeGroup(Ts&&... ar) {
				auto lhp = makeObj<T>(std::forward<Ts>(ar)...);
				return std::make_pair(CastToGroup(lhp.first.get()), lhp.second);
			}
			template <class T, class... Ts>
			std::pair<HLScene,T*> makeScene(Ts&&... ar) {
				auto lhp = makeObj<T>(std::forward<Ts>(ar)...);
				return std::make_pair(Cast<SceneUP>(lhp.first.get()), lhp.second);
			}
			template <class T, class... Ts>
			std::pair<HLDObj,T*> makeDrawable(Ts&&... ar) {
				auto lhp = makeObj<T>(std::forward<Ts>(ar)...);
				return std::make_pair(Cast<DrawableUP>(lhp.first.get()), lhp.second);
			}
			template <class T, class... Ts>
			std::pair<HLDGroup,T*> makeDrawGroup(Ts&&... ar) {
				auto lhp = makeObj<T>(std::forward<Ts>(ar)...);
				return std::make_pair(Cast<DrawGroupUP>(lhp.first.get()), lhp.second);
			}
			static HGroup CastToGroup(HObj hObj);
			const std::string& getResourceName(spn::SHandle sh) const override;
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
	class UpdGroup : public Object {
		private:
			static thread_local bool tls_bUpdateRoot;
			using UGVec = std::vector<UpdGroup*>;
			static UGVec	s_ug;

			using ObjV = std::vector<HLObj>;
			using ObjVP = std::vector<std::pair<Priority, HLObj>>;
			using ObjVH = std::vector<HObj>;
			using GroupVH = std::vector<HGroup>;

			Priority	_priority;
			ObjVP		_objV;		//!< 優先度は実行中に変わることはないのでキャッシュしておく
			ObjVP		_addObj;	//!< (onUpdateが終った後に)追加予定のオブジェクト
			ObjVH		_remObj;	//!< (onUpdateが終った後に)削除予定のオブジェクト
			GroupVH		_groupV;	//!< Idでグループ検索する時用
			int			_nParent;

			//! Add,RemoveListが空なら自身をグローバルリストに登録
			void _registerUGVec();
			//! オブジェクト又はグループを実際に追加、削除
			/*! onUpdate内で暗黙的に呼ばれる */
			void _doAddRemove();
		public:
			static void SetAsUpdateRoot();
			UpdGroup(Priority p=DefaultPriority);
			~UpdGroup();
			Priority getPriority() const override;

			//! オブジェクト又はグループを追加
			void addObj(HObj hObj);
			void addObjPriority(HObj hObj, Priority p);
			//! オブジェクト又はグループを削除
			void remObj(HObj hObj);

			bool isNode() const override;
			void onConnected(HGroup hGroup) override;
			void onDisconnected(HGroup hGroup) override;
			void onUpdate(bool bFirst) override;
			void onDraw(IEffect& e) const override;
			void enumGroup(CBFindGroup cb, GroupTypeId id, int depth) const override;
			const std::string& getName() const override;
			//! グループ内のオブジェクト全てに配信
			LCValue recvMsg(const GMessageStr& msg, const LCValue& arg) override;
			LCValue recvMsgLua(const GMessageStr& msg, const LCValue& arg) override;
			void proc(UpdProc p, bool bRecursive, Priority prioBegin, Priority prioEnd) override;

			const ObjVP& getList() const;
			ObjVP& getList();
			//! 子要素を全て削除
			void clear();
	};
	template <class T, class Base>
	class GroupT : public Base, public ObjectIdT<T, idtag::Group> {
		using IdT = ObjectIdT<T, idtag::Group>;
		public:
			using Base::Base;
			ObjTypeId getTypeId() const override { return GetTypeId(); }
			static ObjTypeId GetTypeId() { return IdT::Id; }
	};
	#define DefineUpdGroup(name)	class name : public ::rs::GroupT<name, ::rs::UpdGroup> { \
		using base_t = ::rs::GroupT<name, ::rs::UpdGroup>; \
		using base_t::base_t; };
	#define DefineDrawGroupBase(name, base)	class name : public ::rs::GroupT<name, base> { \
		using base_t = ::rs::GroupT<name, base>; \
		public: \
			template <class... Ts> \
			name(Ts&&... ts): base_t(std::forward<Ts>(ts)...) { _dtag.priority = GetPriority(); } \
			static ::rs::Priority GetPriority(); };
	#define ImplDrawGroup(name, prio) \
		::rs::Priority name::GetPriority() { return prio; }
	#define DefineDrawGroup(name)	DefineDrawGroupBase(name, ::rs::DrawGroup)
	#define DefineDrawGroupProxy(name)	DefineDrawGroupBase(name, ::rs::DrawGroupProxy)

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
			LCValue recvMsg(const GMessageStr& msg, const LCValue& arg) override;
			LCValue recvMsgLua(const GMessageStr& msg, const LCValue& arg) override;
			void proc(UpdProc p, bool bRecursive, Priority prioBegin, Priority prioEnd) override;

			const std::string& getName() const override;

			void onUpdate(bool bFirst) override;
			void setIdle(int nFrame);
			int getAccum() const;
			// コピー禁止
			UpdTask& operator = (const UpdTask&) = delete;
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

	struct IEffect;
	// ---- Draw sort algorithms ----
	struct DSort {
		//! ソートに必要な情報が記録されているか(デバッグ用)
		virtual bool hasInfo(const DrawTag& d) const = 0;
		virtual bool compare(const DrawTag& d0, const DrawTag& d1) const = 0;
		virtual void apply(const DrawTag& d, IEffect& e);
		static void DoSort(const DSortV& alg, int cursor, typename DLObjV::iterator itr0, typename DLObjV::iterator itr1);
	};
	//! 描画ソート: Z距離の昇順
	struct DSort_Z_Asc : DSort {
		//! 無効とされる深度値のボーダー (これ以下は無効)
		const static float cs_border;
		bool hasInfo(const DrawTag& d) const override;
		bool compare(const DrawTag& d0, const DrawTag& d1) const override;
	};
	//! 描画ソート: Z距離の降順
	struct DSort_Z_Desc : DSort_Z_Asc {
		bool compare(const DrawTag& d0, const DrawTag& d1) const override;
	};
	//! 描画ソート: ユーザー任意の優先度値 昇順
	struct DSort_Priority_Asc : DSort {
		bool hasInfo(const DrawTag& d) const override;
		bool compare(const DrawTag& d0, const DrawTag& d1) const override;
	};
	//! 描画ソート: ユーザー任意の優先度値 降順
	struct DSort_Priority_Desc : DSort_Priority_Asc {
		bool compare(const DrawTag& d0, const DrawTag& d1) const override;
	};
	//! 描画ソート: Tech&Pass Id
	struct DSort_TechPass : DSort {
		const static uint32_t cs_invalidValue;
		bool hasInfo(const DrawTag& d) const override;
		bool compare(const DrawTag& d0, const DrawTag& d1) const override;
		void apply(const DrawTag& d, IEffect& e) override;
	};
	namespace detail {
		class DSort_UniformPairBase {
			private:
				IEffect*	_pFx = nullptr;
			protected:
				//! UniformIdがまだ取得されて無ければ or 前回と違うEffectの時にIdを更新
				void _refreshUniformId(IEffect& e, const std::string* name, int* id, size_t length);
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
				const ArId& _getUniformId(IEffect& e) {
					_refreshUniformId(e, _strUniform.data(), _unifId.data(), N);
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
			bool hasInfo(const DrawTag& d) const override;
			bool compare(const DrawTag& d0, const DrawTag& d1) const override;
			void apply(const DrawTag& d, IEffect& e) override;
	};

	//! 描画ソート: Vertex&Index Buffer
	struct DSort_Buffer : DSort {
		constexpr static int length = std::tuple_size<DrawTag::VBuffAr>::value;

		bool hasInfo(const DrawTag& d) const override;
		bool compare(const DrawTag& d0, const DrawTag& d1) const override;
		void apply(const DrawTag& d, IEffect& e) override;
	};
	extern const DSortSP	cs_dsort_z_asc,
							cs_dsort_z_desc,
							cs_dsort_priority_asc,
							cs_dsort_priority_desc,
							cs_dsort_techpass,
							cs_dsort_texture,
							cs_dsort_buffer;

	enum class SortAlg {
		Z_Asc,
		Z_Desc,
		Priority_Asc,
		Priority_Desc,
		TechPass,
		Texture,
		Buffer,
		_Num
	};
	using SortAlgList = std::vector<SortAlg>;
	class DrawGroup : public DrawableObj {
		private:
			static const DSortSP cs_dsort[];
			DSortV		_dsort;			//!< ソートアルゴリズム優先度リスト
			bool		_bDynamic;		//!< 毎フレームソートするか
			DLObjV		_dobj;

			void _doDrawSort();
			static DSortV _MakeDSort(const SortAlgList& al);
		public:
			// 描画ソート方式を指定
			DrawGroup(const DSortV& al, bool bDynamic=false);
			DrawGroup(const SortAlgList& al, bool bDynamic=false);
			DrawGroup();

			void addObj(HDObj hObj);
			void remObj(HDObj hObj);
			void onUpdate(bool bFirst) override;
			void setSortAlgorithm(const DSortV& ds, bool bDynamic);
			void setSortAlgorithmId(const SortAlgList& al, bool bDynamic);
			void setPriority(Priority p);
			const DSortV& getSortAlgorithm() const;
			const DLObjV& getMember() const;

			bool isNode() const override;
			void onDraw(IEffect& e) const override;
			const std::string& getName() const override;
			DrawTag& refDTag();
	};
	class DrawGroupProxy : public DrawableObj {
		private:
			HLDGroup		_hlDGroup;
		public:
			DrawGroupProxy(HDGroup hDg);

			void onUpdate(bool bFirst) override;
			const DSortV& getSortAlgorithm() const;
			const DLObjV& getMember() const;

			void setPriority(Priority p);
			bool isNode() const override;
			void onDraw(IEffect& e) const override;
			const std::string& getName() const override;
			DrawTag& refDTag();
	};

	namespace detail {
		//! オブジェクト基底
		/*! UpdatableなオブジェクトやDrawableなオブジェクトはこれから派生して作る
			Sceneと共用 */
		template <class T, class Base>
		class ObjectT : public Base, public ::rs::ObjectIdT<T, ::rs::idtag::Object> {
			using ThisT = ObjectT<T,Base>;
			using IdT = ::rs::ObjectIdT<T, ::rs::idtag::Object>;
			protected:
				struct State {
					virtual ~State() {}
					virtual ObjTypeId getStateId() const = 0;
					virtual void onUpdate(T& self) { self.Base::onUpdate(false); }
					virtual LCValue recvMsg(T& self, const GMessageStr& msg, const LCValue& arg) { return self.Base::recvMsg(msg, arg); }
					// onEnterとonExitは継承しない
					virtual void onEnter(T& /*self*/, ObjTypeId /*prevId*/) {}
					virtual void onExit(T& /*self*/, ObjTypeId /*nextId*/) {}
					virtual void onConnected(T& self, HGroup hGroup) { self.Base::onConnected(hGroup); }
					virtual void onDisconnected(T& self, HGroup hGroup) { self.Base::onDisconnected(hGroup); }
					// --------- Scene用メソッド ---------
					virtual void onDraw(const T& self, IEffect& e) const { self.Base::onDraw(e); }
					virtual void onDown(T& self, ObjTypeId prevId, const LCValue& arg) { self.Base::onDown(prevId, arg); }
					virtual void onPause(T& self) { self.Base::onPause(); }
					virtual void onStop(T& self) { self.Base::onStop(); }
					virtual void onResume(T& self) { self.Base::onResume(); }
					virtual void onReStart(T& self) { self.Base::onReStart(); }
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
				FPState	_state = FPState(&_nullState, false),
						_nextState;
				//! 遅延メッセージリストの先端
				GMessage::Packet* _delayMsg;
				bool _isNullState() const {
					return _state.get() == &_nullState;
				}
				void _setNullState() {
					setStateUse(&_nullState);
				}
			protected:
				using Base::Base;

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
						bool bNull = _isNullState();
						_bSwState = true;
						_nextState.swap(st);
						if(bNull)
							_doSwitchState();
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
					return ret;
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
				void _doSwitchState() {
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
					return 0x0000;
				}
				ObjTypeId getStateId() const {
					return _state->getStateId();
				}
				void destroy() override {
					Base::destroy();
					// 終端ステートに移行
					_setNullState();
				}
				//! 毎フレームの描画 (Scene用)
				void onDraw(IEffect& e) const override {
					// ステート遷移はナシ
					_state->onDraw(getRef(), e);
				}
				//! 上の層のシーンから抜ける時に戻り値を渡す (Scene用)
				void onDown(ObjTypeId prevId, const LCValue& arg) override {
					_callWithSwitchState([&](){ _state->onDown(getRef(), prevId, arg); });
				}
				// ----------- 以下はStateのアダプタメソッド -----------
				void onUpdate(bool /*bFirst*/) override {
					_callWithSwitchState([&](){ return _state->onUpdate(getRef()); });
				}
				LCValue recvMsg(const GMessageStr& msg, const LCValue& arg) override {
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
		template <class T, class Base>
		typename ObjectT<T, Base>::template StateT<void> ObjectT<T, Base>::_nullState;
	}
	template <class T, class Base>
	template <class ST, class D>
	const ObjectIdT<ST,typename detail::ObjectT<T,Base>::tagObjectState> detail::ObjectT<T,Base>::StateT<ST,D>::s_idt;

	template <class T, class Base=Object>
	class ObjectT : public detail::ObjectT<T, Base> {
		using detail::ObjectT<T, Base>::ObjectT;
	};
	// PriorityはUpdateObjと兼用の場合に使われる
	template <class T>
	using DrawableObjT = ObjectT<T, DrawableObj>;

	namespace detail {
		struct ObjectT_LuaBase {
			static LCValue CallRecvMsg(const SPLua& ls, HObj hObj, const GMessageStr& msg, const LCValue& arg);
		};
	}
	template <class T, class Base=Object>
	class ObjectT_Lua : public ObjectT<T,Base> {
		private:
			using base = ObjectT<T,Base>;
			HObj _hMe;
			HObj _getHandle() {
				if(!_hMe)
					_hMe = base::handleFromThis();
				return _hMe;
			}
		protected:
			template <class... Ret, class... Ts>
			auto _callLuaMethod(const std::string& method, Ts&&... ts) {
				auto sp = rs_mgr_obj.getLua();
				sp->push(_getHandle());
				LValueS lv(sp->getLS());
				int top = sp->getTop();
				auto ret = lv.callMethod<Ret...>(luaNS::RecvMsg, method, std::forward<Ts>(ts)...);
				sp->setTop(top);
				return ret;
			}
		public:
			using base::base;
			void onUpdate(bool bFirst) override {
				base::onUpdate(false);
				if(bFirst) {
					if(!base::isDead())
						_callLuaMethod(luaNS::OnUpdate);
					if(base::isDead())
						_callLuaMethod(luaNS::OnExit, "null");
				}
			}
			LCValue recvMsgLua(const GMessageStr& msg, const LCValue& arg) override {
				return detail::ObjectT_LuaBase::CallRecvMsg(rs_mgr_obj.getLua(), _getHandle(), msg, arg);
			}
	};
	DefineUpdGroup(U_UpdGroup)
	DefineDrawGroup(U_DrawGroup)
	DefineDrawGroupProxy(U_DrawGroupProxy)
}
#include "luaimport.hpp"
DEF_LUAIMPORT(rs::Object)
DEF_LUAIMPORT(rs::UpdGroup)
DEF_LUAIMPORT(rs::DrawGroup)
DEF_LUAIMPORT(rs::DrawGroupProxy)
DEF_LUAIMPORT(rs::U_UpdGroup)
DEF_LUAIMPORT(rs::U_DrawGroup)
DEF_LUAIMPORT(rs::U_DrawGroupProxy)
