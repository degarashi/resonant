#include "glx_parse.hpp"
#include "glx_spirit_macro.hpp"

namespace rs {
	// GLX文法のEBNF記述
	namespace glx_rule {
		namespace x3 = boost::spirit::x3;
		const x3::rule<class Glx, GLXStruct>						Glx;
		const x3::rule<class String, std::string>					String;		//!< "で囲まれた文字列
		const x3::rule<class NameToken, std::string>				NameToken;	//!< 文字列トークン
		const x3::rule<class Bracket_t, std::string>				Bracket;	//!< 任意の括弧で囲まれたブロック

		// 変数宣言(attribute, uniform, varying)
		const x3::rule<class AttrEnt, AttrEntry> 					AttrEnt;
		const x3::rule<class VaryEnt, VaryEntry>					VaryEnt;
		const x3::rule<class UnifEnt, UnifEntry>					UnifEnt;
		const x3::rule<class ConstEnt, ConstEntry>					ConstEnt;
		const x3::rule<class MacroEnt, MacroEntry>					MacroEnt;

		const x3::rule<class BoolSet, BoolSetting>					BoolSet;
		const x3::rule<class ValueSet, ValueSetting>				ValueSet;
		const x3::rule<class BlockUse_t, BlockUse>					BlockUse;

		const x3::rule<class ShSet, ShSetting>						ShSet;

		// 各種定義ブロック
		const x3::rule<class AttrBlock, AttrStruct>					AttrBlock;
		const x3::rule<class VaryBlock, VaryStruct>					VaryBlock;
		const x3::rule<class UnifBlock, UnifStruct>					UnifBlock;
		const x3::rule<class ConstBlock, ConstStruct>				ConstBlock;
		const x3::rule<class ShBlock, ShStruct>						ShBlock;
		const x3::rule<class MacroBlock, std::vector<MacroEntry>>	MacroBlock;
		const x3::rule<class PassBlock, TPStruct>					PassBlock;
		const x3::rule<class TechBlock, TPStruct>					TechBlock;

		// 各種変数定義
		const x3::rule<class Vec, std::vector<float>>				Vec;
		const x3::rule<class Arg, ArgItem>							Arg;

		using x3::lit;
		using x3::char_;
		using x3::lexeme;
		using x3::no_case;
		// Arg: GLType NameToken
		const auto Arg_def = GLType > NameToken;
		// ShBlock: GLShadertype\([^\)]+\) NameToken \(Arg (, Arg)*\) \{[^\}]*\}
		DEF_SETVAL(fnSetType, type)
		DEF_SETVAL(fnSetVer, version_str)
		DEF_SETVAL(fnSetName, name)
		DEF_PUSHVAL(fnPushArg, args)
		DEF_SETVAL(fnSetInfo, info)
		const auto ShBlock_def = no_case[GLShadertype][fnSetType] >
						'(' > lexeme[*(char_ - lit(')'))][fnSetVer] > ')' >
						NameToken[fnSetName] > '(' >
						-(Arg[fnPushArg] > *(',' > Arg[fnPushArg])) > ')' >
						lit('{') > (lexeme[*lexeme[char_ - '}']])[fnSetInfo] > '}';
		// MacroBlock: macro \{ MacroEnt \}
		const auto MacroBlock_def = no_case[lit("macro")] > '{' > *MacroEnt > '}';
		// PassBlock: pass \{ (BlockUse | BoolSet | MacroBlock | ShSet | ValueSet)* \}
		DEF_PUSHVAL(fnPushBu, blkL)
		DEF_PUSHVAL(fnPushBs, bsL)
		DEF_SETVAL(fnSetMacro, mcL)
		DEF_PUSHVAL(fnPushSh, shL)
		DEF_PUSHVAL(fnPushTp, tpL)
		DEF_PUSHVAL(fnPushVs, vsL)
		const auto PassBlock_def = lit("pass") > NameToken[fnSetName] > '{' >
				*(BlockUse[fnPushBu] | ValueSet[fnPushVs] | BoolSet[fnPushBs] |
				MacroBlock[fnSetMacro] | ShSet[fnPushSh]) > '}';
		// TechBlock: technique (: NameToken (, NameToken)*) \{ (PassBlock | BlockUse | BoolSet | MacroBlock | ShSet | ValueSet)* \}
		DEF_SETVAL(fnSetDerive, derive)
		const auto TechBlock_def = lit("technique")
								> NameToken[fnSetName] > -(':' > (NameToken % ',')[fnSetDerive]) > '{' >
				*(PassBlock[fnPushTp] | BlockUse[fnPushBu] | ValueSet[fnPushVs] | BoolSet[fnPushBs] |
				MacroBlock[fnSetMacro] | ShSet[fnPushSh]) > '}';
		// GLX: (AttrBlock | ConstBlock | ShBlock | TechBlock | UnifBlock | VaryBlock)*
		DEF_INSVAL(fnInsAttr, atM, name)
		DEF_INSVAL(fnInsConst, csM, name)
		DEF_INSVAL(fnInsSh, shM, name)
		DEF_INSVAL(fnInsUnif, uniM, name)
		DEF_INSVAL(fnInsVary, varM, name)
		const auto Glx_def = *(AttrBlock[fnInsAttr] |
				ConstBlock[fnInsConst] |
				ShBlock[fnInsSh] |
				TechBlock[fnPushTp] |
				UnifBlock[fnInsUnif] |
				VaryBlock[fnInsVary]);

