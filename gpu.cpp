#include "gpu.hpp"
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

namespace rs {
	namespace {
		//! Regex: OpenGL version
		/*!	[1,2,3] = OpenGL version
			[4] = Profile
			[5,6,7] = Driver version */
		boost::regex re_version("(\\d+)\\.(\\d+)(?:\\.(\\d+))?\\s+([\\w ]+)\\s+([^\\.]+)?\\.?([^\\.]+)?\\.?([^\\.]+)?");
		//! Regex: GLSL Version
		/*!	[1,2] = GLSL version (major, minor) */
		boost::regex re_glsl("(\\d+)\\.(\\d+)");
		//! Regex: Extensions...
		/*!	[1] = GL-Extension */
		boost::regex re_ext("[\\w\\d_]+");
	}
	// ---------------------- GPUInfo ----------------------
	void GPUInfo::Version::clear() {
		for(auto& a : ar)
			a = 0;
	}
	void GPUInfo::onDeviceReset() {
		_strVendor = reinterpret_cast<const char*>(GL.glGetString(GL_VENDOR));
		_strRenderer = reinterpret_cast<const char*>(GL.glGetString(GL_RENDERER));

		// プロファイルの特定
		boost::cmatch cm;
		if(boost::regex_search(reinterpret_cast<const char*>(GL.glGetString(GL_VERSION)), cm, re_version)) {
			auto prof = cm.str(4);
			std::transform(prof.cbegin(), prof.cend(), prof.begin(), ::tolower);
			if(prof == std::string("compatibility profile context"))
				_profile = Profile::Compatibility;
			else
				_profile = Profile::Core;

			for(int i=0 ; i<3 ; i++)
				_verGL.ar[i] = boost::lexical_cast<int>(cm.str(i+1));

			for(int i=0 ; i<std::min(static_cast<int>(cm.size()-5),3) ; i++) {
				auto s = cm.str(i+5);
				if(!s.empty())
					_verDriver.ar[i] = boost::lexical_cast<int>(s);
			}
		} else
			_verGL.clear();
		if(boost::regex_search(reinterpret_cast<const char*>(GL.glGetString(GL_SHADING_LANGUAGE_VERSION)), cm, re_glsl)) {
			for(int i=0 ; i<2 ; i++)
				_verSL.ar[i] = boost::lexical_cast<int>(cm.str(i+1));
			_verSL.ar[2] = 0;
		} else
			_verSL.clear();

		auto* cp = reinterpret_cast<const char*>(GL.glGetString(GL_EXTENSIONS));
		while(boost::regex_search(cp, cm, re_ext)) {
			_capSet.insert(cm.str(0));
			cp = cm.suffix().first;
		}
	}
	void GPUInfo::onDeviceLost() {}

	const GPUInfo::Version& GPUInfo::glslVersion() const {
		return _verSL;
	}
	const GPUInfo::Version& GPUInfo::version() const {
		return _verGL;
	}
	const GPUInfo::Version& GPUInfo::driverVersion() const {
		return _verDriver;
	}
	const std::string& GPUInfo::vendor() const {
		return _strVendor;
	}
	const std::string& GPUInfo::renderer() const {
		return _strRenderer;
	}
	const GPUInfo::CapSet& GPUInfo::refCapabilitySet() const {
		return _capSet;
	}
	std::ostream& operator << (std::ostream& os, const GPUInfo::Version& ver) {
		return os << ver.major << '.' << ver.minor << '-' << ver.revision;
	}
	std::ostream& operator << (std::ostream& os, const GPUInfo& info) {
		using std::endl;
		os << "OpenGL Version: " << info.version() << endl
				<< "OpenGL Vendor: " << info.vendor() << endl
				<< "OpenGL Renderer: " << info.renderer() << endl
				<< "GLSL Version: " << info.glslVersion() << endl
				<< "Driver Version: " << info.driverVersion() << endl
				<< "Extensions: " <<  endl;
		auto& cs = info.refCapabilitySet();
		for(auto& c : cs)
			os << c << endl;
		return os;
	}
#ifndef USE_OPENGLES2
	// ----------------------- GPUTime -----------------------
	GPUTime::GPUTime(): _idQuery{0,0}, _idSync{nullptr,nullptr}, _cursor(0), _prevTime(0) {}
	GPUTime::~GPUTime() {
		onDeviceLost();
	}
	void GPUTime::onFrameBegin() {
		GL.glBeginQuery(GL_TIME_ELAPSED, _idQuery[_cursor]);
	}
	void GPUTime::onFrameEnd() {
		GL.glEndQuery(GL_TIME_ELAPSED);
		_idSync[_cursor] = GL.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		GL.glWaitSync(_idSync[_cursor], 0, GL_TIMEOUT_IGNORED);
		_cursor ^= 1;

		GLuint q = _idQuery[_cursor];
		GLsync sync = _idSync[_cursor];
		if(sync != nullptr) {
			GL.glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
			GLuint bEnd = GL_FALSE;
			do {
				GL.glGetQueryObjectuiv(q, GL_QUERY_RESULT_AVAILABLE, &bEnd);
			} while(bEnd == GL_FALSE);
			GL.glGetQueryObjectui64v(q, GL_QUERY_RESULT, &_prevTime);
			GL.glDeleteSync(sync);
			_idSync[_cursor] = nullptr;
		}
	}
	GLuint64 GPUTime::getTime() const {
		return _prevTime;
	}
	void GPUTime::onDeviceReset() {
		if(_idQuery[0] == 0)
			GL.glGenQueries(countof(_idQuery), _idQuery);
	}
	void GPUTime::onDeviceLost() {
		if(_idQuery[0] != 0) {
			for(auto& sync : _idSync) {
				if(sync)
					GL.glDeleteSync(sync);
			}
			GL.glDeleteQueries(countof(_idQuery), _idQuery);

			_idQuery[0] = _idQuery[1] = 0;
			_idSync[0] = _idSync[1] = nullptr;
			_cursor = 0;
			_prevTime = 0;
		}
	}
#endif
}
