#pragma once
#include "../scene.hpp"
#include "../spinner/random.hpp"

//! ベースシーンクラス
class Sc_Base : public rs::Scene<Sc_Base> {
	private:
		using RandomOP = spn::Optional<spn::MTRandom>;
		rs::HDObj		_hInfo;
		RandomOP		_random;
		struct St_Default;
		void initState() override;
	public:
		spn::MTRandom& getRand();
		void checkQuit();
};
//! キューブ描画テスト
class Sc_Cube : public rs::Scene<Sc_Cube> {
	private:
		Sc_Base&	_base;
		rs::HDObj	_hCube;
		struct St_Default;
		void initState() override;
	public:
		Sc_Cube(Sc_Base& b);
};
//! サウンド再生テスト
class Sc_Sound : public rs::Scene<Sc_Sound> {
	private:
		Sc_Base&	_base;
		rs::HLAbF	_hlAb;
		rs::HLSgF	_hlSg;
		struct St_Default;
		void initState() override;
	public:
		Sc_Sound(Sc_Base& b);
};
//! スプライトによる描画ソートテスト
class Sc_DSort : public rs::Scene<Sc_DSort> {
	private:
		Sc_Base&	_base;
		struct St_Init;
		struct St_Test;
		rs::HLDObj	_hlSprite[5];
		void initState() override;
	public:
		Sc_DSort(Sc_Base& b);
};
