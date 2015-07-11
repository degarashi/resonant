#include "updater.hpp"
#include "spinner/sort.hpp"
#include "glx.hpp"

namespace rs {
	void DSort::apply(const DrawTag& d, GLEffect& glx) {}
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

	// ------------------- DSort_Z_Asc -------------------
	const float DSort_Z_Asc::cs_border(-1e8f);
	bool DSort_Z_Asc::hasInfo(const DrawTag& d) const {
		return d.zOffset > cs_border;
	}
	bool DSort_Z_Asc::compare(const DrawTag& d0, const DrawTag& d1) const {
		return d0.zOffset < d1.zOffset;
	}
	// ------------------- DSort_Z_Desc -------------------
	bool DSort_Z_Desc::compare(const DrawTag& d0, const DrawTag& d1) const {
		return d0.zOffset > d1.zOffset;
	}
	// ------------------- DSort_Priority_Asc -------------------
	bool DSort_Priority_Asc::hasInfo(const DrawTag& d) const {
		return true;
	}
	bool DSort_Priority_Asc::compare(const DrawTag& d0, const DrawTag& d1) const {
		return d0.priority < d1.priority;
	}
	// ------------------- DSort_Priority_Desc -------------------
	bool DSort_Priority_Desc::compare(const DrawTag& d0, const DrawTag& d1) const {
		return d0.priority > d1.priority;
	}
	// ------------------- DSort_TechPass -------------------
	const uint32_t DSort_TechPass::cs_invalidValue(~0);
	bool DSort_TechPass::hasInfo(const DrawTag& d) const {
		return d.idTechPass.value != cs_invalidValue;
	}
	bool DSort_TechPass::compare(const DrawTag& d0, const DrawTag& d1) const {
		return d0.idTechPass.value < d1.idTechPass.value;
	}
	void DSort_TechPass::apply(const DrawTag& d, GLEffect& glx) {
		if(hasInfo(d)) {
			if(d.idTechPass.bId16) {
				glx.setTechnique(d.idTechPass.tpId[0], true);
				glx.setPass(d.idTechPass.tpId[1]);
			} else {
				glx.setTechPassId(d.idTechPass.preId);
			}
		}
	}

	namespace detail {
		void DSort_UniformPairBase::_refreshUniformId(GLEffect& glx, const std::string* name, GLint* id, size_t length) {
			if(_pFx != &glx) {
				_pFx = &glx;
				for(int i=0 ; i<length ; i++) {
					auto& s = name[i];
					id[i] = (!s.empty()) ? *glx.getUniformID(s) : -1;
				}
			}
		}
	}
	// ------------------- DSort_Texture -------------------
	bool DSort_Texture::hasInfo(const DrawTag& d) const {
		return d.idTex[0].valid();
	}
	bool DSort_Texture::compare(const DrawTag& d0, const DrawTag& d1) const {
		return d0.idTex < d1.idTex;
	}
	void DSort_Texture::apply(const DrawTag& d, GLEffect& glx) {
		auto& id = _getUniformId(glx);
		for(int i=0 ; i<length ; i++) {
			if(id[i] >= 0)
				glx.setUniform(id[i], d.idTex[i]);
		}
	}

	// ------------------- DSort_Buffer -------------------
	bool DSort_Buffer::hasInfo(const DrawTag& d) const {
		return d.idVBuffer[0].valid() || d.idIBuffer.valid();
	}
	bool DSort_Buffer::compare(const DrawTag& d0, const DrawTag& d1) const {
		auto val0 = d0.idIBuffer.getValue(),
			 val1 = d1.idIBuffer.getValue();
		if(val0 < val1)
			return true;
		if(val0 > val1)
			return false;
		return d0.idVBuffer < d1.idVBuffer;
	}
	void DSort_Buffer::apply(const DrawTag& d, GLEffect& glx) {
		for(int i=0 ; i<length ; i++) {
			auto& hdl = d.idVBuffer[i];
			if(hdl)
				glx.setVStream(hdl, i);
		}
		glx.setIStream(d.idIBuffer);
	}
}
