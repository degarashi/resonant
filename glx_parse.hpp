#pragma once
#define BOOST_NO_CXX11_DECLTYPE_N3276
#include "glhead.hpp"
#include "glx_macro.hpp"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/optional.hpp>
#include <iostream>
#include <string>

#define TRANSFORM_STRUCT_MEMBER(ign, name, member) (decltype(name::member), member)
#define FUSION_ADAPT_STRUCT_AUTO(name, members) \
	BOOST_FUSION_ADAPT_STRUCT(name, BOOST_PP_SEQ_FOR_EACH(TRANSFORM_STRUCT_MEMBER, name, members))

#define DEF_TYPE(typ, name, seq) struct typ##_ : x3::symbols<unsigned> { \
			enum TYPE { BOOST_PP_SEQ_FOR_EACH(PPFUNC_ENUM, T, seq) }; \
			const static char* cs_typeStr[BOOST_PP_SEQ_SIZE(seq)]; \
			typ##_(): x3::symbols<unsigned>(std::string(name)) { \
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
	struct GLBoolsetting_ : x3::symbols<unsigned> {
		GLBoolsetting_(): x3::symbols<unsigned>(std::string("BooleanSettingName")) {
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
	struct GLSetting_ : x3::symbols<unsigned> {
		enum TYPE : unsigned { BOOST_PP_SEQ_FOR_EACH(PPFUNC_GLSET_ENUM, T, SEQ_GLSETTING) };
		const static char* cs_typeStr[BOOST_PP_SEQ_SIZE(SEQ_GLSETTING)];
		GLSetting_(): x3::symbols<unsigned>(std::string("SettingName")) {
				add
				BOOST_PP_SEQ_FOR_EACH(PPFUNC_GLSET_ADD, T, SEQ_GLSETTING);
		}
	};
	//! 設定項目毎に用意したほうがいい？
	struct GLStencilop_ : x3::symbols<unsigned> {
		GLStencilop_(): x3::symbols<unsigned>(std::string("StencilOperator")) {
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
	struct GLFunc_ : x3::symbols<unsigned> {
		GLFunc_(): x3::symbols<unsigned>(std::string("CompareFunction")) {
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
	struct GLEq_ : x3::symbols<unsigned> {
		GLEq_(): x3::symbols<unsigned>(std::string("BlendEquation")) {
			add("add", GL_FUNC_ADD)
				("subtract", GL_FUNC_SUBTRACT)
				("invsubtract", GL_FUNC_REVERSE_SUBTRACT);
		}
	};
	struct GLBlend_ : x3::symbols<unsigned> {
		GLBlend_(): x3::symbols<unsigned>(std::string("BlendOperator")) {
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
	struct GLFace_ : x3::symbols<unsigned> {
		GLFace_(): x3::symbols<unsigned>(std::string("Face(Front or Back)")) {
			add("front", GL_FRONT)
				("back", GL_BACK)
				("frontandback", GL_FRONT_AND_BACK);
		}
	};
	struct GLFacedir_ : x3::symbols<unsigned> {
		GLFacedir_(): x3::symbols<unsigned>(std::string("FaceDir")) {
			add("ccw", GL_CCW)
				("cw", GL_CW);
		}
	};
	struct GLColormask_ : x3::symbols<unsigned> {
		GLColormask_(): x3::symbols<unsigned>(std::string("ColorMask")) {
			add("r", 0x08)
				("g", 0x04)
				("b", 0x02)
				("a", 0x01);
		}
	};
	struct GLShadertype_ : x3::symbols<unsigned> {
		GLShadertype_(): x3::symbols<unsigned>(std::string("ShaderType")) {
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
		using ValueT = boost::variant<boost::blank, unsigned int,float,bool>;

		unsigned				type;
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
		mutable std::string		info_str;	//!< シェーダー文字列を1つに纏めた物

		const std::string& getShaderString() const;
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

	GLXStruct ParseGlx(std::string str);
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
FUSION_ADAPT_STRUCT_AUTO(rs::GLXStruct, (atM)(csM)(shM)(tpL)(uniM)(varM))
FUSION_ADAPT_STRUCT_AUTO(rs::ArgItem, (type)(name))
FUSION_ADAPT_STRUCT_AUTO(rs::BlockUse, (type)(bAdd)(name))
