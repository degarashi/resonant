#include "sdlwrap.hpp"

namespace rs {
	// --------------------- RWE_Error ---------------------
	RWops::RWE_Error::RWE_Error(const std::string& title): std::runtime_error("") {}
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
	RWops::RWE_OutOfRange::RWE_OutOfRange(int64_t pos, int64_t size): RWE_Error("file pointer out of range"), _pos(pos), _size(size) {
		std::stringstream ss;
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
		SDL_RWops* ops = SDL_RWFromFile(str, mode.c_str());
		SDLEC_Chk(Throw)
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
				auto handler = mgr_rw.getUriHandler(uri, _access);
				HLRW hlRW = (*handler)->loadURI(uri, _access, true);
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
		return std::move(ret);
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
			int64_t operator()(T& t) const {
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
		return std::move(buff);
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
	HLRW RWMgr::fromURI(const spn::URI& uri, int access, bool bNoShared) {
		if(auto handler = getUriHandler(uri, access))
			return (*handler)->loadURI(uri, access, bNoShared);
		AssertT(Throw, false, (RWops::RWE_File)(const std::string&), uri.plain_utf8())
	}
	HLRW RWMgr::fromFile(const std::string& path, int access, bool bNoShared) {
		if(bNoShared)
			return base_type::acquire(RWops::FromFile(path, access));
		auto* str = &path[0];
		str = spn::PathBlock::RemoveDriveLetter(str, str+path.length());
		return base_type::acquire(str, RWops::FromFile(str, access)).first;
	}
	HLRW RWMgr::fromConstMem(const void* p, int size, typename RWops::Callback* cb) {
		return base_type::acquire(RWops::FromConstMem(p,size,cb));
	}
	HLRW RWMgr::fromMem(void* p, int size, typename RWops::Callback* cb) {
		return base_type::acquire(RWops::FromMem(p,size, cb));
	}
	void RWMgr::addUriHandler(const SPUriHandler& h) {
		Assert(Trap, std::find(_handler.begin(), _handler.end(), h)==_handler.end())
		_handler.push_back(h);
	}
	void RWMgr::remUriHandler(const SPUriHandler& h) {
		auto itr = std::find(_handler.begin(), _handler.end(), h);
		Assert(Trap, itr!=_handler.end())
		_handler.erase(itr);
	}
	RWMgr::OPUriHandler RWMgr::getUriHandler(const spn::URI& uri, int access) const {
		for(auto& h : _handler) {
			if(h->canLoad(uri, access))
				return h;
		}
		Assert(Warn, false, "can't handle URI \"%s\"", uri.plainUri_utf8())
		return spn::none;
	}
	// ---------------------------- UriH_File ----------------------------
	UriH_File::UriH_File(spn::ToPathStr path): _basePath(path.moveTo()) {}
	bool UriH_File::canLoad(const spn::URI& uri, int access) const {
		return uri.getType_utf8() == "file";
	}
	HLRW UriH_File::loadURI(const spn::URI& uri, int access, bool bNoShared) {
		if(canLoad(uri, access))
			return mgr_rw.fromFile(uri.plain_utf8(), access, bNoShared);
		return HLRW();
	}
}