		using x3::alnum;
		using x3::int_;
		using x3::float_;
		using x3::bool_;
		using x3::uint_;
		using x3::repeat;
		// String: "[^"]+"
		const auto String_def = lit('"') > +(char_ - '"') > '"';
		// NameToken: [:Alnum:_]+;
		const auto NameToken_def = lexeme[+(alnum | char_('_'))];
		// Bracket: (\.)[^\1]Bracket?[^\1](\1)
		const auto Bracket_def = lit('{') > lexeme[*(char_ - lit('{') - lit('}'))] > -Bracket > lit('}');
		// AttrEnt: GLPrecision? GLType NameToken : GLSem;
		const auto AttrEnt_def = (-(GLPrecision) >> GLType >> NameToken >> ':') > GLSem > ';';
		// VaryEnt: GLPrecision? GLType NameToken;
		const auto VaryEnt_def = -(GLPrecision) >> GLType >> NameToken >> ';';
		// UnifEntry: Precision`GLPrecision` ValueName`rlNameToken` [SizeSem`rlNameToken`] = DefaultValue`rlVec|float|bool`
		const auto UnifEnt_def = (-(GLPrecision) >> GLType) > NameToken >
					-('[' > int_ > ']') >
					-(lit('=') > (Vec | float_ | bool_)) > ';';
		// Vector: [Float+]
		const auto Vec_def = '[' > +float_ > ']';
		// MacroEntry: NameToken(=NameToken)?;
		const auto MacroEnt_def = NameToken > -('=' > NameToken) > ';';
		// ConstEntry: GLPrecision? GLType = NameToken (Vector|Float|Bool);
		const auto ConstEnt_def = (-(GLPrecision) >> GLType >> NameToken >>
			lit('=')) > (Vec | float_ | bool_) > ';';

		// BoolSet: GLBoolsetting = Bool;
		const auto BoolSet_def = no_case[GLBoolsetting] > '=' > no_case[bool_] > ';';
		// ValueSet: GLSetting = (0xUint|GLFunc|GLStencilop|GLEq|GLBlend|GLFace|GLFacedir|GLColormask){1,4} | Float | Bool;
		const auto ValueSet_def = no_case[GLSetting] > '=' >
			repeat(1,4)[(lit("0x") > uint_) |
			no_case[GLFunc | GLStencilop | GLEq | GLBlend | GLFace | GLFacedir | GLColormask]
			| float_ | bool_] > ';';
		// BlockUse: GLBlocktype (= | +=) NameToken (, NameToken)*;
		DEF_SETVAL(Bu_fnSetName, name)
		const auto Bu_fnSetFalse = [](auto& ctx){ _val(ctx).bAdd = false; };
		const auto Bu_fnSetTrue = [](auto& ctx){ _val(ctx).bAdd = false; };
		const auto BlockUse_def = no_case[GLBlocktype][fnSetType]
									> (lit('=')[Bu_fnSetFalse] | lit("+=")[Bu_fnSetTrue])
									> (NameToken % ',')[Bu_fnSetName] > ';';
		// ShSet: GLShadertype = NameToken \((Vector|Bool|Float)? (, Vector|Bool|Float)*\);
		DEF_SETVAL(Sh_fnSetName, shName)
		DEF_PUSHVAL(Sh_fnPushArgs, args)
		const auto ShSet_def = no_case[GLShadertype][fnSetType]
								> '=' > NameToken[Sh_fnSetName]
								> lit('(')
								> (-(Vec|bool_|float_)[Sh_fnPushArgs]
										> *(lit(',') > (Vec|bool_|float_)[Sh_fnPushArgs]))
								> lit(");");

