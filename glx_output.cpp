#include "glx_parse.hpp"
#include "spinner/macro.hpp"

namespace rs {
	namespace {
		const static char* c_shType[] = {"VertexShader", "GeometryShader", "FragmentShader"};
	}

	void OutputComment(std::ostream& os, const std::string& msg) {
		const char* c_separate = "-------------";
		os << "// " << c_separate << ' ' << msg << ' ' << c_separate << std::endl;
	}
	// (prec) type name の順に出力
	std::ostream& operator << (std::ostream& os, const EntryBase& e) {
		if(e.prec)
			os << GLPrecision_::cs_typeStr[*e.prec] << ' ';
		os << GLType_::cs_typeStr[e.type] << ' ';
		os.write(e.name.c_str(), e.name.length());
		return os;
	}

	std::ostream& operator << (std::ostream& os, const AttrEntry& e) {
		return os << "attribute " << static_cast<const EntryBase&>(e) << ';';
	}
	std::ostream& operator << (std::ostream& os, const VaryEntry& e) {
		return os << "varying " << static_cast<const EntryBase&>(e) << ';';
	}
	// nameの代わりにsemanticを出力
	std::ostream& operator << (std::ostream& os, const UnifEntry& e) {
		os << "uniform " << static_cast<const EntryBase&>(e);
		if(e.arraySize) {
			os << '[' << *e.arraySize << ']';
		}
		return os << ';';
	}

	std::ostream& operator << (std::ostream& os, const ConstEntry& e) {
		os << "const " << static_cast<const EntryBase&>(e) << '=';

		struct Tmp : boost::static_visitor<> {
			std::ostream& _dst;
			Tmp(std::ostream& dst): _dst(dst) {}

			void operator()(bool b) {
				// bool, floatの時は値だけを出力
				_dst << b;
			}
			void operator()(float v) {
				_dst << v;
			}
			void operator()(const std::vector<float>& v) {
				// ベクトル値は括弧を付ける
				int nV = v.size();
				_dst << nV << '(';
				for(int i=0 ; i<nV-1 ; i++)
					_dst << v[i] << ',';
				_dst << v.back() << ')';
			}
		};

		Tmp tmp(os);
		boost::apply_visitor(tmp, e.defVal);
		return os << ';';
	}

	std::ostream& operator << (std::ostream& os, const CodeStruct& s) {
		OutputComment(os, (boost::format("code block \"%1%\"") % s.name).str());
		os << s.info << std::endl;
		return os;
	}
	std::ostream& operator << (std::ostream& os, const ShStruct& s) {
		using std::endl;

		os << '"' << s.name << '"' << endl;
		os << "version: " << s.version_str << endl;
		os << "type: " << c_shType[s.type] << endl;
		os << "codeblock: ";
		for(auto& c : s.code)
			os << c << ' ';
		os << endl;
		os << "args: ";
		for(auto& a : s.args)
			os << GLType_::cs_typeStr[a.type] << ' ' << a.name << ", ";
		return os << endl;
	}

	std::ostream& operator << (std::ostream& os, const BlockUse& b) {
		using std::endl;

		os << GLBlocktype_::cs_typeStr[b.type] << endl;
		os << "derives: ";
		for(auto& a : b.name)
			os << a << ", ";
		return os << endl;
	}
	std::ostream& operator << (std::ostream& os, const BoolSetting& s) {
		return os << s.type << ": value=" << s.value << std::endl;
	}
	std::ostream& operator << (std::ostream& os, const MacroEntry& e) {
		// Entry オンリーなら #define Entry
		// Entry=Value なら #define Entry Value とする
		os << "#define " << e.fromStr;
		if(e.toStr)
			os << ' ' << *e.toStr;
		return os << std::endl;
	}
	std::ostream& operator << (std::ostream& os, const ValueSetting& s) {
		os << GLSetting_::cs_typeStr[s.type] << "= ";
		for(auto& a : s.value)
			os << a << ' ';
		return os << std::endl;
	}

	#define SEQ_TPS_ (blkL, BlockUse)(bsL, BoolSetting)(mcL, MacroEntry)(shL, ShSetting)(vsL, ValueSetting)(tpL, Pass)
	#define SEQ_TPS MAKE_SEQ(2, SEQ_TPS_)
	#define SEQ_GLX_ (atM, Attribute)(csM, Const)(shM, Shader)(uniM, Uniform)(varM, Varying)(codeM, Code)
	#define SEQ_GLX MAKE_SEQ(2, SEQ_GLX_)
	#define PRINTIT(ign,data,elem) for(auto& a : data.BOOST_PP_TUPLE_ELEM(0,elem)) {PrintIt(os, BOOST_PP_STRINGIZE(BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(1,elem), _array)), a);}
	#define PRINTITM(ign,data,elem) for(auto& a : data.BOOST_PP_TUPLE_ELEM(0,elem)) {PrintIt(os, BOOST_PP_STRINGIZE(BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(1,elem), _map)), a.second);}
	namespace {
		template <class T>
		void PrintIt(std::ostream& os, const T& t) {
			os << t;
		}
		void PrintIt(std::ostream& os, const boost::recursive_wrapper<TPStruct>& tp) {
			os << tp.get();
		}
		template <class T>
		void PrintIt(std::ostream& os, const char* msg, const T& t) {
			os << msg << std::endl;
			PrintIt(os, t);
			os << "-------------" << std::endl;
		}
		struct Visitor : boost::static_visitor<> {
			std::ostream& _dst;
			Visitor(std::ostream& os): _dst(os) {}
			void operator()(float f) const {
				_dst << "float(" << f << ')'; }
			void operator()(bool b) const {
				_dst << "bool(" << b << ')'; }

			template <class T>
			void operator()(const std::vector<T>& ar) const {
				_dst << "vector[";
				for(auto& a : ar) {
					_dst << a << ' ';
				}
				_dst << ']';
			}
		};
	}

	std::ostream& operator << (std::ostream& os, const ShSetting& s) {
		os << c_shType[s.type] << "= " << s.shName << std::endl << "args: ";
		for(auto& a : s.args) {
			boost::apply_visitor(Visitor(os), a);
			os << ", ";
		}
		return os << std::endl;
	}
	std::ostream& operator << (std::ostream& os, const TPStruct& t) {
		BOOST_PP_SEQ_FOR_EACH(PRINTIT, t, SEQ_TPS)
		return os;
	}
	std::ostream& operator << (std::ostream& os, const GLXStruct& glx) {
		BOOST_PP_SEQ_FOR_EACH(PRINTITM, glx, SEQ_GLX)
		for(auto& a : glx.tpL)
			PrintIt(os, "Tech: ", a);
		return os;
	}
}
