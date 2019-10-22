//
// Cleans textures, expands depth, improves distant objects, and fixes the transparancy and bright edges bugs... In short, makes *everything* look better!
//

#if defined(USE_BINDLESS_TEXTURES)
layout(std140) uniform u_bindlessTexturesBlock
{
uniform sampler2D					u_DiffuseMap;
uniform sampler2D					u_LightMap;
uniform sampler2D					u_NormalMap;
uniform sampler2D					u_DeluxeMap;
uniform sampler2D					u_SpecularMap;
uniform sampler2D					u_PositionMap;
uniform sampler2D					u_WaterPositionMap;
uniform sampler2D					u_WaterHeightMap;
uniform sampler2D					u_HeightMap;
uniform sampler2D					u_GlowMap;
uniform sampler2D					u_EnvironmentMap;
uniform sampler2D					u_TextureMap;
uniform sampler2D					u_LevelsMap;
uniform sampler2D					u_CubeMap;
uniform sampler2D					u_SkyCubeMap;
uniform sampler2D					u_SkyCubeMapNight;
uniform sampler2D					u_EmissiveCubeMap;
uniform sampler2D					u_OverlayMap;
uniform sampler2D					u_SteepMap;
uniform sampler2D					u_SteepMap1;
uniform sampler2D					u_SteepMap2;
uniform sampler2D					u_SteepMap3;
uniform sampler2D					u_WaterEdgeMap;
uniform sampler2D					u_SplatControlMap;
uniform sampler2D					u_SplatMap1;
uniform sampler2D					u_SplatMap2;
uniform sampler2D					u_SplatMap3;
uniform sampler2D					u_RoadsControlMap;
uniform sampler2D					u_RoadMap;
uniform sampler2D					u_DetailMap;
uniform sampler2D					u_ScreenImageMap;
uniform sampler2D					u_ScreenDepthMap;
uniform sampler2D					u_ShadowMap;
uniform sampler2D					u_ShadowMap2;
uniform sampler2D					u_ShadowMap3;
uniform sampler2D					u_ShadowMap4;
uniform sampler2D					u_ShadowMap5;
uniform sampler3D					u_VolumeMap;
uniform sampler2D					u_MoonMaps[4];
};
#else //!defined(USE_BINDLESS_TEXTURES)
uniform sampler2D	u_TextureMap;
#endif //defined(USE_BINDLESS_TEXTURES)


uniform vec2		u_Dimensions;

varying vec2		var_TexCoords;

const float scale = 1.0;
const float thresh = 0.93;

void main()
{
    vec4 sum = vec4(0.0);
    int x=0;
    int y=0;

    vec2 recipres = vec2(1.0f / u_Dimensions.x, 1.0f / u_Dimensions.y);

	for(y=-1; y<=1; y++)
	{
		for(x=-1; x<=1; x++) sum+=texture2D(u_TextureMap, var_TexCoords + (vec2(x,y) * recipres));
	}

    sum/=(3.0*3.0);

    vec4 s = texture2D(u_TextureMap, var_TexCoords);
    gl_FragColor = s;

#ifdef BLUR_METHOD
	//
	// This version uses the blured color, which can also blur distant objects... Looks a little like dof in some ways...
	//

    // use the blurred colour if it's darker
    if ((sum.r + sum.g + sum.b) * 0.333 < ((s.r + s.g + s.b) * 0.333) * thresh)
    {
		gl_FragColor = sum*scale;
    }
#else //!BLUR_METHOD
	//
	// This version instead calculates the brightness difference and subtracts from original color (so no distant screen blur)...
	//

	// use the diff between this color and the blurred colour if the blurred color is darker
    if ((sum.r + sum.g + sum.b) * 0.333 < ((s.r + s.g + s.b) * 0.333) * thresh)
    {
		float diff = ((s.r + s.g + s.b) - (sum.r + sum.g + sum.b)) * 0.33333;
		gl_FragColor = s*scale;
		gl_FragColor -= vec4(diff);
    }
#endif //BLUR_METHOD

	gl_FragColor.a = 1.0;
}
