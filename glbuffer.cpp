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
		Buffer::Buffer(Buffer&& b): GLBufferCore(std::move(b)), Token(std::move(b)) {}
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
	GLBuffer::~GLBuffer() {
		onDeviceLost();
	}
	void GLBuffer::_initData() {
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
	draw::Buffer GLBuffer::getDrawToken(IPreFunc& pf, HRes hRes) const {
		pf.addPreFunc(std::move(_preFunc));
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
	void GLIBuffer::initData(spn::ByteBuff&& buff) {
		GLBuffer::initData(std::forward<spn::ByteBuff>(buff), sizeof(GLubyte));
	}
	void GLIBuffer::initData(const spn::U16Buff& buff) {
		GLBuffer::initData(reinterpret_cast<const void*>(&buff[0]), buff.size(), sizeof(GLushort));
	}
	void GLIBuffer::updateData(const GLubyte* src, size_t nElem, GLuint offset) {
		GLBuffer::updateData(src, nElem, offset*sizeof(GLubyte));
	}
	void GLIBuffer::updateData(const GLushort* src, size_t nElem, GLuint offset) {
		GLBuffer::updateData(src, nElem, offset*sizeof(GLushort));
	}
}
