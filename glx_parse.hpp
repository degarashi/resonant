#pragma once
#define BOOST_NO_CXX11_DECLTYPE_N3276
#include "glhead.hpp"
#include "glx_macro.hpp"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/optional.hpp>
#include <iostream>
#include <string>

#define TRANSFORM_STRUCT_MEMBER(ign, name, member) (decltype(name::member), member)
#define FUSION_ADAPT_STRUCT_AUTO(name, members) \
	BOOST_FUSION_ADAPT_STRUCT(name, BOOST_PP_SEQ_FOR_EACH(TRANSFORM_STRUCT_MEMBER, name, members))

#define DEF_TYPE(typ, name, seq) struct typ##_ : qi::symbols<char,unsigned> { \
			enum TYPE { BOOST_PP_SEQ_FOR_EACH(PPFUNC_ENUM, T, seq) }; \
			const static char* cs_typeStr[BOOST_PP_SEQ_SIZE(seq)]; \
			typ##_(): symbols(std::string(name)) { \
				add \
				BOOST_PP_SEQ_FOR_EACH(PPFUNC_ADD, T, seq); } }; \
		extern const typ##_ typ;
namespace rs {
	enum class VSem : unsigned int {
		BOOST_PP_SEQ_ENUM(SEQ_VSEM),
		NUM_SEMANTIC
	};
}
using namespace boost::spirit;
namespace rs {
	// ---------------- GLXシンボルリスト ----------------
	//! GLSL変数型
	DEF_TYPE(GLType, "GLSL-ValueType", SEQ_GLTYPE)
	//! GLSL変数入出力フラグ
	DEF_TYPE(GLInout, "InOut-Flag", SEQ_INOUT)
	//! GLSL頂点セマンティクス
	DEF_TYPE(GLSem, "VertexSemantics", SEQ_VSEM)
	//! GLSL浮動小数点数精度
	DEF_TYPE(GLPrecision, "PrecisionFlag", SEQ_PRECISION)
	//! boolでフラグを切り替える項目
	struct GLBoolsetting_ : qi::symbols<char, unsigned> {
		GLBoolsetting_(): symbols(std::string("BooleanSettingName")) {
			add("cullface", GL_CULL_FACE)
				("polyoffsetfill", GL_POLYGON_OFFSET_FILL)
				("scissortest", GL_SCISSOR_TEST)
				("samplealphatocoverage", GL_SAMPLE_ALPHA_TO_COVERAGE)
				("samplecoverage", GL_SAMPLE_COVERAGE)
				("stenciltest", GL_STENCIL_TEST)
				("depthtest", GL_DEPTH_TEST)
				("blend", GL_BLEND)
				("dither", GL_DITHER);
		}
	};
	//! 数値指定するタイプの設定項目フラグ
	struct GLSetting_ : qi::symbols<char, unsigned> {
		enum TYPE { BOOST_PP_SEQ_FOR_EACH(PPFUNC_GLSET_ENUM, T, SEQ_GLSETTING) };
		const static char* cs_typeStr[BOOST_PP_SEQ_SIZE(SEQ_GLSETTING)];
		GLSetting_(): symbols(std::string("SettingName")) {
				add
				BOOST_PP_SEQ_FOR_EACH(PPFUNC_GLSET_ADD, T, SEQ_GLSETTING);
		}
	};
	//! 設定項目毎に用意したほうがいい？
	struct GLStencilop_ : qi::symbols<char, unsigned> {
		GLStencilop_(): symbols(std::string("StencilOperator")) {
			add("keep", GL_KEEP)
				("zero", GL_ZERO)
				("replace", GL_REPLACE)
				("increment", GL_INCR)
				("decrement", GL_DECR)
				("invert", GL_INVERT)
				("incrementwrap", GL_INCR_WRAP)
				("decrementwrap", GL_DECR_WRAP);
		}
	};
	struct GLFunc_ : qi::symbols<char, unsigned> {
		GLFunc_(): symbols(std::string("CompareFunction")) {
			add("never", GL_NEVER)
				("always", GL_ALWAYS)
				("less", GL_LESS)
				("lessequal", GL_LEQUAL)
				("equal", GL_EQUAL)
				("greater", GL_GREATER)
				("greaterequal", GL_GEQUAL)
				("notequal", GL_NOTEQUAL);
		}
	};
	struct GLEq_ : qi::symbols<char, unsigned> {
		GLEq_(): symbols(std::string("BlendEquation")) {
			add("add", GL_FUNC_ADD)
				("subtract", GL_FUNC_SUBTRACT)
				("invsubtract", GL_FUNC_REVERSE_SUBTRACT);
		}
	};
	struct GLBlend_ : qi::symbols<char, unsigned> {
		GLBlend_(): symbols(std::string("BlendOperator")) {
			add("zero", GL_ZERO)
				("one", GL_ONE)
				("invsrccolor", GL_ONE_MINUS_SRC_COLOR)
				("invdstcolor", GL_ONE_MINUS_DST_COLOR)
				("srccolor", GL_SRC_COLOR)
				("dstcolor", GL_DST_COLOR)
				("invsrcalpha", GL_ONE_MINUS_SRC_ALPHA)
				("invdstalpha", GL_ONE_MINUS_DST_ALPHA)
				("srcalpha", GL_SRC_ALPHA)
				("dstalpha", GL_DST_ALPHA)
				("invconstantalpha", GL_ONE_MINUS_CONSTANT_ALPHA)
				("constantalpha", GL_CONSTANT_ALPHA)
				("srcalphasaturate", GL_SRC_ALPHA_SATURATE);
		}
	};
	struct GLFace_ : qi::symbols<char, unsigned> {
		GLFace_(): symbols(std::string("Face(Front or Back)")) {
			add("front", GL_FRONT)
				("back", GL_BACK)
				("frontandback", GL_FRONT_AND_BACK);
		}
	};
	struct GLFacedir_ : qi::symbols<char, unsigned> {
		GLFacedir_(): symbols(std::string("FaceDir")) {
			add("ccw", GL_CCW)
				("cw", GL_CW);
		}
	};
	struct GLColormask_ : qi::symbols<char, unsigned> {
		GLColormask_(): symbols(std::string("ColorMask")) {
			add("r", 0x08)
				("g", 0x04)
				("b", 0x02)
				("a", 0x01);
		}
	};
	struct GLShadertype_ : qi::symbols<char, unsigned> {
		GLShadertype_(): symbols(std::string("ShaderType")) {
			add("vertexshader", (unsigned)rs::ShType::VERTEX)
				("fragmentshader", (unsigned)rs::ShType::FRAGMENT)
				("geometryshader", (unsigned)rs::ShType::GEOMETRY);
		}
	};
	//! 変数ブロックタイプ
	DEF_TYPE(GLBlocktype, "BlockType", SEQ_BLOCK)

