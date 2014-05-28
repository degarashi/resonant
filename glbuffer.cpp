#include "glresource.hpp"

namespace rs {
	// --------------------------- GLBufferCore ---------------------------
	GLBufferCore::GLBufferCore(GLuint flag, GLuint dtype): _buffType(flag), _drawType(dtype), _stride(0), _idBuff(0) {}
	GLuint GLBufferCore::getBuffID() const { return _idBuff; }
	GLuint GLBufferCore::getBuffType() const { return _buffType; }
	GLuint GLBufferCore::getStride() const { return _stride; }
	RUser<GLBufferCore> GLBufferCore::use() const {
		return RUser<GLBufferCore>(*this);
	}
	void GLBufferCore::use_begin() const {
		GL.glBindBuffer(getBuffType(), getBuffID());
	}
	void GLBufferCore::use_end() const {
		GL.glBindBuffer(getBuffType(), 0);
	}

	// --------------------------- draw::Buffer ---------------------------
	namespace draw {
		Buffer::Buffer(const GLBufferCore& core, HRes hRes):
			GLBufferCore(core), Token(hRes) {}
		void Buffer::exec() {
			use_begin();
		}
	}

	// --------------------------- GLBuffer ---------------------------
	void GLBuffer::onDeviceLost() {
		if(_idBuff != 0) {
			GL.glDeleteBuffers(1, &_idBuff);
			_idBuff = 0;
		}
	}
	void GLBuffer::onDeviceReset() {
		if(_idBuff == 0) {
			GL.glGenBuffers(1, &_idBuff);
			if(_buff) {
				auto u = use();
				GL.glBufferData(_buffType, _buffSize, _pBuffer, _drawType);
			}
		}
	}
	GLBuffer::GLBuffer(GLuint flag, GLuint dtype): GLBufferCore(flag, dtype), _buffSize(0) {}
	GLBuffer::~GLBuffer() {
		onDeviceLost();
	}
	void GLBuffer::_initData() {
		Assert(Trap, getBuffID() > 0);
		_preFunc = [this]() {
			use_begin();
			GL.glBufferData(_buffType, _buffSize, _pBuffer, _drawType);
		};
	}
	void GLBuffer::initData(const void* src, size_t nElem, GLuint stride) {
		size_t sz = nElem*stride;
		spn::ByteBuff b(sz);
		std::memcpy(b.data(), src, sz);
		initData(std::move(b), stride);
	}
	void GLBuffer::updateData(const void* src, size_t nElem, GLuint offset) {
		size_t szCopy = nElem * _stride,
				ofs = offset*_stride;
		std::memcpy(reinterpret_cast<char*>(_pBuffer)+ofs, src, szCopy);
		_preFunc = [=]() {
			use_begin();
			GL.glBufferSubData(_buffType, ofs, szCopy, src);
		};
	}
	GLuint GLBuffer::getSize() const {
		return _buffSize;
	}
	GLuint GLBuffer::getNElem() const {
		return _buffSize / _stride;
	}
	draw::Buffer GLBuffer::getDrawToken(IPreFunc& pf, HRes hRes) const {
		if(_preFunc) {
			PreFunc pfunc(std::move(const_cast<GLBuffer*>(this)->_preFunc));
			HLRes hlRes(hRes);
			pf.addPreFunc([pfunc, hlRes]() {
				pfunc();
			});
		}
		return draw::Buffer(*this, hRes);
	}

	// --------------------------- GLVBuffer ---------------------------
	GLVBuffer::GLVBuffer(GLuint dtype): GLBuffer(GL_ARRAY_BUFFER, dtype) {}

	// --------------------------- GLIBuffer ---------------------------
	GLIBuffer::GLIBuffer(GLuint dtype): GLBuffer(GL_ELEMENT_ARRAY_BUFFER, dtype) {}
	void GLIBuffer::initData(const GLubyte* src, size_t nElem) {
		GLBuffer::initData(src, nElem, sizeof(GLubyte));
	}
	void GLIBuffer::initData(const GLushort* src, size_t nElem) {
		GLBuffer::initData(src, nElem, sizeof(GLushort));
	}
	void GLIBuffer::updateData(const GLubyte* src, size_t nElem, GLuint offset) {
		GLBuffer::updateData(src, nElem, offset*sizeof(GLubyte));
	}
	void GLIBuffer::updateData(const GLushort* src, size_t nElem, GLuint offset) {
		GLBuffer::updateData(src, nElem, offset*sizeof(GLushort));
	}
	GLenum GLIBuffer::getSizeFlag() const {
		return GetSizeFlag(getStride());
	}
	GLenum GLIBuffer::GetSizeFlag(int stride) {
		switch(stride) {
			case sizeof(GLubyte):
				return GL_UNSIGNED_BYTE;
			case sizeof(GLushort):
				return GL_UNSIGNED_SHORT;
			case sizeof(GLuint):
				return GL_UNSIGNED_INT;
			default:
				Assert(Trap, false, "unknown ibuffer size type");
		}
		return 0;
	}
}

