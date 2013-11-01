#include "error.hpp"

LogOUT g_logOut = [](const std::string& str){
	#ifdef ANDROID
		LOGI(str.c_str());
	#else
		std::cout << str << std::endl;
	#endif
};
void LogOutput(const std::string& s) {
	g_logOut(s);
}
