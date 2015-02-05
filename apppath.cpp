#include "apppath.hpp"
#include "sdlwrap.hpp"

namespace rs {
	// ------------------- AppPath -------------------
	AppPath::AppPath(const std::string& apppath): _pbApp(apppath), _pbAppDir(_pbApp) {
		Assert(Trap, _pbApp.isAbsolute())
	}
	void AppPath::setFromText(HRW hRW, bool bAdd) {
		if(!bAdd)
			_path.clear();

		RWops& ops = hRW.ref();
		spn::ByteBuff buff = ops.readAll();
		const char *ptr = reinterpret_cast<const char*>(&buff[0]),
					*ptrE = ptr + buff.size();
		char tmp[256];
		PathV* pEnt = nullptr;
		auto fnRead = [&tmp](const char* cur, const char* to, char cEnd) {
			int wcur = 0;
			while(cur != to) {
				char c = *cur++;
				if(c == cEnd) {
					tmp[wcur] = '\0';
					return cur;
				} else
					tmp[wcur++] = c;
			}
			throw std::invalid_argument("syntax error");
		};
		while(ptr < ptrE) {
			if(*ptr == '[') {
				// ---- Entry ----
				++ptr;
				// Read until ']'
				ptr = fnRead(ptr, ptrE, ']');
				pEnt = &_path[tmp];
				ptr = fnRead(ptr, ptrE, '\n');
			} else {
				// ---- Path ----
				// Read until '\n'
				ptr = fnRead(ptr, ptrE, '\n');
				spn::PathBlock pbtmp(_pbAppDir);
				pbtmp <<= tmp;
				pEnt->emplace_back(std::move(pbtmp));
			}
		}
	}
	HLRW AppPath::getRW(const std::string& resname, const std::string& filename, std::string* opt) const {
		HLRW hlRet;
		enumPath(resname, filename, [&hlRet, &opt](const spn::Dir& d){
			auto path = d.plain_utf8();
			if(auto lh = mgr_rw.fromFile(path, RWops::Read)) {
				if(opt)
					*opt = std::move(path);
				hlRet = std::move(lh);
				return false;
			}
			return true;
		});
		return hlRet;

	}
	void AppPath::enumPath(const std::string& resname, const std::string& pattern, CBEnum cb) const {
		auto fnCheck = [&cb](const std::string& pat){
			bool bContinue = true;
			spn::Dir::EnumEntryWildCard(pat, [&bContinue, &cb](const spn::Dir& d){
				if(bContinue)
					bContinue = cb(d);
			});
		};
		// 絶対パスならそれで検索
		spn::PathBlock pb(pattern);
		if(pb.isAbsolute())
			fnCheck(pattern);

		// リソース名に関連付けされたベースパスを付加して検索
		auto itr = _path.find(resname);
		if(itr != _path.end()) {
			auto& pathv = itr->second;
			for(auto& path : pathv) {
				pb = path;
				pb <<= pattern;
				fnCheck(pb.plain_utf8());
			}
		}
	}
	const spn::PathBlock& AppPath::getAppPath() const {
		return _pbApp;
	}
	const spn::PathBlock& AppPath::getAppDir() const {
		return _pbAppDir;
	}

	// ------------------- AppPathCache -------------------
	void AppPathCache::_init(const std::string rtname[], size_t n) {
		_cache.resize(n);
		for(size_t i=0 ; i<n ; i++)
			_cache[i].first = rtname[i];
	}
	const spn::URI& AppPathCache::uriFromResourceName(int n, const std::string& name) const {
		auto& cache = _cache[n];
		auto itr = cache.second.find(name);
		if(itr != cache.second.end())
			return itr->second;

		spn::URI uri;
		uri.setType("file");
		mgr_path.enumPath(cache.first, name, [&uri](const spn::Dir& d){
			uri.setPath(d.plain_utf8());
			return false;
		});
		Assert(Trap, !uri.path().empty(), "resource %1% not found", name)
		auto& ent = cache.second[name];
		ent = std::move(uri);
		return ent;
	}
}
