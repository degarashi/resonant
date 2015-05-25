#pragma once
#include "../scene.hpp"

//! ベースシーンクラス
class Sc_Base : public rs::Scene<Sc_Base> {
	private:
		rs::HDObj	_hInfo;
		struct St_Default;
		void initState() override;
		friend class Sc_DSort;
	public:
		void checkQuit();
};
//! キューブ描画テスト
class Sc_Cube : public Sc_Base {
	private:
		rs::HDObj	_hCube;
		struct St_Default;
		void initState() override;
};
//! サウンド再生テスト
class Sc_Sound : public Sc_Base {
	private:
		rs::HLAbF	_hlAb;
		rs::HLSgF	_hlSg;
		struct St_Default;
		void initState() override;
};
//! スプライトによる描画ソートテスト
class Sc_DSort : public Sc_Base {
	private:
		struct St_Init;
		struct St_Test;
		rs::HLDObj	_hlSprite[5];
		void initState() override;
};
