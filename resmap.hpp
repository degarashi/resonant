#pragma once
#include "spinner/resmgr.hpp"
#include <unordered_map>
#include "spinner/emplace.hpp"

namespace rs {
	//! リソースハンドルにKEYをつけて共有する (弱参照)
	template <class KEY, class WHDL>
	class WHandleMap {
		using RMap = std::unordered_map<KEY,WHDL>;
		RMap	_resource;
		using LHdl = spn::HdlLock<typename WHDL::SHdl, true>;

		public:
			void setObj(const KEY& key, WHDL wh) {
				spn::EmplaceOrReplace(_resource, key, wh);
			}
			LHdl getObj(const KEY& key) {
				// 無効ならエントリを消す
				auto itr = _resource.find(key);
				if(itr != _resource.end()) {
					LHdl hdl(itr->second.lock());
					if(!hdl.get().valid())
						_resource.erase(itr);
					return std::move(hdl);
				}
				return LHdl();
			}
			void cleanUp() {
				for(auto itr=_resource.begin() ; itr!=_resource.end() ;) {
					if(!itr->second.isHandleValid())
						itr = _resource.erase(itr);
					else
						++itr;
				}
			}
	};
	//! リソースハンドルにKEYをつけて共有する (強参照)
	template <class KEY, class SHDL>
	class SHandleMap {
		using RMap = std::unordered_map<KEY, spn::HdlLock<SHDL,true>>;
		RMap	_resource;

		public:
			void setObj(const KEY& key, SHDL sh) {
				spn::EmplaceOrReplace(_resource, key, sh);
			}
			void remObj(const KEY& key) {
				auto itr = _resource.find(key);
				if(itr != _resource.end())
					_resource.erase(itr);
			}
			SHDL getObj(const KEY& key) {
				auto itr = _resource.find(key);
				if(itr != _resource.end())
					return itr->second;
				return SHDL();
			}
	};
	//! 汎用ID登録マネージャ
	/*! staticな名前を登録する為なので、IDの削除はできない
		KEY,IDの組み合わせが同じだと共用されるが問題ない筈 */
	template <class KEY, class ID>
	class IDMap {
		using IDHash = std::unordered_map<KEY, ID>;

		static ID _refid(const KEY& key, bool bGet) {
			static IDHash* ms_idmap = new IDHash();
			static ID ms_idcur = 0;			// IDの先頭
			if(bGet) {
				auto itr = ms_idmap->find(key);
				AssertP(Trap, itr != ms_idmap->end())
				return itr->second;
			} else {
				AssertP(Trap, ms_idmap->find(key)==ms_idmap->end())
				ID id = ms_idcur++;
				ms_idmap->insert(std::make_pair(key, id));
				return id;
			}
		}
		public:
			constexpr static ID InvalidID = std::numeric_limits<ID>::max();
			static ID RegID(const KEY& key) {
				return _refid(key, false);
			}
			static ID GetID(const KEY& key) {
				return _refid(key, true);
			}
	};
	//! delete演算子を呼ぶデリータ
	template <class T>
	struct Del_Delete {
		void operator()(T* p) const { delete p; }
	};
	//! 何もしないデリータ
	template <class T>
	struct Del_Nothing {
		void operator()(T* /*p*/) const {}
	};
	//! リソースハンドルを解放
	template <class HDL>
	struct Del_Release {
		void operator()(HDL& hdl) const { hdl.release(); }
	};

	//! メモリの解放 / not解放をフラグで管理する簡易スマートポインタ
	template <class T, class DEL=Del_Delete<T>>
	class FlagPtr {
		bool	_bDelete;
		T*		_ptr;
		public:
			FlagPtr(): _bDelete(false) {}
			FlagPtr(FlagPtr&& fp): FlagPtr() {
				swap(fp);
			}
			FlagPtr(T* p, bool bDel): _bDelete(bDel), _ptr(p) {}
			~FlagPtr() {
				if(_bDelete) {
					DEL del;
					del(_ptr);
				}
			}
			void swap(FlagPtr& fp) noexcept {
				std::swap(_bDelete, fp._bDelete);
				std::swap(_ptr, fp._ptr);
			}
			void reset(T* p, bool bDel) {
				this->~FlagPtr();
				new(this) FlagPtr(p, bDel);
			}
			void reset() {
				reset(nullptr, false);
			}
			//! コピー禁止
			void operator = (const FlagPtr& p) const = delete;
			T* operator -> () { return _ptr; }
			const T* operator -> () const { return _ptr; }
			T* get() { return _ptr; }
			const T* get() const { return _ptr; }
			bool operator == (const FlagPtr& fp) const {
				return _ptr == fp._ptr;
			}
			bool operator == (const T* p) const {
				return _ptr == p;
			}
	};
}
