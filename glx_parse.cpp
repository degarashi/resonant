#include "glx_parse.hpp"
#define DEF_ERR(r, msg) r.name(msg); qi::on_error<qi::fail>(r, err);

namespace rs {
	void GR_Glx::_initRule0() {
		using boost::phoenix::at_c;
		using boost::phoenix::push_back;
		using boost::phoenix::val;
		using boost::phoenix::construct;
		using boost::phoenix::insert;

		rlString %= lit('"') > +(standard::char_ - '"') > '"';
		rlNameToken %= qi::lexeme[+(standard::alnum | standard::char_('_'))];
		rlBracket %= lit(_r1) > qi::as_string[qi::lexeme[*(standard::char_ - lit(_r1) - lit(_r2))]] > -(rlBracket(_r1,_r2)) > lit(_r2);

		rlAttrEnt = (-(GLPrecision[at_c<0>(_val)=_1]) >> GLType[at_c<1>(_val)=_1] >> rlNameToken[at_c<2>(_val)=_1] >> ':') > GLSem[at_c<3>(_val)=_1] > ';';
		rlVaryEnt %= -(GLPrecision) >> GLType >> rlNameToken >> ';';

		// (prec) valueType valueName<sizeSem> : defaultStr
		// valueName<sizeSem> = defaultValue
		rlUnifEnt = (-(GLPrecision[at_c<0>(_val)=_1]) >> GLType[at_c<1>(_val)=_1]) > rlNameToken[at_c<2>(_val)=_1] >
					-('<' > rlNameToken[at_c<3>(_val)=_1] > '>') >
					-(lit('=') > (rlVec | qi::float_ | qi::bool_)[at_c<4>(_val)=_1]) > ';';
		rlVec %= '[' > +qi::float_ > ']';
		rlMacroEnt %= rlNameToken > -('=' > rlNameToken) > ';';
		rlConstEnt = (-(GLPrecision[at_c<0>(_val)=_1]) >> GLType[at_c<1>(_val)=_1] >> rlNameToken[at_c<2>(_val)=_1] >>
			lit('=')) > (rlVec | qi::float_ | qi::bool_)[at_c<3>(_val)=_1] > ';';
		rlBoolSet %= qi::no_case[GLBoolsetting] > '=' > qi::no_case[qi::bool_] > ';';
		rlValueSet %= qi::no_case[GLSetting] > '=' >
			qi::repeat(1,4)[(lit("0x") > qi::uint_) |
			qi::no_case[GLFunc | GLStencilop | GLEq | GLBlend | GLFace | GLFacedir | GLColormask]
			| qi::float_ | qi::bool_] > ';';
		rlBlockUse = qi::no_case[GLBlocktype][at_c<0>(_val)=_1] > (lit('=')[at_c<1>(_val)=val(false)] | lit("+=")[at_c<1>(_val)=val(true)]) > (rlNameToken % ',')[at_c<2>(_val)=_1] > ';';
		rlShSet = qi::no_case[GLShadertype][at_c<0>(_val)=_1] > '=' > rlNameToken[at_c<1>(_val)=_1] > lit('(') >
			(-(rlVec|qi::bool_|qi::float_)[push_back(at_c<2>(_val), _1)] > *(lit(',') > (rlVec|qi::bool_|qi::float_)[push_back(at_c<2>(_val), _1)])) >
			lit(");");

		rlAttrBlock = lit("attribute") > rlNameToken[at_c<0>(_val)=_1] > -(':' > (rlNameToken % ',')[at_c<1>(_val)=_1]) >
							'{' > *(rlAttrEnt[push_back(at_c<2>(_val), _1)] | rlComment) > '}';
		rlVaryBlock = lit("varying") > rlNameToken[at_c<0>(_val)=_1] > -(':' > (rlNameToken % ',')[at_c<1>(_val)=_1]) >
							'{' > *(rlVaryEnt[push_back(at_c<2>(_val), _1)] | rlComment) > '}';
		rlUnifBlock = lit("uniform") > rlNameToken[at_c<0>(_val)=_1] > -(':' > (rlNameToken % ',')[at_c<1>(_val)=_1]) >
							'{' > *(rlUnifEnt[push_back(at_c<2>(_val), _1)] | rlComment) > '}';
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
