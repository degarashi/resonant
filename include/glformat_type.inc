// (Typeフラグ, 要素のバイトサイズ)
const static std::pair<GLenum, size_t> c_GLTypeList[] = {
	{GL_UNSIGNED_BYTE, sizeof(GLubyte)},
	{GL_UNSIGNED_SHORT_5_6_5, sizeof(GLushort)},
	{GL_UNSIGNED_SHORT_4_4_4_4, sizeof(GLushort)},
	{GL_UNSIGNED_SHORT_5_5_5_1, sizeof(GLushort)},
#ifndef USE_OPENGLES2
	{GL_BYTE, sizeof(GLbyte)},
	{GL_UNSIGNED_BYTE_3_3_2, sizeof(GLubyte)},
	{GL_UNSIGNED_BYTE_2_3_3_REV, sizeof(GLubyte)},
	{GL_UNSIGNED_SHORT_5_6_5_REV, sizeof(GLushort)},
	{GL_UNSIGNED_SHORT_4_4_4_4_REV, sizeof(GLushort)},
	{GL_UNSIGNED_SHORT, sizeof(GLushort)},
	{GL_SHORT, sizeof(GLshort)},
	{GL_UNSIGNED_INT, sizeof(GLuint)},
	{GL_INT, sizeof(GLint)},
	{GL_HALF_FLOAT, sizeof(GLshort)},
	{GL_FLOAT, sizeof(GLfloat)},
	{GL_UNSIGNED_SHORT_1_5_5_5_REV, sizeof(GLushort)},
	{GL_UNSIGNED_INT_8_8_8_8, sizeof(GLuint)},
	{GL_UNSIGNED_INT_8_8_8_8_REV, sizeof(GLuint)},
	{GL_UNSIGNED_INT_10_10_10_2, sizeof(GLuint)},
	{GL_UNSIGNED_INT_2_10_10_10_REV, sizeof(GLuint)},
	{GL_UNSIGNED_INT_24_8, sizeof(GLuint)},
	{GL_UNSIGNED_INT_10F_11F_11F_REV, sizeof(GLuint)},
	{GL_UNSIGNED_INT_5_9_9_9_REV, sizeof(GLuint)},
	{GL_FLOAT_32_UNSIGNED_INT_24_8_REV, sizeof(GLuint)}
#endif
};
