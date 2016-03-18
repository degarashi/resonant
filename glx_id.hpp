//! GLEffectで使うUniformIdやTech&PassId等, 定数の登録支援
#pragma once
#include <string>
#include <vector>
#include <boost/format.hpp>
#include "wrapper.hpp"
#include "spinner/error.hpp"

namespace rs {
	inline std::string ConvertToStr(const std::string& s) { return s; }
	template <class T>
	std::string ConvertToStr(const T& t) {
		return std::to_string(t);
	}
	template <class T0, class T1>
	std::string ConvertToStr(const std::pair<T0,T1>& t) {
		return ConvertToStr(t.first) + ":" + ConvertToStr(t.second);
	}
	using StrV = std::vector<std::string>;
	using StrPair = std::pair<std::string, std::string>;
	using StrPairV = std::vector<StrPair>;
	using IdValue = Wrapper<int>;
	template <class Key>
	class IdMgr {
		private:
			using EntryV = std::vector<Key>;
			EntryV _entry;

		public:
			IdValue genId(const Key& key) {
				auto itr = std::find(_entry.begin(), _entry.end(), key);
				AssertP(Warn, itr==_entry.end(), "Idの重複生成 %1%", ConvertToStr(key))
				if(itr == _entry.end()) {
					_entry.emplace_back(key);
					return IdValue(_entry.size()-1);
				}
				return IdValue(itr - _entry.begin());
			}
			const EntryV& getList() {
				return _entry;
			}
	};

	template <class Tag>
	class IdMgr_Glx {
		private:
			using UnifIdM = IdMgr<std::string>;
			using TechIdM = IdMgr<StrPair>;
			static UnifIdM		s_unifIdM;
			static TechIdM		s_techIdM;
		public:
			static IdValue GenUnifId(const std::string& name) {
				return s_unifIdM.genId(name);
			}
			static IdValue GenTechId(const std::string& tech, const std::string& pass) {
				return s_techIdM.genId({tech, pass});
			}
			static decltype(auto) GetUnifList() {
				return s_unifIdM.getList();
			}
			static decltype(auto) GetTechList() {
				return s_techIdM.getList();
			}
	};
	template <class Tag>
	typename IdMgr_Glx<Tag>::UnifIdM IdMgr_Glx<Tag>::s_unifIdM;
	template <class Tag>
	typename IdMgr_Glx<Tag>::TechIdM IdMgr_Glx<Tag>::s_techIdM;
}

