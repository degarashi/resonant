#include "glx_parse.hpp"

namespace rs {
	void GR_Glx::_initRule1() {
		using boost::phoenix::at_c;
		using boost::phoenix::push_back;
		using boost::phoenix::val;
		using boost::phoenix::construct;
		using boost::phoenix::insert;

		// Arg: GLType NameToken
		rlArg %= GLType > rlNameToken;
		// ShBlock: GLShadertype NameToken \(Arg (, Arg)*\) \{[^\}]*\}
		rlShBlock = qi::no_case[GLShadertype][at_c<0>(_val)=_1] > rlNameToken[at_c<1>(_val)=_1] > '(' >
						-(rlArg[push_back(at_c<2>(_val),_1)] > *(',' > rlArg[push_back(at_c<2>(_val),_1)])) > ')' >
						lit('{') > (qi::lexeme[qi::as_string[*qi::lexeme[standard::char_ - '}']]])[at_c<3>(_val)=_1] > '}';
		// MacroBlock: macro \{ MacroEnt \}
		rlMacroBlock %= qi::no_case[lit("macro")] > '{' > *rlMacroEnt > '}';
		// PassBlock: pass \{ (BlockUse | BoolSet | MacroBlock | ShSet | ValueSet)* \}
		rlPassBlock = lit("pass") > rlNameToken[at_c<0>(_val)=_1] > '{' >
				*(rlBlockUse[push_back(at_c<1>(_val),_1)] | rlBoolSet[push_back(at_c<2>(_val),_1)] |
				rlMacroBlock[at_c<3>(_val)=_1] | rlShSet[push_back(at_c<4>(_val), _1)] |
				rlValueSet[push_back(at_c<6>(_val),_1)]) > '}';
		// TechBlock: technique (: NameToken (, NameToken)*) \{ (PassBlock | BlockUse | BoolSet | MacroBlock | ShSet | ValueSet)* \}
		rlTechBlock = lit("technique") > rlNameToken[at_c<0>(_val)=_1] > -(':' > (rlNameToken % ',')[at_c<7>(_val)=_1]) > '{' >
				*(rlPassBlock[push_back(at_c<5>(_val),_1)] | rlBlockUse[push_back(at_c<1>(_val),_1)] | rlBoolSet[push_back(at_c<2>(_val),_1)] |
				rlMacroBlock[at_c<3>(_val)=_1] | rlShSet[push_back(at_c<4>(_val), _1)] |
				rlValueSet[push_back(at_c<6>(_val),_1)]) > '}';
		// GLX: (Comment | (AttrBlock | ConstBlock | ShBlock | TechBlock | UnifBlock | VaryBlock))*
		rlGLX = *(rlComment | (rlAttrBlock[insert(at_c<0>(_val), construct<std::pair<std::string,AttrStruct>>(at_c<0>(_1), _1))] |
					rlConstBlock[insert(at_c<1>(_val), construct<std::pair<std::string,ConstStruct>>(at_c<0>(_1), _1))] |
					rlShBlock[insert(at_c<2>(_val), construct<std::pair<std::string,ShStruct>>(at_c<1>(_1), _1))] |
					rlTechBlock[push_back(at_c<3>(_val), _1)] |
					rlUnifBlock[insert(at_c<4>(_val), construct<std::pair<std::string,UnifStruct>>(at_c<0>(_1), _1))] |
					rlVaryBlock[insert(at_c<5>(_val), construct<std::pair<std::string,VaryStruct>>(at_c<0>(_1), _1))]));
		// CommentS: //.*$
		rlCommentS = lit("//") > qi::lexeme[*(standard::char_ - qi::eol - qi::eoi) > (qi::eol | qi::eoi)];
		// CommentL: /\*.*\*/
		rlCommentL = lit("/*") > qi::lexeme[*(standard::char_ - qi::lit("*/")) > qi::lit("*/")];
		// Comment: CommentS | CommentL
		rlComment = rlCommentS | rlCommentL;
	}
}
