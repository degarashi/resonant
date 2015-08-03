#include "sdlwrap.hpp"
#include "spinner/random.hpp"
#include "spinner/charvec.hpp"
#include "apppath.hpp"

namespace rs {
	// --------------------- RWE_Error ---------------------
	RWops::RWE_Error::RWE_Error(const std::string& /*title*/): std::runtime_error("") {}
	void RWops::RWE_Error::setMessage(const std::string& msg) {
		std::stringstream ss;
		ss << "RWops - " << _title << std::endl << msg;
		reinterpret_cast<std::runtime_error&>(*this) = std::runtime_error(ss.str());
	}
	// --------------------- RWE_File ---------------------
	RWops::RWE_File::RWE_File(const std::string& path): RWE_Error("can't open file"), _path(path) {
		setMessage(path);
	}
	// --------------------- RWE_OutOfRange ---------------------
	RWops::RWE_OutOfRange::RWE_OutOfRange(int64_t pos, Hence hence, int64_t size): RWE_Error("file pointer out of range"), _hence(hence), _pos(pos), _size(size) {
		std::stringstream ss;
		ss << "position: ";
		switch(_hence) {
			case Hence::Begin:
				ss << "Begin"; break;
			case Hence::Current:
				ss << "Current"; break;
			default:
				ss << "End";
		}
		ss << std::endl;
		ss << "file length: " << size << std::endl;
		ss << "current: " << pos << std::endl;
		setMessage(ss.str());
	}
	// --------------------- RWE_Permission ---------------------
	RWops::RWE_Permission::RWE_Permission(Access have, Access tr): RWE_Error("invalid permission"), _have(have), _try(tr) {
		std::stringstream ss;
		ss << "permission: "<< std::endl;
		ss << "(having): " << std::hex << have << std::endl;
		ss << "(needed): " << std::hex << tr << std::endl;
		setMessage(ss.str());
	}
	// --------------------- RWE_Memory ---------------------
	RWops::RWE_Memory::RWE_Memory(size_t size): RWE_Error("can't allocate memory"), _size(size) {
		std::stringstream ss;
		ss << "size: " << size;
		setMessage(ss.str());
	}
	// --------------------- RWE_NullMemory ---------------------
	RWops::RWE_NullMemory::RWE_NullMemory(): RWE_Error("null memory pointer detected") {
		setMessage("");
	}

