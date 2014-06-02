#include "glx_parse.hpp"
#define DEF_ERR(r, msg) r.name(msg); qi::on_error<qi::fail>(r, err);

namespace rs {
	void GR_Glx::_initRule0() {
		using boost::phoenix::at_c;
		using boost::phoenix::push_back;
		using boost::phoenix::val;
		using boost::phoenix::construct;
		using boost::phoenix::insert;

		// String: "[^"]+"
		rlString %= lit('"') > +(standard::char_ - '"') > '"';
		// NameToken: [:Alnum:_]+;
		rlNameToken %= qi::lexeme[+(standard::alnum | standard::char_('_'))];
		// Bracket: (\.)[^\1]Bracket?[^\1](\1)
		rlBracket %= lit(_r1) > qi::as_string[qi::lexeme[*(standard::char_ - lit(_r1) - lit(_r2))]] > -(rlBracket(_r1,_r2)) > lit(_r2);

		// AttrEnt: GLPrecision? GLType NameToken : GLSem;
		rlAttrEnt = (-(GLPrecision[at_c<0>(_val)=_1]) >> GLType[at_c<1>(_val)=_1] >> rlNameToken[at_c<2>(_val)=_1] >> ':') > GLSem[at_c<3>(_val)=_1] > ';';
		// VaryEnt: GLPrecision? GLType NameToken;
		rlVaryEnt %= -(GLPrecision) >> GLType >> rlNameToken >> ';';

		// UnifEntry: Precision`GLPrecision` ValueName`rlNameToken` [SizeSem`rlNameToken`] = DefaultValue`rlVec|float|bool`
		rlUnifEnt = (-(GLPrecision[at_c<0>(_val)=_1]) >> GLType[at_c<1>(_val)=_1]) > rlNameToken[at_c<2>(_val)=_1] >
					-('[' > qi::int_[at_c<3>(_val)=_1] > ']') >
					-(lit('=') > (rlVec | qi::float_ | qi::bool_)[at_c<4>(_val)=_1]) > ';';
		// Vector: [Float+]
		rlVec %= '[' > +qi::float_ > ']';
		// MacroEntry: NameToken(=NameToken)?;
		rlMacroEnt %= rlNameToken > -('=' > rlNameToken) > ';';
		// ConstEntry: GLPrecision? GLType = NameToken (Vector|Float|Bool);
		rlConstEnt = (-(GLPrecision[at_c<0>(_val)=_1]) >> GLType[at_c<1>(_val)=_1] >> rlNameToken[at_c<2>(_val)=_1] >>
			lit('=')) > (rlVec | qi::float_ | qi::bool_)[at_c<3>(_val)=_1] > ';';
		// BoolSet: GLBoolsetting = Bool;
		rlBoolSet %= qi::no_case[GLBoolsetting] > '=' > qi::no_case[qi::bool_] > ';';
		// ValueSet: GLSetting = (0xUint|GLFunc|GLStencilop|GLEq|GLBlend|GLFace|GLFacedir|GLColormask){1,4} | Float | Bool;
		rlValueSet %= qi::no_case[GLSetting] > '=' >
			qi::repeat(1,4)[(lit("0x") > qi::uint_) |
			qi::no_case[GLFunc | GLStencilop | GLEq | GLBlend | GLFace | GLFacedir | GLColormask]
			| qi::float_ | qi::bool_] > ';';
		// BlockUse: GLBlocktype (= | +=) NameToken (, NameToken)*;
		rlBlockUse = qi::no_case[GLBlocktype][at_c<0>(_val)=_1] > (lit('=')[at_c<1>(_val)=val(false)] | lit("+=")[at_c<1>(_val)=val(true)]) > (rlNameToken % ',')[at_c<2>(_val)=_1] > ';';
		// ShSet: GLShadertype = NameToken \((Vector|Bool|Float)? (, Vector|Bool|Float)*\);
		rlShSet = qi::no_case[GLShadertype][at_c<0>(_val)=_1] > '=' > rlNameToken[at_c<1>(_val)=_1] > lit('(') >
			(-(rlVec|qi::bool_|qi::float_)[push_back(at_c<2>(_val), _1)] > *(lit(',') > (rlVec|qi::bool_|qi::float_)[push_back(at_c<2>(_val), _1)])) >
			lit(");");
		// AttrBlock: attribute NameToken (: NameToken)? \{(AttrEnt|Comment)\}
		rlAttrBlock = lit("attribute") > rlNameToken[at_c<0>(_val)=_1] > -(':' > (rlNameToken % ',')[at_c<1>(_val)=_1]) >
							'{' > *(rlAttrEnt[push_back(at_c<2>(_val), _1)] | rlComment) > '}';
		// VaryBlock: varying NameToken (: NameToken)? \{VaryEnt|Comment\}
		rlVaryBlock = lit("varying") > rlNameToken[at_c<0>(_val)=_1] > -(':' > (rlNameToken % ',')[at_c<1>(_val)=_1]) >
							'{' > *(rlVaryEnt[push_back(at_c<2>(_val), _1)] | rlComment) > '}';
		// UnifBlock: uniform NameToken (: NameToken) \{UnifEnt|Comment\}
		rlUnifBlock = lit("uniform") > rlNameToken[at_c<0>(_val)=_1] > -(':' > (rlNameToken % ',')[at_c<1>(_val)=_1]) >
							'{' > *(rlUnifEnt[push_back(at_c<2>(_val), _1)] | rlComment) > '}';
		// ConstBlock: const NameToken (: NameToken) \{ConstEnt|Comment\}
		rlConstBlock = lit("const") > rlNameToken[at_c<0>(_val)=_1] > -(':' > (rlNameToken % ',')[at_c<1>(_val)=_1]) >
							'{' > *(rlConstEnt[push_back(at_c<2>(_val), _1)] | rlComment) > '}';
	}