	extern const GLSetting_ GLSetting;
	extern const GLBoolsetting_ GLBoolsetting;
	extern const GLStencilop_ GLStencilop;
	extern const GLFunc_ GLFunc;
	extern const GLEq_ GLEq;
	extern const GLBlend_ GLBlend;
	extern const GLFace_ GLFace;
	extern const GLFacedir_ GLFacedir;
	extern const GLColormask_ GLColormask;
	extern const GLShadertype_ GLShadertype;

	// ---------------- GLX構文解析データ ----------------
	struct EntryBase {
		boost::optional<unsigned>	prec;
		int							type;
		std::string					name;
	};
	std::ostream& operator << (std::ostream& os, const EntryBase& e);
	//! Attribute宣言エントリ
	struct AttrEntry : EntryBase {
		unsigned					sem;
	};
	std::ostream& operator << (std::ostream& os, const AttrEntry& e);
	//! Varying宣言エントリ
	struct VaryEntry : EntryBase {};
	std::ostream& operator << (std::ostream& os, const VaryEntry& e);
	//! Uniform宣言エントリ
	struct UnifEntry : EntryBase {
		boost::optional<int>	arraySize;
		boost::optional<boost::variant<std::vector<float>, float, bool>>	defStr;
	};
	std::ostream& operator << (std::ostream& os, const UnifEntry& e);
	//! Const宣言エントリ
	struct ConstEntry : EntryBase {
		boost::variant<bool, float, std::vector<float>>		defVal;
	};
	std::ostream& operator << (std::ostream& os, const ConstEntry& e);
	//! Bool設定項目エントリ
	struct BoolSetting {
		GLuint				type;		//!< 設定項目ID
		bool				value;		//!< 設定値ID or 数値
	};
	std::ostream& operator << (std::ostream& os, const BoolSetting& s);
	//! 数値設定エントリ
	struct ValueSetting {
		using ValueT = boost::variant<boost::blank,unsigned int,float,bool>;

