#pragma once
#include "glresource.hpp"
#include <unordered_set>

namespace rs {
	// OpenGLES2では同期オブジェクトが無いのでGPUTimeは無効
#ifndef USE_OPENGLES2
	//! フレームレート表示
	/*! 毎フレームテキストハンドルを作り直す */
	class GPUTime : public IGLResource {
		GLuint		_idQuery[2];
		GLsync		_idSync[2];
		int			_cursor;
		GLuint64	_prevTime;
		public:
			GPUTime();
			~GPUTime();
			void onFrameBegin();
			void onFrameEnd();
			GLuint64 getTime() const;

			void onDeviceLost() override;
			void onDeviceReset() override;
	};
#endif
	//! GPUの情報表示
	class GPUInfo : public IGLResource {
		struct Version {
			union {
				struct {
					int major,
						minor,
						revision;
				};
				int ar[3];
			};
			void clear();
		};
		friend std::ostream& operator << (std::ostream& os, const GPUInfo::Version& ver);
		enum class Profile {
			Core,
			Compatibility
		};
		using CapSet = std::unordered_set<std::string>;
		CapSet		_capSet;
		Version		_verGL,		//!< OpenGL version
					_verSL,		//!< GLSL version
					_verDriver;
		std::string	_strVendor,
					_strRenderer;
		Profile		_profile;

		public:
			const Version& glslVersion() const;
			const Version& version() const;
			const Version& driverVersion() const;
			const std::string& vendor() const;
			const std::string& renderer() const;
			const CapSet& refCapabilitySet() const;

			void onDeviceLost() override;
			void onDeviceReset() override;
			friend std::ostream& operator << (std::ostream& os, const GPUInfo& info);
	};
	std::ostream& operator << (std::ostream& os, const GPUInfo::Version& ver);
	std::ostream& operator << (std::ostream& os, const GPUInfo& info);
}