	RWops RWops::FromConstMem(const void* mem, size_t size, Callback* cb) {
		AssertT(Trap, mem, (RWE_NullMemory))
		SDL_RWops* ops = SDL_RWFromConstMem(mem,size);
		SDLEC_Chk(Throw)
		return RWops(ops,
					Type::ConstMem,
					Read,
					ExtBuff{const_cast<void*>(mem),size},
					cb);
	}
	RWops RWops::FromMem(void* mem, size_t size, Callback* cb) {
		AssertT(Trap, mem, (RWE_NullMemory))
		SDL_RWops* ops = SDL_RWFromMem(mem,size);
		SDLEC_Chk(Throw)
		return RWops(ops,
					Type::Mem,
					Read|Write,
					ExtBuff{mem,size},
					cb);
	}
	RWops RWops::FromFile(const std::string& path, int access) {
		auto* str = path.c_str();
		str = spn::PathBlock::RemoveDriveLetter(str, str + path.length());
		std::string mode = ReadModeStr(access);
		SDL_RWops* ops = SDLEC(Throw, SDL_RWFromFile, str, mode.c_str());
		return RWops(ops,
					Type::File,
					access,
					path,
					nullptr);
	}
	RWops RWops::FromURI(SDL_RWops* ops, const spn::URI& uri, int access) {
		AssertT(Trap, ops, (RWE_File)(const std::string&), uri.plain_utf8())
		return RWops(ops,
					Type::File,
					access,
					uri,
					nullptr);
	}
	RWops RWops::_FromVector(spn::ByteBuff&& buff, Callback* cb, std::false_type) {
		SDL_RWops* ops = SDL_RWFromMem(&buff[0], buff.size());
		SDLEC_Chk(Throw)
		return RWops(ops,
					Type::Vector,
					Read|Write,
					std::move(buff),
					cb);
	}
	RWops RWops::_FromVector(spn::ByteBuff&& buff, Callback* cb, std::true_type) {
		SDL_RWops* ops = SDL_RWFromConstMem(&buff[0], buff.size());
		SDLEC_Chk(Throw)
		return RWops(ops,
					Type::ConstVector,
					Read,
					std::move(buff),
					cb);
	}
	void RWops::_deserializeFromType(int64_t pos) {
		if(_type != Type::File) {
			auto& buff = boost::get<spn::ByteBuff>(_data);
			if(_type == Type::ConstVector)
				_ops = SDL_RWFromConstMem(&buff[0], buff.size());
			else {
				AssertP(Trap, _type==Type::Vector)
				_ops = SDL_RWFromMem(&buff[0], buff.size());
			}
		} else {
			if(_data.which() == 1)
				_ops = SDL_RWFromFile(boost::get<std::string>(_data).c_str(), ReadModeStr(_access).c_str());
			else {
				AssertP(Trap, _data.which() == 2)
				auto& uri = boost::get<spn::URI>(_data);
				auto& h = mgr_rw.getHandler();
				h.procHandler(uri, _access);
				HLRW hlRW = h.procHandler(uri, _access);
				*this = std::move(hlRW.ref());
			}
		}
		seek(pos, Hence::Begin);
	}
	int RWops::ReadMode(const char* mode) {
		int ret = 0;
		const std::pair<char,int> c_flag[] = {{'r', Read}, {'w', Write}, {'b', Binary}};
		const char* c = mode;
		while(*c != '\0') {
			for(auto& p : c_flag) {
				if(p.first == *c)
					ret |= p.second;
			}
			++c;
		}
		return ret;
	}
	std::string RWops::ReadModeStr(int mode) {
		std::string ret;
		ret.resize(3);
		auto* pDst = &ret[0];
		if(mode & Read)
			*pDst++ = 'r';
		if(mode & Write)
			*pDst++ = 'w';
		if(mode & Binary)
			*pDst++ = 'b';
		ret.resize(pDst - &ret[0]);
		return ret;
	}
	RWops::~RWops() {
		close();
	}
	void RWops::close() {
		if(_ops) {
			if(_endCB)
				_endCB->onRelease(*this);
			SDL_RWclose(_ops);
			_clear();
		}
	}
	void RWops::_clear() {
		_ops = nullptr;
		_access = 0;
		_endCB.reset(nullptr);
		_data = boost::blank();
	}
	RWops::RWops(RWops&& ops): _ops(ops._ops), _access(ops._access),
		_type(ops._type), _data(std::move(ops._data)), _endCB(std::move(ops._endCB))
	{
		ops._clear();
	}
	RWops& RWops::operator = (RWops&& ops) {
		this->~RWops();
		return *(new(this) RWops(std::move(ops)));
	}

	int RWops::getAccessFlag() const {
		return _access;
	}
	size_t RWops::read(void* dst, size_t blockSize, size_t nblock) {
		return SDL_RWread(_ops, dst, blockSize, nblock);
	}
	size_t RWops::write(const void* src, size_t blockSize, size_t nblock) {
		return SDL_RWwrite(_ops, src, blockSize, nblock);
	}
	namespace {
		struct Visitor : boost::static_visitor<int64_t> {
			RWops& _ops;
			Visitor(RWops& op): _ops(op) {}

			template <class T>
			int64_t operator()(RWops::ExtBuff& e) const {
				return e.size;
			}
			template <class T>
			int64_t operator()(T& /*t*/) const {
				auto pos = _ops.tell();
				_ops.seek(0, RWops::Hence::End);
				int64_t ret = _ops.tell();
				_ops.seek(pos, RWops::Hence::Begin);
				return ret;
			}
		};
	}
	int64_t RWops::size() {
		return boost::apply_visitor(Visitor(*this), _data);
	}
	int64_t RWops::seek(int64_t offset, Hence hence) {
		auto res = SDL_RWseek(_ops, offset, hence);
		AssertT(Trap, res>=0, (RWE_OutOfRange)(int64_t)(Hence)(int64_t), offset, hence, tell())
		return res;
	}
	int64_t RWops::tell() const {
		return SDL_RWtell(_ops);
	}
	uint16_t RWops::readBE16() {
		return SDL_ReadBE16(_ops);
	}
	uint32_t RWops::readBE32() {
		return SDL_ReadBE32(_ops);
	}
	uint64_t RWops::readBE64() {
		return SDL_ReadBE64(_ops);
	}
	uint16_t RWops::readLE16() {
		return SDL_ReadLE16(_ops);
	}
	uint32_t RWops::readLE32() {
		return SDL_ReadLE32(_ops);
	}
	uint64_t RWops::readLE64() {
		return SDL_ReadLE64(_ops);
	}
	bool RWops::writeBE(uint16_t value) {
		return SDL_WriteBE16(_ops, value) == 1;
	}
	bool RWops::writeBE(uint32_t value) {
		return SDL_WriteBE32(_ops, value) == 1;
	}
	bool RWops::writeBE(uint64_t value) {
		return SDL_WriteBE64(_ops, value) == 1;
	}
	bool RWops::writeLE(uint16_t value) {
		return SDL_WriteLE16(_ops, value) == 1;
	}
	bool RWops::writeLE(uint32_t value) {
		return SDL_WriteLE32(_ops, value) == 1;
	}
	bool RWops::writeLE(uint64_t value) {
		return SDL_WriteLE64(_ops, value) == 1;
	}
	SDL_RWops* RWops::getOps() {
		return _ops;
	}
	spn::ByteBuff RWops::readAll() {
		auto pos = tell();
		size_t sz = size();
		spn::ByteBuff buff(sz);
		seek(0, Begin);
		read(&buff[0], sz, 1);
		seek(pos, Begin);
		return buff;
	}
	RWops::Type RWops::getType() const { return _type; }
	bool RWops::isMemory() const { return _type != File; }