		GLSetting_::TYPE 		type;
		std::vector<ValueT>		value;
	};
	std::ostream& operator << (std::ostream& os, const ValueSetting& s);

	//! 変数ブロック使用宣言
	struct BlockUse {
		unsigned					type;	// Attr,Vary,Unif,Const
		bool						bAdd;
		std::vector<std::string>	name;
	};
	std::ostream& operator << (std::ostream& os, const BlockUse& b);
	//! シェーダー設定エントリ
	struct ShSetting {
		int							type;
		std::string					shName;
		// 引数指定
		std::vector<boost::variant<std::vector<float>, float, bool>>	args;
	};
	std::ostream& operator << (std::ostream& os, const ShSetting& s);
	//! マクロ宣言エントリ
	struct MacroEntry {
		std::string						fromStr;
		boost::optional<std::string>	toStr;
	};
	std::ostream& operator << (std::ostream& os, const MacroEntry& e);

	template <class Ent>
	struct Struct {
		std::string					name;
		std::vector<std::string>	derive;
		std::vector<Ent>			entry;

		Struct() = default;
		Struct(const Struct& a) = default;
		Struct(Struct&& a): Struct() { swap(a); }
		Struct& operator = (const Struct& a) {
			this->~Struct();
			new(this) Struct(a);
			return *this;
		}
		void swap(Struct& a) noexcept {
			std::swap(name, a.name);
			std::swap(derive, a.derive);
			std::swap(entry, a.entry);
		}
		void iterate(std::function<void (const Ent&)> cb) const {
			for(const auto& e : entry)
				cb(e);
		}
		void output(std::ostream& os) const {
			using std::endl;

			// print name
			os << '"' << name << '"' << endl;

			// print derives
			os << "derives: ";
			for(auto& d : derive)
				os << d << ", ";
			os << endl;

			// print entries
			for(auto& e : entry) {
				os << e << endl;
			}
		}
	};
	using AttrStruct = Struct<AttrEntry>;
	using VaryStruct = Struct<VaryEntry>;
	using UnifStruct = Struct<UnifEntry>;
	using ConstStruct = Struct<ConstEntry>;
	template <class T>
	std::ostream& operator << (std::ostream& os, const Struct<T>& s) {
		s.output(os);
		return os;
	}

	struct ArgItem {
		int			type;
		std::string	name;

		void output(std::ostream& os) const;
	};
	struct ShStruct {
		//! シェーダータイプ
		uint32_t				type;
		//! バージョン文字列
		std::string				version_str;
		//! シェーダー名
		std::string				name;
		//! 引数群(型ID + 名前)
		std::vector<ArgItem>	args;
		//! シェーダーの中身(文字列)
		std::string				info;

		void swap(ShStruct& a) noexcept;
		void output(std::ostream& os) const;
	};

	template <typename T>
	using NameMap = std::map<std::string, T>;
	//! Tech,Pass
	struct TPStruct;
	struct TPStruct {
		std::string					name;

		std::vector<BlockUse>		blkL;
		std::vector<BoolSetting> 	bsL;
		std::vector<MacroEntry>		mcL;
		std::vector<ShSetting>		shL;
		std::vector<boost::recursive_wrapper<TPStruct>>		tpL;
		std::vector<ValueSetting> 	vsL;

		std::vector<std::string>	derive;
	};
	std::ostream& operator << (std::ostream& os, const TPStruct& t);

	//! 括弧の中身
	struct Bracket;
	struct Bracket {
		std::string str;
		boost::optional<boost::recursive_wrapper<Bracket>> child;
	};

