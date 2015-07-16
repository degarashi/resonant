#include "tokenmemory.hpp"
#include "spinner/error.hpp"

namespace rs {
	namespace draw {
		// ------------------- TokenMemory -------------------
		namespace {
			template <class F>
			void Proc(uint8_t* data, std::size_t used, F&& f) {
				std::size_t cur = 0;
				while(cur < used) {
					auto ptr = reinterpret_cast<intptr_t>(data + cur);
					auto ofs = *reinterpret_cast<intptr_t*>(ptr);
					ptr += ofs+sizeof(intptr_t);
					auto* token = reinterpret_cast<Token*>(ptr);
					auto cur_ofs = token->getSize();
					std::forward<F>(f)(token);
					cur += cur_ofs + sizeof(intptr_t);
				}
			}
		}
		TokenMemory::TokenMemory(std::size_t s):
			_buffer(s),
			_used(0)
		{}
		TokenMemory::~TokenMemory() {
			Proc(_buffer.data(), _used, [](auto* token){
				token->~Token();
			});
		}
		void* TokenMemory::getMemory(std::size_t s, intptr_t ofs) {
			AssertP(Trap, ofs >= 0)
			if(_buffer.size() < s+_used+sizeof(intptr_t))
				return nullptr;
			auto ptr = reinterpret_cast<intptr_t>(_buffer.data() + _used);
			*reinterpret_cast<intptr_t*>(ptr) = ofs;
			ptr += sizeof(intptr_t);
			_used += s+sizeof(intptr_t);
			return reinterpret_cast<void*>(ptr);
		}
		void TokenMemory::exec() {
			Proc(_buffer.data(), _used, [](auto* token){
				token->exec();
			});
		}
		void TokenMemory::clear() {
			_used = 0;
		}
		TokenMemory::TokenMemory(TokenMemory&& t):
			_buffer(std::move(t._buffer)),
			_used(t._used)
		{
			t.clear();
		}
		TokenMemory& TokenMemory::operator = (TokenMemory&& t) {
			_buffer = std::move(t._buffer);
			t.clear();
			return *this;
		}
		// ------------------- TokenBuffer -------------------
		TokenBuffer::TokenBuffer(): _bInit(false) {}
		TokenBuffer::TokenBuffer(TokenBuffer&& t): TokenBuffer() {
			if(t._bInit)
				t.asToken()->takeout(*this);
			t._bInit = false;
		}
		TokenBuffer::~TokenBuffer() {
			if(_bInit) {
				asToken()->~Token();
				_bInit = false;
			}
		}
		Token* TokenBuffer::asToken() {
			AssertP(Trap, _bInit)
			auto p = reinterpret_cast<intptr_t>(_data.data()) + _offset;
			return reinterpret_cast<Token*>(p);
		}
		const Token* TokenBuffer::asToken() const {
			return const_cast<TokenBuffer*>(this)->asToken();
		}
		void* TokenBuffer::allocate_memory(std::size_t s, intptr_t ofs) {
			AssertP(Trap, _data.size() >= s)
			AssertP(Trap, ofs >= 0)
			if(_bInit)
				asToken()->~Token();
			else
				_bInit = true;
			_offset = ofs;
			return _data.data();
		}
		void TokenBuffer::exec() {
			asToken()->exec();
		}
		void TokenBuffer::clone(TokenDst& dst) const {
			asToken()->clone(dst);
		}
		void TokenBuffer::takeout(TokenDst& dst) {
			asToken()->takeout(dst);
		}
		std::size_t TokenBuffer::getSize() const {
			return asToken()->getSize();
		}
		// ------------------- TokenML -------------------
		TokenML::TokenML(std::size_t s): _laneSize(s) {}
		void* TokenML::allocate_memory(std::size_t s, intptr_t ofs) {
			if(_buffer.empty())
				_buffer.emplace_back(_laneSize);
			else {
				if(void* ret = _buffer.back().getMemory(s, ofs))
					return ret;
				_buffer.emplace_back(_laneSize);
			}
			void* ret = _buffer.back().getMemory(s, ofs);
			AssertP(Trap, ret)
			return ret;
		}
		void TokenML::setLaneSize(std::size_t s) {
			_laneSize = s;
		}
		void TokenML::exec() {
			for(auto& b : _buffer)
				b.exec();
		}
		void TokenML::clear() {
			_buffer.clear();
		}
		void TokenML::append(TokenML&& ml) {
			std::move(ml._buffer.begin(), ml._buffer.end(), std::back_inserter(_buffer));
			ml._buffer.clear();
		}
	}
}