	std::pair<const void*, size_t> RWops::getMemoryPtrC() const {
		if(_type == Type::ConstMem || _type == Type::Mem) {
			auto& d = boost::get<ExtBuff>(_data);
			return std::make_pair(d.ptr, d.size);
		}
		if(_type == Type::Vector || _type == Type::ConstVector) {
			auto& d = boost::get<spn::ByteBuff>(_data);
			return std::make_pair(&d[0], d.size());
		}
		AssertP(Trap, false);
		return std::make_pair(nullptr, 0);
	}
	std::pair<void*, size_t> RWops::getMemoryPtr() {
		if(_type == Type::Mem) {
			auto& d = boost::get<ExtBuff>(_data);
			return std::make_pair(d.ptr, d.size);
		}
		if(_type == Type::Vector) {
			auto& d = boost::get<spn::ByteBuff>(_data);
			return std::make_pair(&d[0], d.size());
		}
		AssertP(Trap, false);
		return std::make_pair(nullptr, 0);
	}
	// ---------------------------- RWMgr ----------------------------
	namespace {
		constexpr int RANDOMLEN_MIN = 8,
						RANDOMLEN_MAX = 16,
						RANDOMIZER_ID = 0x10000000,
						MAX_RETRY_COUNT = 64;
		const spn::CharVec<char> c_charVec{
			{'A', 'Z'},
			{'a', 'z'},
			{'0', '9'},
			{'_'}
		};
		const std::string c_tmpDirName("tmp");
	}
	RWMgr::RWMgr(const std::string& org_name, const std::string& app_name):
		_orgName(org_name),
		_appName(app_name)
	{
		static bool s_bInitRandom = false;
		if(!s_bInitRandom) {
			// 初回だけランダム生成器を初期化
			s_bInitRandom = true;
			mgr_random.initEngine(RANDOMIZER_ID);
			// 一時ファイルを掃除
			_cleanupTemporaryFile();
		}
	}
	RWMgr::UriHandlerV& RWMgr::getHandler() {
		return _handlerV;
	}
	HLRW RWMgr::HChk::operator()(UriHandler& h, const spn::URI& uri, int access) const {
		return h.openURI_RW(uri, access);
	}
	HLRW RWMgr::fromURI(const spn::URI& uri, int access) {
		HLRW ret = getHandler().procHandler(uri, access);
		AssertT(Throw, ret.valid(), (RWops::RWE_File)(const std::string&), uri.plain_utf8())
		return ret;
	}
	HLRW RWMgr::fromFile(const std::string& path, int access) {
		return base_type::acquire(RWops::FromFile(path, access));
	}
	HLRW RWMgr::fromConstMem(const void* p, int size, typename RWops::Callback* cb) {
		return base_type::acquire(RWops::FromConstMem(p,size,cb));
	}
	HLRW RWMgr::fromMem(void* p, int size, typename RWops::Callback* cb) {
		return base_type::acquire(RWops::FromMem(p,size, cb));
	}
	struct SDLDeleter {
		void operator ()(void* ptr) const {
			SDL_free(ptr);
		}
	};
	using SDLPtr = std::unique_ptr<void, SDLDeleter>;
	std::string RWMgr::makeFilePath(const std::string& dirName) const {
		#ifdef ANDROID
			std::string path(SDL_AndroidGetInternalStoragePath());
		#else
			SDLPtr str(SDL_GetPrefPath(_orgName.c_str(), _appName.c_str()));
			std::string path(reinterpret_cast<const char*>(str.get()));
		#endif
		if(!dirName.empty())
			path.append(dirName);
		return path;
	}
	std::pair<HLRW,std::string> RWMgr::createTemporaryFile() {
		// Temporaryディレクトリ構造を作る
		spn::Dir tmpdir(makeFilePath(c_tmpDirName));
		using spn::FStatus;
		// 既に別のファイルがあるなどしてディレクトリが作れなければ内部で例外を投げる
		tmpdir.mkdir(FStatus::GroupRead | FStatus::OtherRead | FStatus::UserRWX);
		// ランダムなファイル名[A-Za-z0-9_]{8,16}を重複考えず作る
		std::string str;
		auto rnd = mgr_random.get(RANDOMIZER_ID);
		int retry_count = MAX_RETRY_COUNT;
		while(--retry_count >= 0) {
			int length = rnd.getUniform<int>({RANDOMLEN_MIN, RANDOMLEN_MAX});
			int csize = c_charVec.size();
			for(int i=0 ; i<length ; i++)
				str.append(1, c_charVec.get(rnd.getUniform<int>({0, csize-1})));
			// 同名のファイルが既に存在しないかチェック
			spn::Dir dir(tmpdir);
			dir.pushBack(str);
			if(!dir.isFile()) {
				std::string fpath = dir.plain_utf8();
				return std::make_pair(fromFile(fpath, RWops::Write), fpath);
			}
		}
		// 指定回数リトライしても駄目な場合はエラーとする
		Assert(Trap, false, "can't create temporary file")
		return std::make_pair(HLRW(), std::string());
	}
	void RWMgr::_cleanupTemporaryFile() {
		std::string path = makeFilePath(c_tmpDirName);
		path += "/*";
		auto strlist = spn::Dir::EnumEntryWildCard(path);
		for(auto& s : strlist) {
			spn::Dir dir(s);
			dir.remove();
		}
	}