	GR_Glx::GR_Glx(): GR_Glx::base_type(rlGLX, "OpenGL_effect_parser") {
		_initRule0();
		_initRule1();

		using boost::phoenix::val;
		using boost::phoenix::construct;

		auto err = (std::cout << val("Error! Expectiong ")
			<< _4 << val(" here: \"")
			<< construct<std::string>(_3, _2)
			<< val("\"") << std::endl);

		DEF_ERR(rlBlockUse, "block-use_parser")
		DEF_ERR(rlConstEnt, "const_entry_parser")
		DEF_ERR(rlConstBlock, "const_block_parser")
		DEF_ERR(rlComment, "comment_parser")
		DEF_ERR(rlCommentL, "long_comment_parser")
		DEF_ERR(rlCommentS, "short_comment_parser")
		DEF_ERR(rlString, "quoted_string_parser")
		DEF_ERR(rlNameToken, "name_token_parser")
		DEF_ERR(rlBracket, "bracket_parser")
		DEF_ERR(rlAttrEnt, "attribute_entry_parser")
		DEF_ERR(rlVaryEnt, "varying_entry_parser")
		DEF_ERR(rlUnifEnt, "uniform_entry_parser")
		DEF_ERR(rlMacroEnt, "macro_entry_parser")
		DEF_ERR(rlVec, "vector_value_parser")
		DEF_ERR(rlBoolSet, "boolean_setting_parser")
		DEF_ERR(rlValueSet, "value_setting_parser")
		DEF_ERR(rlShSet, "shader_setting_parser")
		DEF_ERR(rlShBlock, "shader_definition_parser")
		DEF_ERR(rlAttrBlock, "attribute_block_parser")
		DEF_ERR(rlVaryBlock, "varying_block_parser")
		DEF_ERR(rlUnifBlock, "uniform_block_parser")
		DEF_ERR(rlMacroBlock, "macro_block_parser")
		DEF_ERR(rlPassBlock, "pass_block_parser")
		DEF_ERR(rlTechBlock, "technique_block_parser")
		DEF_ERR(rlGLX, "OpenGL_effect_parser")
	}
}
