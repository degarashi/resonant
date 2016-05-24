#include "apppath.hpp"
#include "sdlwrap.hpp"

namespace rs {
	// ------------------- E_SyntaxError -------------------
	AppPath::E_SyntaxError::E_SyntaxError(int nline, char expect): std::invalid_argument("") {
		boost::format fmt("in line %1%:\n AppPath source(text) syntax error: expected '%2%'");
		static_cast<std::invalid_argument&>(*this) = std::invalid_argument((fmt % nline % expect).str());
	}
	AppPath::E_SyntaxError::E_SyntaxError(int nline, const std::string& msg): std::invalid_argument("") {
		boost::format fmt("in line %1%:\n AppPath source(text) syntax error: %2%");
		static_cast<std::invalid_argument&>(*this) = std::invalid_argument((fmt % nline % msg).str());
	}
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
		int line = 1;
		auto fnRead = [&tmp](int line, const char* cur, const char* to, char cEnd) {
			int wcur = 0;
			while(cur != to) {
				char c = *cur++;
				if(c == cEnd) {
					tmp[wcur] = '\0';
					return cur;
				} else
					tmp[wcur++] = c;
			}
			throw E_SyntaxError(line, cEnd);
		};
		while(ptr < ptrE) {
			if(*ptr == '[') {
				// ---- Entry ----
				++ptr;
				// Read until ']'
				ptr = fnRead(line, ptr, ptrE, ']');
				pEnt = &_path[tmp];
				ptr = fnRead(line++, ptr, ptrE, '\n');
			} else {
				// ---- Path ----
				// Read until '\n'
				ptr = fnRead(line++, ptr, ptrE, '\n');
				spn::PathBlock pbtmp(_pbAppDir);
				pbtmp <<= tmp;
				if(!pEnt)
					throw E_SyntaxError(line, "no active entry.");
				pEnt->emplace_back(std::move(pbtmp));
			}
		}
	}
	HLRW AppPath::getRW(const std::string& resname, const std::string& filename, const int access, std::string* opt) const {
		HLRW hlRet;
		enumPath(resname, filename, [&hlRet, &opt, access](const spn::Dir& d){
			auto path = d.plain_utf8();
			if(auto lh = mgr_rw.fromFile(path, access)) {
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
