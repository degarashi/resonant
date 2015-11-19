#include "glx.hpp"

namespace rs {
	// ----------------- VDecl::VDInfo -----------------
	bool VDecl::VDInfo::operator == (const VDInfo& v) const {
		return ((streamID ^ v.streamID)
				| (offset ^ v.offset)
				| (elemFlag ^ v.elemFlag)
				| (bNormalize ^ v.bNormalize)
				| (elemSize ^ v.elemSize)
				| (semID ^ v.semID)) == 0;
	}
	bool VDecl::VDInfo::operator != (const VDInfo& v) const {
		return !(this->operator == (v));
	}
	// ----------------- VDecl -----------------
	VDecl::VDecl() {}
	VDecl::VDecl(const VDInfoV& vl) {
		_vdInfo = vl;
		_init();
	}
	VDecl::VDecl(std::initializer_list<VDInfo> il) {
		int n = il.size();
		_vdInfo.resize(n);
		// 一旦Vectorに変換
		auto itr = il.begin();
		for(int i=0 ; i<n ; i++)
			_vdInfo[i] = *itr++;
		_init();
	}
	void VDecl::_init() {
		// StreamID毎に集計
		std::vector<VDInfo> tmp[VData::MAX_STREAM];
		for(auto& v : _vdInfo)
			tmp[v.streamID].push_back(v);

		// 頂点定義のダブり確認
		for(auto& t : tmp) {
			// オフセットでソート
			std::sort(t.begin(), t.end(), [](const VDInfo& v0, const VDInfo& v1) { return v0.offset < v1.offset; });

			unsigned int ofs = 0;
			for(auto& t2 : t) {
				AssertT(Trap, ofs<=t2.offset, GLE_Error, "invalid vertex offset")
				ofs += GLFormat::QuerySize(t2.elemFlag) * t2.elemSize;
			}
		}

		_func.resize(_vdInfo.size());
		int cur = 0;
		for(int i=0 ; i<static_cast<int>(countof(tmp)) ; i++) {
			_nEnt[i] = cur;
			for(auto& t2 : tmp[i]) {
				_func[cur] = [t2](GLuint stride, const VData::AttrA& attr) {
					auto attrID = attr[t2.semID];
					if(attrID < 0)
						return;
					GL.glEnableVertexAttribArray(attrID);
					#ifndef USE_OPENGLES2
						auto typ = GLFormat::QueryGLSLInfo(t2.elemFlag)->type;
						if(typ == GLSLType::BoolT || typ == GLSLType::IntT) {
							// PCにおけるAMDドライバの場合、Int値はIPointerでセットしないと値が化けてしまう為
							GL.glVertexAttribIPointer(attrID, t2.elemSize, t2.elemFlag, stride, reinterpret_cast<const GLvoid*>(t2.offset));
						} else
					#endif
							GL.glVertexAttribPointer(attrID, t2.elemSize, t2.elemFlag, t2.bNormalize, stride, reinterpret_cast<const GLvoid*>(t2.offset));
				};
				++cur;
			}
		}
		_nEnt[VData::MAX_STREAM] = _nEnt[VData::MAX_STREAM-1];
	}
	void VDecl::apply(const VData& vdata) const {
		for(int i=0 ; i<VData::MAX_STREAM ; i++) {
			auto& ovb = vdata.buff[i];
			// VStreamが設定されていればBindする
			if(ovb) {
				auto& vb = *ovb;
				auto u = vb.use();
				GLuint stride = vb.getStride();
				for(int j=_nEnt[i] ; j<_nEnt[i+1] ; j++)
					_func[j](stride, vdata.attrID);
			}
		}
	}
	bool VDecl::operator == (const VDecl& vd) const {
		// VDInfoの比較
		return _vdInfo == vd._vdInfo;
	}
	bool VDecl::operator != (const VDecl& vd) const {
		return !(this->operator == (vd));
	}
}

