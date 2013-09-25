#pragma once
#include <SDL.h>
#include <SDL_mixer.h>
#include <algorithm>
#include "spinner/misc.hpp"
#include "spinner/resmgr.hpp"
#include "spinner/pqueue.hpp"
#include "sdlwrap.hpp"

#define MIXW_base(bStop) mixw::CheckMixError(bStop, __LINE__);
#define MIXW_Check MIXW_base(true)
#define MIXW_Warn MIXW_base(false)
#ifdef DEBUG
#define MIXW_ACheck MIXW_Check
#define MIXW_AWarn MIXW_Warn
#else
#define MIXW_ACheck
#define MIXW_AWarn
#endif
#define MixEC(...) mixw::MixEC_base<mixw::MixChecker>(__VA_ARGS__)
#define MixECA(...) mixw::MixEC_base<mixw::MixCheckerA>(__VA_ARGS__)

namespace mixw {
	void CheckMixError(bool bAssert, int line=-1);

	struct MixChecker {
		~MixChecker();
	};
	struct MixCheckerA {
		~MixCheckerA();
	};
	template <class Chk, class Func, class... TsA>
	auto MixEC_base(const Func& func, TsA&&... ts) -> decltype(func(std::forward<TsA>(ts)...)) {
		Chk chk;
		return func(std::forward<TsA>(ts)...);
	}

	using EffectM = std::function<void (void*, Uint8*, int)>;
	using EffectSE = std::function<void (int, void*, Uint8*, int)>;
	using HookM = std::function<void ()>;
	using HookSE = std::function<void (int)>;

	using SPEffectM = std::shared_ptr<EffectM>;
	using SPEffectSE = std::shared_ptr<EffectSE>;
	using SPHookM = std::shared_ptr<HookM>;
	using SPHookSE = std::shared_ptr<HookSE>;

	enum class FadeState {
		None,
		FadingIn,
		FadingOut
	};
	class Music {
		public:
			enum class Type {
				Cmd,
				Wav,
				Mod,
				Mid,
				Ogg,
				Mp3
			};
		private:
			Mix_Music*	_music;
			Type		_type;
			bool		_bPlaying;
			FadeState	_fadeStatus;

		public:
			// MusicのAPIはファイルからの読み込みのみ対応
			Music(const std::string& path);
			~Music();

			void play(int nLoop);
			void fadeIn(int nLoop, int ms);
			void fadeIn(int nLoop, int ms, double pos);
			void pause();
			void fadeOut(int ms);
			void setVolume(int volume);
			void resume();
			void rewind();

			bool isPlaying() const;
			FadeState getType() const;
			void setPosition(double pos);
	};
	class MusicMgr : public spn::ResMgrN<Music, MusicMgr> {};
	DEF_HANDLE(MusicMgr, Msc, Music)

	class SEGroup {
		int 	_groupID;
		using Members = std::vector<int>;
		Members	_members;

		public:
			void clear() const;
			int getNChannel() const;
			int getAvailable() const;
			int getOldest() const;
			int getNewer() const;
			void fadeOut() const;
			void halt() const;
			void setVolume(int volume) const;
			void addChannel(int id);
			void addChannels(int from, int to);
	};
	class SEGroupMgr : public spn::ResMgrA<SEGroup, SEGroupMgr> {};
	DEF_HANDLE(SEGroupMgr, Sg, SEGroup)

	class SEffect {
		HLSg	_hlGroup;
		int		_channel;
		public:
			void play(bool bDupl, int nLoop);
			void play(bool bDupl, int nLoop, int ticks);
			void fadeIn(bool bDupl, int nLoop, int ms);
			void fadeIn(bool bDupl, int nLoop, int ms, int ticks);
			// 重複して再生している場合は、最後に再生したチャンネルだけの操作になる
			void pause();
			void pauseAfter(int ms);
			void fadeOut(int ms);
			void resume();
			bool isPlaying() const;
			FadeState getFadeState() const;
			void setPosition(Sint16 angle, Uint8 dist);
	};
	class SEffectMgr : public spn::ResMgrN<SEffect, SEffectMgr> {};
	DEF_HANDLE(SEffectMgr, Se, SEffect)

	// ChannelとMusicを統合
	class SoundMgr : public spn::Singleton<SoundMgr> {
		MusicMgr	_mMgr;
		SEffectMgr	_seMgr;
		SEGroupMgr	_sgMgr;

		using GroupID = std::vector<int>;

		enum class Format {
			U8 = AUDIO_U8,
			S8 = AUDIO_S8,
			U16LE = AUDIO_U16LSB,
			U16BE = AUDIO_U16MSB,
			U16Sys = AUDIO_U16SYS,
			S16LE = AUDIO_S16LSB,
			S16BE = AUDIO_S16MSB,
			S16Sys = AUDIO_S16SYS
		};
		enum class Channel {
			Monoral = 1,
			Stereo = 2
		};
		constexpr static int MAX_VOLUME = MIX_MAX_VOLUME;

		void _printDecoders(std::ostream& os);

		static sdlw::Mutex	s_mutex;
		using HookML = std::vector<SPHookM>;
		using HookSL = std::vector<std::vector<SPHookSE>>;
		static HookML s_finishHookML;
		static HookSL s_finishHookSL;

		template <class T>
		struct FxPair {
			uint32_t 	priority;
			T			effector;

			FxPair(uint32_t prio, T&& t): priority(prio), effector(std::move(t)) {}
			bool operator == (const FxPair& fx) const {
				return effector == fx.effector;
			}
			bool operator != (const FxPair& fx) const {
				return !(this->operator == (fx));
			}
			bool operator < (const FxPair& fx) const {
				return priority < fx.priority;
			}
		};
		using FxM = FxPair<SPEffectM>;
		using FxSE = FxPair<SPEffectSE>;

		static void HookMusicFinished();
		static void HookSEFinished(int channel);
		static void HookMusic(void* udata, Uint8* stream, int len);
		static void HookSE(int channel, void* udata, Uint8* stream, int len);

		public:
			SoundMgr(int hz=MIX_DEFAULT_FREQUENCY, Format format=static_cast<Format>(MIX_DEFAULT_FORMAT),
					Channel channel=Channel::Stereo, int chunksize=4096, int nchunk=8);
			~SoundMgr();

			HLMsc loadMusic(const std::string& path);
			HLSe loadSE(const std::string& path);
			HLSg createSEGroup();

			void setMusicEffector(EffectM h);
			void setMusicFinishHook(HookM h);
			void clearMusicEffector();
			void addPostEffector(EffectM h);
			void remPostEffector(EffectM h);
			void clearPostEffector();

			void

		// setPosition(channel, angle, dist);
	};
}
