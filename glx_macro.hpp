#pragma once
#include <boost/preprocessor.hpp>

#define SEQ_VSEM BOOST_PP_REPEAT(NUM_TEXCOORD, PPFUNC_ADDNUM, TEXCOORD)(POSITION)(COLOR)(NORMAL)(BINORMAL)(TANGENT)
#define SEQ_GLTYPE (void)(bool)(int)(float)(vec2)(vec3)(vec4)(ivec2)(ivec3)(ivec4)(bvec2)(bvec3)(bvec4)(mat2)(mat3)(mat4)(sampler2D)(samplerCube)
#define SEQ_PRECISION (highp)(mediump)(lowp)
#define SEQ_INOUT (in)(out)(inout)
#define SEQ_BLOCK (attribute)(varying)(uniform)(const)

#ifdef USE_OPENGLES2
	#define GLDEPTHRANGE glDepthRangef
#else
	#define GLDEPTHRANGE glDepthRange
#endif
#define SEQ_GLSETTING ((linewidth,glLineWidth,float))((frontface,glFrontFace,unsigned))((cullfacedir,glCullFace,unsigned))((depthrange,GLDEPTHRANGE,float,float))((viewport,glViewport,float,float,float,float))\
		((scissor,glScissor,float,float,float,float))((samplecoverage,glSampleCoverage,float,bool))((stencilfunc,glStencilFunc,unsigned,float,unsigned))((stencilfuncfront,stencilFuncFront,unsigned,float,unsigned))\
		((stencilfuncback,stencilFuncBack,unsigned,float,unsigned))((stencilop,glStencilOp,unsigned,unsigned,unsigned))((stencilopfront,stencilOpFront,unsigned,unsigned,unsigned))((stencilopback,stencilOpBack,unsigned,unsigned,unsigned))\
		((depthfunc,glDepthFunc,unsigned))((blendeq,glBlendEquation,unsigned))((blendeqca,glBlendEquationSeparate,unsigned,unsigned))((blendfunc,glBlendFunc,unsigned,unsigned))\
		((blendfuncca,glBlendFuncSeparate,unsigned,unsigned,unsigned,unsigned))((blendcolor,glBlendColor,float,float,float,float))\
		((colormask,glColorMask,bool,bool,bool,bool))((depthmask,glDepthMask,bool))\
		((stencilmask,glStencilMask,unsigned))((stencilmaskfront,stencilMaskFront,unsigned))((stencilmaskback,stencilMaskBack,unsigned))\
		((polygonmode,polygonMode,unsigned))

#define NUM_TEXCOORD 8
#define PPFUNC_ADDNUM(z,n,data) (BOOST_PP_CAT(data,n))
#define PPFUNC_STR(ign, data, elem) BOOST_PP_STRINGIZE(elem),
#define PPFUNC_ADD(ign, data, elem) (BOOST_PP_STRINGIZE(elem), BOOST_PP_CAT(elem, data))
#define PPFUNC_ENUM(ign, data, elem) BOOST_PP_CAT(elem, data),
#define EMPTY

#define PPFUNC_GLSET_ENUM(ign,data,elem) BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(0,elem), data),
#define PPFUNC_GLSET_ADD(ign,data,elem) (BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0,elem)), BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(0,elem),data))

// tupleのn番要素以降を列挙
#define ENUMTUPLE_FUNC(z,n,data) (BOOST_PP_TUPLE_ELEM(n,data)())
#define ENUMTUPLE(n,tup) BOOST_PP_SEQ_ENUM(BOOST_PP_REPEAT_FROM_TO(n, BOOST_PP_TUPLE_SIZE(tup), ENUMTUPLE_FUNC, tup))
#define CONCAT_SCOPE(a,b)	a::b
#define PPFUNC_GLSET_FUNC(ign,data,elem) [](const ValueSettingR& vs) { vs.action(CONCAT_SCOPE(&IGL, BOOST_PP_TUPLE_ELEM(1,elem)), ENUMTUPLE(2,elem)); },
#define PPFUNC_GLSET_NARG(ign,data,elem) BOOST_PP_SUB(BOOST_PP_TUPLE_SIZE(elem), 2),
#define PPFUNC_GLSET_NAME(ign,data,elem) BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0,elem)),
