#include "unittest/test.hpp"
#include "glx.hpp"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace {
	#include "include/glformat_type.inc"
}
namespace rs {
	namespace test {
		class VDeclTest : public spn::test::RandomTestInitializer {};
		TEST_F(VDeclTest, Serialization) {
			auto rd = VDeclTest::getRand();
			auto makeIntRandom = [&rd](int from, int to, int mult=1) {
				return [&rd,from,to,mult](){ return rd.template getUniformRange<int>(from, to) * mult; };
			};
			auto rdNStream = makeIntRandom(1,VData::MAX_STREAM);
			auto rdNValue = makeIntRandom(0,8);
			auto rdOfs = makeIntRandom(0,3,4);
			auto rdBool = makeIntRandom(0,1);
			auto rdElemType = makeIntRandom(0, countof(c_GLTypeList));
			auto rdSemID = makeIntRandom(0, static_cast<int>(VSem::NUM_SEMANTIC)-1);
			auto rdElemSize = makeIntRandom(1,4);

			// ランダムなVDInfoVを作ってシリアライズ前後のデータを比較
			// random stream length
			VDecl::VDInfoV vd;
			int nStream = rdNStream();
			for(int i=0 ; i<nStream ; i++) {
				int nValue = rdNValue(),
					ofs = 0;
				for(int j=0 ; j<nValue ; j++) {
					auto& type = c_GLTypeList[rdElemType()];
					VDecl::VDInfo vdInfo {
						static_cast<GLenum>(i),
						static_cast<GLenum>(ofs),			// random offset
						static_cast<GLenum>(type.first),	// random element type
						static_cast<GLenum>(rdBool()),		// normalize or not
						static_cast<GLenum>(rdElemSize()),	// random element size
						static_cast<GLenum>(rdSemID())		// random semantics ID
					};
					ofs += type.second*vdInfo.elemSize + rdOfs();
					vd.push_back(vdInfo);
				}
			}
			VDecl vdecl(vd);
			spn::test::CheckSerializedDataText(vdecl);
		}
	}
}