	// file:// filetreeのみ
	// pack:// zipのみ
	// res:// zip, filetreeの順で探す
	// ---------------------------- UriH_PackedZip ----------------------------
	bool UriH_PackedZip::Capable(const spn::URI& uri) {
		auto typ = uri.getType_utf8();
		return (typ == "pack" || typ == "file");
	}
	UriH_PackedZip::UriH_PackedZip(spn::ToPathStr zippath):
		_stream(new spn::AdaptStd(
			std::unique_ptr<std::istream>(
				new std::ifstream(zippath.getStringPtr())
			)
		)),
		_ztree(*_stream)
	{}
	spn::UP_Adapt UriH_PackedZip::openURI(const spn::URI& uri) {
		if(Capable(uri)) {
			if(auto* pFile = _ztree.findFile(uri.plain_utf32())) {
				std::unique_ptr<std::iostream> up(new std::stringstream);
				std::unique_ptr<spn::AdaptIOStd> ret(new spn::AdaptIOStd(std::move(up)));
				_ztree.extract(*ret, pFile->index, *_stream);
				return std::move(ret);
			}
		}
		return nullptr;
	}
	HLRW UriH_PackedZip::openURI_RW(const spn::URI& /*uri*/, int /*access*/) {
		return HLRW();
	}

	// ---------------------------- UriH_AssetZip ----------------------------
	// ---------------------------- UriH_File ----------------------------
	UriH_File::UriH_File(spn::ToPathStr path): _basePath(path.moveTo()) {}
	bool UriH_File::Capable(const spn::URI& uri, int /*access*/) {
		auto typ = uri.getType_utf8();
		return (typ == "file" || typ == "res");
	}
	spn::UP_Adapt UriH_File::openURI(const spn::URI& uri) {
		if(Capable(uri, RWops::Read)) {
			try {
				std::unique_ptr<std::ifstream> up(new std::ifstream(uri.plain_utf8()));
				Assert(Throw, up->is_open())
				return spn::UP_Adapt(new spn::AdaptStd(std::move(up)));
			} catch(const std::exception& e) {
				// ファイルが読み込めないのはエラーとして扱わない
			}
		}
		return nullptr;
	}
	HLRW UriH_File::openURI_RW(const spn::URI& uri, int access) {
		// (SDLのファイルロード関数がやってくれる筈だが、明示的にやる)
		if(Capable(uri, access)) {
			try {
				// 相対パス時はプログラムが置いてある地点から探索
				auto& path = uri.path();
				if(!path.isAbsolute()) {
					spn::PathBlock pb(mgr_path.getAppDir());
					pb <<= path;
					return mgr_rw.fromFile(pb.plain_utf8(), access);
				}
				return mgr_rw.fromFile(uri.plain_utf8(), access);
			} catch(const std::runtime_error& e) {
				// ファイルが読み込めないのはエラーとして扱わない
			}
		}
		return HLRW();
	}
}