		DEF_PUSHVAL(fnPushEntry, entry)
		// AttrBlock: attribute NameToken (: NameToken)? \{(AttrEnt*)\}
		const auto AttrBlock_def = lit("attribute") > NameToken[fnSetName] > -(':' > (NameToken % ',')[fnSetDerive]) >
							'{' > *AttrEnt[fnPushEntry] > '}';
		// VaryBlock: varying NameToken (: NameToken)? \{VaryEnt*\}
		const auto VaryBlock_def = lit("varying") > NameToken[fnSetName] > -(':' > (NameToken % ',')[fnSetDerive]) >
							'{' > *VaryEnt[fnPushEntry] > '}';
		// UnifBlock: uniform NameToken (: NameToken)? \{UnifEnt*\}
		const auto UnifBlock_def = lit("uniform") > NameToken[fnSetName] > -(':' > (NameToken % ',')[fnSetDerive]) >
							'{' > *UnifEnt[fnPushEntry] > '}';
		// ConstBlock: const NameToken (: NameToken)? \{ConstEnt*\}
		const auto ConstBlock_def = lit("const") > NameToken[fnSetName] > -(':' > (NameToken % ',')[fnSetDerive]) >
							'{' > *ConstEnt[fnPushEntry] > '}';
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-parameter"
		BOOST_SPIRIT_DEFINE(Arg,
							ShBlock,
							MacroBlock,
							PassBlock,
							TechBlock,
							Glx);
		BOOST_SPIRIT_DEFINE(String,
							NameToken,
							Bracket,
							AttrEnt,
							VaryEnt,
							UnifEnt,
							Vec,
							MacroEnt,
							ConstEnt,
							BoolSet,
							ValueSet,
							BlockUse,
							ShSet,
							AttrBlock,
							VaryBlock,
							UnifBlock,
							ConstBlock);
		#pragma GCC diagnostic pop
	}
	namespace {
		boost::regex re_comment(R"(//[^\n$]+)"),		//!< 一行コメント
					re_comment2(R"(/\*[^\*]*\*/)");		//!< 範囲コメント
	}
	GLXStruct ParseGlx(std::string str) {
		// コメント部分を除去 -> スペースに置き換える
		str = boost::regex_replace(str, re_comment, " ");
		str = boost::regex_replace(str, re_comment2, " ");
		auto itr = str.cbegin();
		GLXStruct result;
		try {
			bool bS = x3::phrase_parse(itr, str.cend(), glx_rule::Glx, x3::standard::space, result);
			#ifdef DEBUG
				LogOutput((bS) ? "------- analysis succeeded! -------"
							: "------- analysis failed! -------");
				if(itr != str.cend()) {
					LogOutput("<but not reached to end>\nremains: %1%", std::string(itr, str.cend()));
				} else {
					// 解析結果の表示
					std::stringstream ss;
					ss << result;
					LogOutput(ss.str());
				}
			#endif
			if(!bS || itr!=str.cend()) {
				std::stringstream ss;
				ss << "GLEffect parse error:";
				if(itr != str.cend())
					ss << "remains:\n" << std::string(itr, str.cend());
				throw std::runtime_error(ss.str());
			}
		} catch(const x3::expectation_failure<std::string::const_iterator>& e) {
			LogOutput((boost::format("expectation_failure: \nat: %1%\nwhich: %2%\nwhat: %3%")
						% std::string(e.where(), str.cend())
						% e.which()
						% e.what()).str());
		}
		return result;
	}
}
