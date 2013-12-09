#include "glx_parse.hpp"
#include "spinner/macro.hpp"

namespace rs {
	namespace {
		const static char* c_shType[] = {"VertexShader", "GeometryShader", "PixelShader"};
	}
	// (prec) type name の順に出力
	void EntryBase::output(std::ostream& os, const char* prefix, char postfix) const {
		os << prefix;
		if(prec)
			os << GLPrecision_::cs_typeStr[*prec] << ' ';
		os << GLType_::cs_typeStr[type] << ' ';
		os.write(name.c_str(), name.length());
		os << postfix;
	}

	void AttrEntry::output(std::ostream& os) const {
		EntryBase::output(os, "attribute ", ';');
	}
	void VaryEntry::output(std::ostream& os) const {
		EntryBase::output(os, "varying ", ';');
	}
	// nameの代わりにsemanticを出力
	void UnifEntry::output(std::ostream& os) const {
		EntryBase::output(os, "uniform ", ';');
	}

	void ConstEntry::output(std::ostream& os) const {
		EntryBase::output(os, "const ", '=');

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
		boost::apply_visitor(tmp, defVal);
		os << ';';
	}

	void ShStruct::output(std::ostream& os) const {
		using std::endl;

		os << '"' << name << '"' << endl;
		os << "type: " << c_shType[type] << endl;
		os << "args: ";
		for(auto& a : args)
			os << GLType_::cs_typeStr[a.type] << ' ' << a.name << ", ";
		os << endl;
	}

	void BlockUse::output(std::ostream& os) const {
		using std::endl;

		os << GLBlocktype_::cs_typeStr[type] << endl;
		os << "derives: ";
		for(auto& a : name)
			os << a << ", ";
		os << endl;
	}
	void BoolSetting::output(std::ostream& os) const {
		os << type << ": value=" << value << std::endl;
	}
	void MacroEntry::output(std::ostream& os) const {
		// Entry オンリーなら #define Entry
		// Entry=Value なら #define Entry Value とする
		os << "#define " << fromStr;
		if(toStr)
			os << ' ' << *toStr;
		os << std::endl;
	}
	void ValueSetting::output(std::ostream& os) const {
		os << GLSetting_::cs_typeStr[type] << "= ";
		for(auto& a : value)
			os << a << ' ';
		os << std::endl;
	}

	#define SEQ_TPS_ (blkL, BlockUse)(bsL, BoolSetting)(mcL, MacroEntry)(shL, ShSetting)(vsL, ValueSetting)(tpL, Pass)
	#define SEQ_TPS MAKE_SEQ(2, SEQ_TPS_)
	#define SEQ_GLX_ (atM, Attribute)(csM, Const)(shM, Shader)(uniM, Uniform)(varM, Varying)
	#define SEQ_GLX MAKE_SEQ(2, SEQ_GLX_)
	#define PRINTIT(ign,data,elem) for(auto& a : BOOST_PP_TUPLE_ELEM(0,elem)) {PrintIt(BOOST_PP_STRINGIZE(BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(1,elem), _array)), a);}
	#define PRINTITM(ign,data,elem) for(auto& a : BOOST_PP_TUPLE_ELEM(0,elem)) {PrintIt(BOOST_PP_STRINGIZE(BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(1,elem), _map)), a.second);}
	namespace {
		template <class T>
		void PrintIt(const T& t) {
			t.output(std::cout);
		}
		void PrintIt(const boost::recursive_wrapper<TPStruct>& tp) {
			tp.get().output(std::cout);
		}
		template <class T>
		void PrintIt(const char* msg, const T& t) {
			std::cout << msg << std::endl;
			PrintIt(t);
			std::cout << "-------------" << std::endl;
		}
		struct Visitor : boost::static_visitor<> {
			void operator()(float f) const {
				std::cout << "float(" << f << ')'; }
			void operator()(bool b) const {
				std::cout << "bool(" << b << ')'; }

			template <class T>
			void operator()(const std::vector<T>& ar) const {
				std::cout << "vector[";
				for(auto& a : ar) {
					std::cout << a << ' ';
				}
				std::cout << ']';
			}
		};
	}

	void ShSetting::output(std::ostream& os) const {
		os << c_shType[type] << "= " << shName << std::endl << "args: ";
		for(auto& a : args) {
			boost::apply_visitor(Visitor(), a);
			std::cout << ", ";
		}
		std::cout << std::endl;
	}
	void TPStruct::output(std::ostream&) const {
		BOOST_PP_SEQ_FOR_EACH(PRINTIT, _, SEQ_TPS)
	}
	void GLXStruct::output(std::ostream&) const {
		BOOST_PP_SEQ_FOR_EACH(PRINTITM, _, SEQ_GLX)
		for(auto& a : tpL)
			PrintIt("Tech: ", a);
	}
}
