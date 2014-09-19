#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include "test.hpp"

int main(int argc, char **argv) {
	spn::MTRandomMgr rmgr;
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

