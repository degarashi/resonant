#include "glformat.hpp"

namespace rs {
	// ------------------------- GLFormat -------------------------
	GLFormat::GLFormat(GLenum fmt): value(fmt) {}
	bool GLFormat::Check(GLenum fmt, ID id) {
		auto itr = s_idMap->find(FmtID(id,fmt));
		return itr != s_idMap->end() && boost::get<ID_t>(itr->second) == Invalid;
	}
	GLFormat::ID GLFormat::QueryFormat(GLenum fmt, ID tag) {
		FmtID id(tag, fmt);
		auto itr = s_idMap->find(id);
		if(itr != s_idMap->end())
			return static_cast<ID>(boost::get<ID_t>(itr->second));
		return Invalid;
	}
	GLFormat::OPInfo GLFormat::QueryInfo(GLenum fmt) {
		auto itr = s_idMap->find(FmtID(Query_Info, fmt));
		if(itr != s_idMap->end())
			return boost::get<GLFormatDesc>(itr->second);
		return spn::none;
	}
	GLFormat::OPInfo GLFormat::QuerySDLtoGL(uint32_t fmt) {
		auto itr = s_SDLtoGL->find(fmt);
		if(itr != s_SDLtoGL->end())
			return itr->second;
		return spn::none;
	}
	size_t GLFormat::QuerySize(GLenum typ) {
		auto itr = s_idMap->find(FmtID(Query_TypeSize, typ));
		if(itr != s_idMap->end())
			return boost::get<ID_t>(itr->second);
		return 0;
	}
	size_t GLFormat::QueryByteSize(GLenum fmt, GLenum typ) {
		return QueryInfo(fmt)->numElem * QuerySize(typ);
	}
	const std::string& GLFormat::QueryEnumString(GLenum value) {
		for(auto& p : *s_valueMap) {
			if(p.first == value)
				return p.second;
		}
		const static std::string c_unknown("UNKNOWN_VALUE");
		return c_unknown;
	}
	GLFormat::OPGLSLInfo GLFormat::QueryGLSLInfo(GLenum fmt) {
		auto itr = s_idMap->find(FmtID(Query_GLSLTypeInfo, fmt));
		if(itr != s_idMap->end())
			return boost::get<GLSLFormatDesc>(itr->second);
		return spn::none;
	}
	GLenum GLFormat::get() const { return value; }
	const GLFormatV::RetFormatV GLFormatV::cs_retV[] = {
		[](GLenum fmt) { return GLDepthFmt(fmt, nullptr); },
		[](GLenum fmt) { return GLStencilFmt(fmt, nullptr); },
		[](GLenum fmt) { return GLDSFmt(fmt, nullptr); },
		[](GLenum fmt) { return GLInFmt(fmt, nullptr); },
		[](GLenum fmt) { return GLInSizedFmt(fmt, nullptr); },
		[](GLenum fmt) { return GLInCompressedFmt(fmt, nullptr); },
		[](GLenum fmt) { return GLInRenderFmt(fmt, nullptr); },
		[](GLenum fmt) { return GLInReadFmt(fmt, nullptr); },
		[](GLenum fmt) { return GLTypeFmt(fmt, nullptr); }
	};

	GLFormatV GLFormatV::Detect(GLenum fmt) {
		return cs_retV[static_cast<int>(GLFormat::QueryFormat(fmt, GLFormat::Query_All))](fmt);
	}

	namespace {
		struct TmpVisitor : boost::static_visitor<GLenum> {
			template <class T>
			GLenum operator()(const T& t) const {
				const GLFormat& fm = reinterpret_cast<const GLFormat&>(t);
				return fm.get();
			}
		};
	}
	GLenum GLFormatV::get() const {
		return boost::apply_visitor(TmpVisitor(), *this);
	}
	GLFormat::IDMap* GLFormat::s_idMap;
	GLFormat::SDLtoGL* GLFormat::s_SDLtoGL;
	GLFormat::GLValueMap* GLFormat::s_valueMap;

