#pragma once
#include "spinner/resmgr.hpp"
#include "spinner/dir.hpp"
#include "handle.hpp"

namespace rs {
	#define mgr_path (::rs::AppPath::_ref())
	//! アプリケーションのパスや引数、その他システムパスを登録
	/*! 将来的にはLuaによる変数定義で置き換え */
	class AppPath : public spn::Singleton<AppPath> {
		private:
			//! アプリケーション本体のパス
			spn::PathBlock	_pbApp,
							_pbAppDir;
			using PathV = std::vector<spn::PathBlock>;
			// AppPathを先頭に付加した状態で格納
			using PathM = std::unordered_map<std::string, PathV>;
			PathM			_path;

		public:
			AppPath(const std::string& apppath);
			//! シンプルなテキスト記述でシステムパスを設定
			/*! [ResourceType]\n
				path0\n
				path1\n
				...
			*/
			void setFromText(HRW hRW, bool bAdd);
			//! Luaスクリプト形式でパスを設定 (予定)
			// void setFromLua();

			const spn::PathBlock& getAppPath() const;
			const spn::PathBlock& getAppDir() const;
			HLRW getRW(const std::string& resname, const std::string& filename, std::string* opt) const;
			using CBEnum = std::function<bool (const spn::Dir&)>;
			void enumPath(const std::string& resname, const std::string& pattern, CBEnum cb) const;
	};
	class AppPathCache {
		private:
			// リソース名 -> URIのキャッシュ
			using Cache = std::unordered_map<std::string, spn::URI>;
			using CacheV = std::vector<std::pair<std::string, Cache>>;
			mutable CacheV	_cache;

			void _init(const std::string rtname[], size_t n);
		public:
			template <size_t N>
			AppPathCache(const std::string (&rtname)[N]): _cache(N) {
				_init(rtname, N);
			}
			const spn::URI& uriFromResourceName(int n, const std::string& name) const;
	};
	template <class Dat, class Derived>
	class ResMgrApp : public spn::ResMgrN<Dat, Derived, std::allocator, spn::URI> {
		public:
			using base_t = spn::ResMgrN<Dat, Derived, std::allocator, spn::URI>;
			using LHdl = typename base_t::LHdl;
		private:
			AppPathCache	_cache;
			int				_idResType = 0;
		protected:
			template <class T>
			ResMgrApp(T&& t): _cache(std::forward<T>(t)) {}

			void _setResourceTypeId(int id) {
				_idResType = id;
			}
			spn::URI _modifyResourceName(spn::URI& key) const override {
				// Protocolを持っていないリソース名を有効なURIに置き換え (ResMgrへの登録名はURIで行う)
				if(key.getType_utf8().empty()) {
					try {
						key = _uriFromResourceName(key.path().plain_utf8());
					} catch(const std::invalid_argument& e) {
						LogOutput("resource name not matched %1%", key.plain_utf8());
					}
				}
				return key;
			}
			const spn::URI& _uriFromResourceName(const std::string& name) const {
				return _cache.uriFromResourceName(_idResType, name);
			}
			const spn::URI& _uriFromResourceName(const spn::URI& name) const {
				return name;
			}
		public:
			// CB = function<Dat (const spn::URI&)>
			// INIT = function<void (Handle)>
			template <class KEY, class CB, class INIT>
			LHdl loadResourceApp(KEY&& name, CB&& cb, INIT&& cbInit) {
				auto ret = base_t::acquire(std::forward<KEY>(name), std::forward<CB>(cb));
				if(ret.second)
					std::forward<INIT>(cbInit)(ret.first.get());
				return std::move(ret.first);
			}
	};
}
