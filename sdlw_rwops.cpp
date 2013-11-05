#include "sdlwrap.hpp"

namespace rs {
	RWops RWops::FromConstMem(const void* mem, int size, EndCB cb) {
		return RWops(SDL_RWFromConstMem(mem,size), Read, Type::ConstMem, cb, mem, size);
	}
	RWops RWops::FromMem(void* mem, int size, EndCB cb) {
		return RWops(SDL_RWFromMem(mem,size), Read|Write, Type::Mem, cb, mem, size);
	}
	RWops RWops::FromFilePointer(FILE* fp, bool autoClose, const char* mode) {
		return RWops(SDL_RWFromFP(fp, autoClose ? SDL_TRUE : SDL_FALSE),
					 _ReadMode(mode), Type::FilePointer, nullptr);
	}
	RWops RWops::FromFile(spn::ToPathStr path, const char* mode) {
		return RWops(SDL_RWFromFile(path.getStringPtr(), mode),
					_ReadMode(mode), Type::File, nullptr);
	}
	int RWops::_ReadMode(const char* mode) {
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

	RWops::RWops(SDL_RWops* ops, int access, Type type, EndCB cb, const void* ptr, size_t sz):
		_ops(ops), _access(access), _type(type), _endCB(cb), _ptr(ptr), _size(sz)
	{
		if(!ops)
			throw std::runtime_error("invalid file");
	}
	RWops::~RWops() {
		close();
	}
	void RWops::close() {
		if(_ops) {
			if(_endCB)
				_endCB(*this);
			SDL_RWclose(_ops);
			_clear();
		}
	}
	void RWops::_clear() {
		_ops = nullptr;
		_access = 0;
		_endCB = nullptr;
	}
	RWops::RWops(RWops&& ops): _ops(ops._ops), _access(ops._access),
		_type(ops._type), _ptr(ops._ptr), _size(ops._size), _endCB(std::move(ops._endCB))
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
	int64_t RWops::size() {
		if(isMemory())
			return _size;
		auto pos = tell();
		seek(0, Hence::End);
		int64_t ret = tell();
		seek(pos, Hence::Begin);
		return ret;
	}
	int64_t RWops::seek(int64_t offset, Hence hence) {
		return SDL_RWseek(_ops, offset, hence);
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
	bool RWops::isMemory() const { return _ptr != nullptr; }
	std::pair<const void*, size_t> RWops::getMemoryPtrC() const { return std::make_pair(_ptr, _size); }
	std::pair<void*, size_t> RWops::getMemoryPtr() {
		AssertP(Trap, _type==Type::Vector);
		return std::make_pair(const_cast<void*>(_ptr), _size);
	}

	// ---------------------------- RWMgr ----------------------------
	HLRW RWMgr::fromFile(spn::ToPathStr path, const char* mode, bool bNotKey) {
		auto rw = RWops::FromFile(path, mode);
		if(bNotKey)
			return base_type::acquire(std::move(rw));
		return base_type::acquire(spn::PathStr(path.getStringPtr()), RWops::FromFile(path, mode)).first;
	}
	HLRW RWMgr::fromConstMem(const void* p, int size, typename RWops::EndCB cb) {
		return base_type::acquire(RWops::FromConstMem(p,size,cb));
	}
	HLRW RWMgr::fromMem(void* p, int size, typename RWops::EndCB cb) {
		return base_type::acquire(RWops::FromMem(p,size, cb));
	}
	HLRW RWMgr::fromFP(FILE* fp, bool bAutoClose, const char* mode) {
		return base_type::acquire(RWops::FromFilePointer(fp, bAutoClose, mode));
	}
}