	//! エフェクト全般
	struct GLXStruct {
		NameMap<AttrStruct>		atM;
		NameMap<ConstStruct>	csM;
		NameMap<ShStruct>		shM;
		std::vector<TPStruct>	tpL;
		NameMap<UnifStruct>		uniM;
		NameMap<VaryStruct>		varM;
	};
	std::ostream& operator << (std::ostream& os, const GLXStruct& glx);
}
FUSION_ADAPT_STRUCT_AUTO(rs::AttrEntry, (prec)(type)(name)(sem))
FUSION_ADAPT_STRUCT_AUTO(rs::VaryEntry, (prec)(type)(name))
FUSION_ADAPT_STRUCT_AUTO(rs::UnifEntry, (prec)(type)(name)(arraySize)(defStr))
FUSION_ADAPT_STRUCT_AUTO(rs::ConstEntry, (prec)(type)(name)(defVal))
FUSION_ADAPT_STRUCT_AUTO(rs::BoolSetting, (type)(value))
FUSION_ADAPT_STRUCT_AUTO(rs::ValueSetting, (type)(value))
FUSION_ADAPT_STRUCT_AUTO(rs::ShSetting, (type)(shName)(args))
FUSION_ADAPT_STRUCT_AUTO(rs::MacroEntry, (fromStr)(toStr))
FUSION_ADAPT_STRUCT_AUTO(rs::AttrStruct, (name)(derive)(entry))
FUSION_ADAPT_STRUCT_AUTO(rs::VaryStruct, (name)(derive)(entry))
FUSION_ADAPT_STRUCT_AUTO(rs::UnifStruct, (name)(derive)(entry))
FUSION_ADAPT_STRUCT_AUTO(rs::ConstStruct, (name)(derive)(entry))
FUSION_ADAPT_STRUCT_AUTO(rs::ShStruct, (type)(version_str)(name)(args)(info))
FUSION_ADAPT_STRUCT_AUTO(rs::TPStruct, (name)(blkL)(bsL)(mcL)(shL)(tpL)(vsL)(derive))
FUSION_ADAPT_STRUCT_AUTO(rs::Bracket, (str)(child))
FUSION_ADAPT_STRUCT_AUTO(rs::GLXStruct, (atM)(csM)(shM)(tpL)(uniM)(varM))
FUSION_ADAPT_STRUCT_AUTO(rs::ArgItem, (type)(name))
FUSION_ADAPT_STRUCT_AUTO(rs::BlockUse, (type)(bAdd)(name))

namespace rs {
	using Itr = std::string::const_iterator;
	//! GLX構文解析器
	struct GR_Glx : qi::grammar<Itr, GLXStruct(), standard::space_type> {
		void _initRule0();
		void _initRule1();

		// GLX文法のEBNF記述
		qi::rule<Itr, std::string(), standard::space_type> 			rlString,		//!< "で囲まれた文字列
																	rlNameToken;	//!< 文字列トークン
		qi::rule<Itr, Bracket(char,char), standard::space_type>		rlBracket;		//!< 任意の括弧で囲まれたブロック
		// 変数宣言(attribute, uniform, varying)
		qi::rule<Itr, AttrEntry(), standard::space_type> 				rlAttrEnt;
		qi::rule<Itr, VaryEntry(), standard::space_type>				rlVaryEnt;
		qi::rule<Itr, UnifEntry(), standard::space_type>				rlUnifEnt;
		qi::rule<Itr, ConstEntry(), standard::space_type>				rlConstEnt;
		qi::rule<Itr, MacroEntry(), standard::space_type>				rlMacroEnt;

		qi::rule<Itr, BoolSetting(), standard::space_type>				rlBoolSet;
		qi::rule<Itr, ValueSetting(), standard::space_type>				rlValueSet;
		qi::rule<Itr, BlockUse(), standard::space_type>					rlBlockUse;

		qi::rule<Itr, ShSetting(), standard::space_type>				rlShSet;
		// 各種定義ブロック
		qi::rule<Itr, AttrStruct(), standard::space_type>				rlAttrBlock;
		qi::rule<Itr, VaryStruct(), standard::space_type>				rlVaryBlock;
		qi::rule<Itr, UnifStruct(), standard::space_type>				rlUnifBlock;
		qi::rule<Itr, ConstStruct(), standard::space_type>				rlConstBlock;
		qi::rule<Itr, ShStruct(), standard::space_type>					rlShBlock;
		qi::rule<Itr, std::vector<MacroEntry>(), standard::space_type>	rlMacroBlock;
		qi::rule<Itr, TPStruct(), standard::space_type>					rlPassBlock,
																		rlTechBlock;
		qi::rule<Itr, GLXStruct(), standard::space_type>				rlGLX;
		// 各種変数定義
		qi::rule<Itr, std::vector<float>(), standard::space_type>		rlVec;
		qi::rule<Itr, ArgItem(), standard::space_type>					rlArg;

		GR_Glx();
	};
}
