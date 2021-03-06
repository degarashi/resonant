#pragma once

#define DEF_REGMEMBER(n, clazz, elem, getter)	::rs::LuaImport::RegisterMember<getter,clazz>(lsc, BOOST_PP_STRINGIZE(elem), &clazz::elem);
#define DEF_REGMEMBER_HDL(n, data, elem)	DEF_REGMEMBER(n, BOOST_PP_SEQ_ELEM(1,data), elem, ::rs::LI_GetHandle<BOOST_PP_SEQ_ELEM(0,data)>)
#define DEF_REGMEMBER_PTR(n, clazz, elem)	DEF_REGMEMBER(n, clazz, elem, ::rs::LI_GetPtr<clazz>)

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
#include "spinner/check_macro.hpp"
namespace rs {
	DEF_HASMETHOD(LuaExport)
	class LuaState;
	template <class T>
	void CallLuaExport(LuaState& lsc, std::true_type) {
		T::LuaExport(lsc);
	}
	template <class T>
	void CallLuaExport(LuaState&, std::false_type) {}
}
#define DEF_LUAIMPLEMENT_DERIVED(clazz, der) \
	namespace rs{ namespace lua{ \
		template <> \
		const char* LuaName(clazz*) { return LuaName((der*)nullptr); } \
		template <> \
		void LuaExport(LuaState& lsc, clazz*) { LuaExport(lsc, (der*)nullptr); } \
	}}
#define DEF_LUAIMPLEMENT_HDL_IMPL(mgr, clazz, class_name, base, seq_member, seq_method, seq_ctor, makeobj) \
		namespace rs{ namespace lua{ \
			template <> \
			const char* LuaName(clazz*) { return #class_name; } \
			template <> \
			void LuaExport(LuaState& lsc, clazz*) { \
				LuaImport::BeginImportBlock(#class_name); \
				lsc.getGlobal(::rs::luaNS::DerivedHandle); \
				lsc.getGlobal(base); \
				lsc.push(#class_name); \
				lsc.prepareTableGlobal(#class_name); \
				lsc.call(3,1); \
				lsc.push(::rs::luaNS::objBase::_New); \
				lsc.push(makeobj<BOOST_PP_SEQ_ENUM((mgr)(clazz)seq_ctor)>); \
				lsc.rawSet(-3); \
				lsc.setField(-1, luaNS::objBase::_Pointer, false); \
				lsc.setField(-1, luaNS::objBase::ClassName, mgr::_ref().getResourceName(spn::SHandle())); \
				\
				LuaImport::BeginImportBlock("Values"); \
				lsc.rawGet(-1, ::rs::luaNS::objBase::ValueR); \
				lsc.rawGet(-2, ::rs::luaNS::objBase::ValueW); \
				BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER_HDL, (typename mgr::data_type)(clazz), seq_member) \
				lsc.pop(2); \
				LuaImport::EndImportBlock(); \
				\
				LuaImport::BeginImportBlock("Functions"); \
				lsc.rawGet(-1, ::rs::luaNS::objBase::Func); \
				BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER_HDL, (typename mgr::data_type)(clazz), seq_method) \
				lsc.pop(1); \
				LuaImport::EndImportBlock(); \
				\
				CallLuaExport<clazz>(lsc, rs::HasMethod_LuaExport_t<clazz>()); \
				lsc.pop(1); \
				LuaImport::EndImportBlock(); \
			} \
		}}
#define DEF_LUAIMPLEMENT_HDL(mgr, clazz, class_name, base, seq_member, seq_method, seq_ctor) \
	DEF_LUAIMPLEMENT_HDL_IMPL(mgr, clazz, class_name, base, seq_member, seq_method,  seq_ctor, ::rs::MakeHandle)
#define DEF_LUAIMPLEMENT_HDL_NOBASE(mgr, clazz, class_name, seq_member, seq_method, seq_ctor) \
	DEF_LUAIMPLEMENT_HDL(mgr, clazz, class_name, ::rs::luaNS::ObjectBase, seq_member, seq_method, seq_ctor)
#define DEF_LUAIMPLEMENT_HDL_NOCTOR(mgr, clazz, class_name, base, seq_member, seq_method) \
	DEF_LUAIMPLEMENT_HDL_IMPL(mgr, clazz, class_name, base, seq_member, seq_method, NOTHING, ::rs::MakeHandle_Fake)
#define DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(mgr, clazz, class_name, seq_member, seq_method) \
	DEF_LUAIMPLEMENT_HDL_IMPL(mgr, clazz, class_name, ::rs::luaNS::ObjectBase, seq_member, seq_method, NOTHING, ::rs::MakeHandle_Fake)

#define DEF_LUAIMPLEMENT_PTR_NOCTOR(clazz, class_name, seq_member, seq_method) \
	DEF_LUAIMPLEMENT_PTR_BASE(clazz, class_name, seq_member, seq_method, MakeHandle_Fake, NOTHING)
#define DEF_LUAIMPLEMENT_PTR(clazz, class_name, seq_member, seq_method, seq_ctor) \
	DEF_LUAIMPLEMENT_PTR_BASE(clazz, class_name, seq_member, seq_method, MakeObject, seq_ctor)
#define DEF_LUAIMPLEMENT_PTR_BASE(clazz, class_name, seq_member, seq_method, makeobj, seq_ctor) \
		namespace rs{ namespace lua{ \
			template <> \
			const char* LuaName(clazz*) { return #class_name; } \
			template <> \
			void LuaExport(LuaState& lsc, clazz*) { \
				LuaImport::BeginImportBlock(#class_name); \
				lsc.getGlobal(::rs::luaNS::DerivedHandle); \
				lsc.getGlobal(::rs::luaNS::ObjectBase); \
				lsc.push(#class_name); \
				lsc.prepareTableGlobal(#class_name); \
				lsc.call(3,1); \
				\
				lsc.push(::rs::luaNS::objBase::_New); \
				lsc.push(makeobj<BOOST_PP_SEQ_ENUM((clazz)seq_ctor)>); \
				lsc.rawSet(-3); \
				lsc.setField(-1, luaNS::objBase::_Pointer, true); \
				lsc.setField(-1, luaNS::objBase::ClassName, #class_name); \
				\
				LuaImport::BeginImportBlock("Values"); \
				lsc.rawGet(-1, ::rs::luaNS::objBase::ValueR); \
				lsc.rawGet(-2, ::rs::luaNS::objBase::ValueW); \
				BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER_PTR, clazz, seq_member) \
				lsc.pop(2); \
				LuaImport::EndImportBlock(); \
				\
				LuaImport::BeginImportBlock("Functions"); \
				lsc.rawGet(-1, ::rs::luaNS::objBase::Func); \
				BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER_PTR, clazz, seq_method) \
				lsc.pop(1); \
				LuaImport::EndImportBlock(); \
				\
				CallLuaExport<clazz>(lsc, rs::HasMethod_LuaExport_t<clazz>()); \
				lsc.pop(1); \
				LuaImport::EndImportBlock(); \
			} \
		}}

