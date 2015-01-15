#include "updater.hpp"
#include "spinner/sort.hpp"

namespace rs {
	void DSort::DoSort(const DSortV& alg, int cursor, typename DLObjV::iterator itr0, typename DLObjV::iterator itr1) {
		if(cursor == int(alg.size()))
			return;
		auto* pAlg = alg[cursor].get();
		auto fnCmp = [pAlg](const DLObjP& p0, const DLObjP& p1){
			return pAlg->compare(*p0.first, *p1.first);
		};
		spn::insertion_sort(itr0, itr1, fnCmp);
		if(++cursor < int(alg.size())) {
			// 対象となる値が等しい範囲で別の基準にてソート
			auto itr = std::next(itr0);
			while(itr != itr1) {
				if(fnCmp(*itr0, *itr)) {
					DoSort(alg, cursor, itr0, itr);
					itr0 = itr;
				}
				++itr;
			}
		}
	}
	bool DSort_Z_Asc::compare(const DrawTag& d0, const DrawTag& d1) const {
		return d0.zOffset < d1.zOffset;
	}
	bool DSort_Z_Desc::compare(const DrawTag& d0, const DrawTag& d1) const {
		return d0.zOffset > d1.zOffset;
	}
	bool DSort_TechPass::compare(const DrawTag& d0, const DrawTag& d1) const {
		return d0.idTechPass < d1.idTechPass;
	}
	bool DSort_Texture::compare(const DrawTag& d0, const DrawTag& d1) const {
		return d0.idTex < d1.idTex;
	}
	bool DSort_Buffer::compare(const DrawTag& d0, const DrawTag& d1) const {
		return d0.idBuffer < d1.idBuffer;
	}
}

