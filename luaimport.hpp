#pragma once

#define DEF_REGMEMBER(n, clazz, elem, getter)	::rs::LuaImport::RegisterMember<getter,clazz>(lsc, BOOST_PP_STRINGIZE(elem), &clazz::elem);
#define DEF_REGMEMBER_HDL(n, data, elem)	DEF_REGMEMBER(n, BOOST_PP_SEQ_ELEM(1,data), elem, ::rs::LI_GetHandle<BOOST_PP_SEQ_ELEM(0,data)>)
#define DEF_REGMEMBER_PTR(...)				DEF_REGMEMBER(__VA_ARGS__, ::rs::LI_GetPtr)

#define DEF_LUAIMPORT_BASE namespace rs{ \
	class LuaState; \
	namespace lua{ \
		template <class T> \
		void LuaExport(LuaState& lsc, T*); \
		template <class T> \
		const char* LuaName(T*); \
	}}
#define DEF_LUAIMPORT(clazz) \
	DEF_LUAIMPORT_BASE \
	namespace rs { \
		namespace lua { \
		template <> \
		const char* LuaName(clazz*); \
		template <> \
		void LuaExport(LuaState& lsc, clazz*); \
	}}
#define DEF_LUAIMPLEMENT_HDL(mgr, clazz, seq_member, seq_method, seq_ctor) \
		namespace rs{ namespace lua{ \
			template <> \
			const char* LuaName(clazz*) { return #clazz; } \
			template <> \
			void LuaExport(LuaState& lsc, clazz*) { \
				using base_clazz = typename mgr::data_type; \
				\
				lsc.getGlobal(::rs::luaNS::DerivedHandle); \
				lsc.getGlobal(::rs::luaNS::ObjectBase); \
				lsc.call(1,1); \
				lsc.push(::rs::luaNS::objBase::_New); \
				lsc.push(::rs::MakeObj<BOOST_PP_SEQ_ENUM((clazz)seq_ctor)>); \
				lsc.setTable(-3); \
				\
				lsc.getField(-1, ::rs::luaNS::objBase::ValueR); \
				lsc.getField(-2, ::rs::luaNS::objBase::ValueW); \
				BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER_HDL, (base_clazz)(clazz), seq_member) \
				lsc.pop(2); \
				\
				lsc.getField(-1, ::rs::luaNS::objBase::Func); \
				BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER_HDL, (base_clazz)(clazz), seq_method) \
				lsc.pop(1); \
				\
				lsc.setGlobal(#clazz); \
			} \
		}}
#define DEF_LUAIMPLEMENT_PTR(clazz, seq_member, seq_method, seq_ctor) \
		namespace rs{ namespace lua{ \
			template <> \
			const char* LuaName(clazz*) { return #clazz; } \
			template <> \
			void LuaExport(LuaState& lsc, clazz*) { \
				lsc.getGlobal(::rs::luaNS::DerivedHandle); \
				lsc.getGlobal(::rs::luaNS::ObjectBase); \
				lsc.call(1,1); \
				\
				lsc.getField(-1, ::rs::luaNS::objBase::ValueR); \
				lsc.getField(-2, ::rs::luaNS::objBase::ValueW); \
				BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER_PTR, clazz, seq_member) \
				lsc.pop(2); \
				\
				lsc.getField(-1, ::rs::luaNS::objBase::Func); \
				BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER_PTR, clazz, seq_method) \
				lsc.pop(1); \
				\
				lsc.setGlobal(#clazz); \
			} \
		}}

