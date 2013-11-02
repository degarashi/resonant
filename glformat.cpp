#include "glformat.hpp"

namespace rs {
	// ------------------------- GLFormat -------------------------
	GLFormat::GLFormat(GLenum fmt): value(fmt) {}
	bool GLFormat::Check(GLenum fmt, ID id) {
		auto itr = s_idMap->find(FmtID(id,fmt));
		return itr != s_idMap->end() && boost::get<uint32_t>(itr->second) == Invalid;
	}
	GLFormat::ID GLFormat::QueryFormat(GLenum fmt, ID tag) {
		FmtID id(tag, fmt);
		auto itr = s_idMap->find(id);
		if(itr != s_idMap->end())
			return boost::get<ID>(itr->second);
		return Invalid;
	}
	GLFormat::OPInfo GLFormat::QueryInfo(GLenum fmt) {
		auto itr = s_idMap->find(FmtID(Query_Info, fmt));
		if(itr != s_idMap->end())
			return boost::get<GLFormatDesc>(itr->second);
		return boost::none;
	}
	size_t GLFormat::QuerySize(GLenum typ) {
		auto itr = s_idMap->find(FmtID(Query_TypeSize, typ));
		if(itr != s_idMap->end())
			return boost::get<uint32_t>(itr->second);
		return 0;
	}
	size_t GLFormat::QueryByteSize(GLenum fmt, GLenum typ) {
		return QueryInfo(fmt)->numType * QuerySize(typ);
	}
	GLFormat::OPGLSLInfo GLFormat::QueryGLSLInfo(GLenum fmt) {
		auto itr = s_idMap->find(FmtID(Query_GLSLTypeInfo, fmt));
		if(itr != s_idMap->end())
			return boost::get<GLSLFormatDesc>(itr->second);
		return boost::none;
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
		s_idMap = new IDMap(4096);
		#include "glformat_const.inc"
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
			s_idMap->insert(std::make_pair(FmtID(Query_Info, p.first), p.second));
		for(auto& p : c_GLTypeList)
			s_idMap->insert(std::make_pair(FmtID(Query_TypeSize, p.first), p.second));
		for(auto& p : c_GLSLTypeList)
			s_idMap->insert(std::make_pair(FmtID(Query_GLSLTypeInfo, p.first), p.second));
	}
	void GLFormat::Terminate() {
		delete s_idMap;
	}
}
