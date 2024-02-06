#include "dock_controlflow.h"
#include "../imgui_docks/dock_console.h"
#include "../imgui_openjk/gluecode.h"
#include "../tr_debug.h"

DockControlFlow::DockControlFlow() {}

const char *DockControlFlow::label() {
	return "ControlFlow";
}

void showCvar(char *info, cvar_t *cvar) {
	ImGui::Text(info);
	//ImGui::SameLine();
	ImGui::Cvar(cvar);
}

void DockControlFlow::imgui() {

	if (ImGui::CollapsingHeader("void RB_PostProcess() { ... }")) {
		ImGui::PushItemWidth(100);
		showCvar("if (ENABLE_DISPLACEMENT_MAPPING) RB_SSDM_Generate()"         , r_ssdm            );
		showCvar("if (r_ao >= 2) RB_SSAO()"                                    , r_ao              );
		showCvar("if (r_cartoon >= 2) RB_CellShade()"                          , r_cartoon         );
		showCvar("if (r_cartoon >= 3) RB_Paint()"                              , r_cartoon         );
#ifdef __SSDO__
		showCvar("if (AO_DIRECTIONAL) RB_SSDO()"                               , r_ssdo            );
#endif //__SSDO__
		//showCvar("RB_SSS()"                                                    , r_sss             );
		showCvar("if (r_bloom >= 2 || r_anamorphic) RB_CreateAnamorphicImage()", r_anamorphic      );
		showCvar("if (!LATE_LIGHTING_ENABLED) RB_FastLighting()"			   , r_fastLighting);
		showCvar("if (!LATE_LIGHTING_ENABLED) RB_DeferredLighting()"           , r_deferredLighting);
		//showCvar("if (r_ssr > 0 || r_sse > 0) RB_ScreenSpaceReflections()"     , r_ssr             );
		//showCvar("RB_Underwater",r_underwater); // commented out in RB_PostProcess too
		showCvar("RB_MagicDetail"                                              , r_magicdetail     );
		showCvar("Screen RB_GaussianBlur()"                                    , r_screenBlurSlow  );
		showCvar("Screen RB_SoftShadows()"                                        , r_screenBlurFast  );
		//showCvar("RB_HBAO"                                                     , r_hbao            );
		showCvar("RB_SSDM (ENABLE_DISPLACEMENT_MAPPING)"                       , r_ssdm            );
		showCvar("RB_WaterPost"                                                , r_glslWater       );
		showCvar("RB_FogPostShader (FOG_POST_ENABLED && LATE_LIGHTING_ENABLED)", r_fogPost         );
		showCvar("RB_MultiPost"                                                , r_multipost       );
		showCvar("RB_DOF"                                                      , r_dof             );
		showCvar("RB_LensFlare"                                                , r_lensflare       );
		showCvar("RB_TestShader"                                               , r_testshader      );
		showCvar("RB_FastLighting (LATE_LIGHTING_ENABLED == 1)"					, r_fastLighting);
		showCvar("RB_DeferredLighting (LATE_LIGHTING_ENABLED == 1)"            , r_deferredLighting);
		showCvar("RB_FogPostShader (FOG_POST_ENABLED && LATE_LIGHTING_ENABLED)", r_fogPost         );
		showCvar("RB_FastLighting (LATE_LIGHTING_ENABLED >= 2)"					, r_fastLighting);
		showCvar("RB_DeferredLighting (LATE_LIGHTING_ENABLED >= 2)"            , r_deferredLighting);
		showCvar("RB_ESharpening"                                              , r_esharpening     );
		//showCvar("RB_ESharpening2"                                             , r_esharpening2    );
		showCvar("RB_DarkExpand"                                               , r_darkexpand      );
		//showCvar("RB_DistanceBlur"                                             , r_distanceBlur    );
		showCvar("Bloom"                                                       , r_bloom           );
		showCvar("Anamorphic"                                                  , r_anamorphic      );
		showCvar("RB_VolumetricLight (r_dynamiclight != 0)"                    , r_volumeLight     );
		showCvar("Bloom Rays"                                                  , r_bloom           );
		showCvar("FXAA"                                                        , r_fxaa            );
		showCvar("TXAA"                                                        , r_txaa            );
		showCvar("Show Depth"                                                  , r_showdepth       );
		showCvar("Show Normals"                                                , r_shownormals     );
		showCvar("Anaglyph"                                                    , r_trueAnaglyph    );
		showCvar("FSR", r_fsr);
		ImGui::PopItemWidth();
	}
}