	constexpr uint32_t MakeDim() {
		return 0;
	}
	template <class T0, class... Ts>
	constexpr uint32_t MakeDim(T0 t, Ts... ts) {
		return (t << (sizeof...(ts)*3)) | MakeDim(ts...);
	}
	#define NOTHING
	#define ADD_IDMAP(fmtT, fmtGL, second)	s_idMap->insert(std::make_pair(FmtID(fmtT, fmtGL), second));
	#define ADD_FMTID1(z, data, elem)			ADD_IDMAP(data, elem, Invalid)
	#define ADD_FMTID_ALL(z, data, elem)		ADD_IDMAP(Query_All, elem, data)
	#define ADD_FMTID_DSC(z, data, elem)		ADD_IDMAP(Query_DSC, elem, data)
	void GLFormat::Initialize() {
		// エントリの予約数は適当
		s_idMap = new IDMap(1024);
		s_SDLtoGL = new SDLtoGL(1024);
		s_valueMap = new GLValueMap(1024);
		#include "opengl_define/glformat_const.inc"
		// フォーマット判定用エントリ
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID1, Internal, SEQ_INTERNAL)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID1, Internal_Sized, SEQ_INTERNALSIZED)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID1, Internal_Compressed, SEQ_INTERNALCOMPRESSED)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID1, Internal_Render, SEQ_INTERNALRENDER)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID1, Internal_Read, SEQ_INTERNALREAD)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID1, Type, SEQ_TYPE)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID1, DepthStencil, PSEQ_DSFORMAT)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID1, Depth, PSEQ_DEPTHFORMAT)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID1, Stencil, PSEQ_STENCILFORMAT)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID1, DepthStencil, PSEQ_DSFORMAT)
		// フォーマット検索用エントリ
		// より根本のクラスが優先される
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_ALL, DepthStencil, PSEQ_DSFORMAT)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_ALL, Stencil, PSEQ_STENCILFORMAT)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_ALL, Depth, PSEQ_DEPTHFORMAT)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_ALL, Type, SEQ_TYPE)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_ALL, Internal_Read, SEQ_INTERNALREAD)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_ALL, Internal_Render, SEQ_INTERNALRENDER)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_ALL, Internal_Compressed, SEQ_INTERNALCOMPRESSED)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_ALL, Internal_Sized, SEQ_INTERNALSIZED)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_ALL, Internal, SEQ_INTERNAL)

		// Depth | Stencil | DS | Color の判別用
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_DSC, Internal, SEQ_INTERNAL)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_DSC, DepthStencil, PSEQ_DSFORMAT)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_DSC, Stencil, PSEQ_STENCILFORMAT)
		BOOST_PP_SEQ_FOR_EACH(ADD_FMTID_DSC, Depth, PSEQ_DEPTHFORMAT)

		for(auto& p : c_GLFormatList)
			s_idMap->insert(std::make_pair(FmtID(Query_Info, p.format), p));
		for(auto& p : c_GLTypeList)
			s_idMap->insert(std::make_pair(FmtID(Query_TypeSize, p.first), p.second));
		for(auto& p : c_GLSLTypeList)
			s_idMap->insert(std::make_pair(FmtID(Query_GLSLTypeInfo, p.first), p.second));

		// SDLのフォーマット -> 対応するOpenGLフォーマット
		for(auto& p : c_SDLtoGL) {
			auto info = QueryInfo(p.second);
			AssertP(Trap, info)
			s_SDLtoGL->emplace(p.first, *info);
		}
		// 上記と同じだがc_GLFormatListの逆パターンで登録
		for(auto& p : c_GLFormatList) {
			auto itr = s_SDLtoGL->find(p.sdlFormat);
			if(p.sdlFormat!=SDL_PIXELFORMAT_UNKNOWN && itr==s_SDLtoGL->end())
				s_SDLtoGL->emplace(p.sdlFormat, p);
		}

		// --------------- Const変数名の登録 ---------------
		auto fnAddValue = [](std::string name, GLenum value){
			auto itr = s_valueMap->find(value);
			if(itr == s_valueMap->end()) {
				// エントリ作成
				s_valueMap->emplace(value, std::move(name));
			} else {
				// 併記
				itr->second.append(" or ");
				itr->second.append(name);
			}
		};
		#define DEF_GLMETHOD(...)
		#define GLDEFINE(...)
		#define DEF_GLCONST(name, value)	fnAddValue(#name, static_cast<GLenum>(value));
			#ifdef ANDROID
				#include "opengl_define/android_gl.inc"
			#elif defined(WIN32)
				#include "opengl_define/mingw_gl.inc"
			#else
				#include "opengl_define/linux_gl.inc"
			#endif
		#undef DEF_GLCONST
		#undef GLDEFINE
		#undef DEF_GLMETHOD
	}
	void GLFormat::Terminate() {
		delete s_valueMap;
		delete s_SDLtoGL;
		delete s_idMap;
	}
}
