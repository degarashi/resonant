#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include <gtest/gtest.h>

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

