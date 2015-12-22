#pragma once
#include "spinner/rflag.hpp"
#include "../glx_id.hpp"
#include "../handle.hpp"
#include "../luaimport.hpp"

namespace myunif {
	extern const rs::IdValue	U_Position,		// "u_lightPos"
								U_Color,		// "u_lightColor"
								U_Dir,			// "u_lightDir"
								U_Mat,			// "u_lightMat"
								U_Coeff,		// "u_lightCoeff"
								U_LightIVP,		// "u_lightViewProj"
								U_DepthRange,	// "u_depthRange"
								U_ScrLightPos,	// "u_scrLightPos"
								U_ScrLightDir;	// "u_scrLightDir"
}
class DLight {
	private:
		struct Camera;
		struct GetterC : spn::RFlag_Getter<uint32_t> {
			using RFlag_Getter::operator ();
			counter_t operator()(const rs::HLCamF&, Camera*, const DLight&) const;
		};
		struct ScLit_t {
			spn::Vec3	pos,
						dir;
		};
		using ScLit_a = spn::AcCheck<ScLit_t, GetterC>;
		using IVP_t = spn::AcCheck<spn::Mat44, GetterC>;
		#define SEQ_LIGHT \
			((DepthRange)(spn::Vec2)) \
			((Color)(spn::Vec3)) \
			((Coeff)(spn::Vec2)) \
			((Direction)(spn::Vec3)) \
			((Position)(spn::Vec3)) \
			((Camera)(rs::HLCamF)(Position)(Direction)(DepthRange)) \
			((Matrix)(spn::Mat44)(Camera)) \
			((EyeCamera)(rs::HLCamF)) \
			((ScLit)(ScLit_a)(Position)(Direction)(EyeCamera)) \
			((IVP)(IVP_t)(EyeCamera)(Matrix))
		RFLAG_S(DLight, SEQ_LIGHT)
	public:
		RFLAG_SETMETHOD_S(SEQ_LIGHT)
		RFLAG_REFMETHOD_S(SEQ_LIGHT)
		RFLAG_GETMETHOD_S(SEQ_LIGHT)
		#undef SEQ_UNIF
		DLight();
		void prepareUniforms(rs::IEffect& e) const;
};
DEF_LUAIMPORT(DLight)
