uniform sampler2D			u_DiffuseMap;			// Screen
uniform sampler2D			u_PositionMap;			// Positions
uniform sampler2D			u_NormalMap;			// Normals
uniform sampler2D			u_DeluxeMap;			// Occlusion

uniform vec2				u_Dimensions;
uniform vec3				u_ViewOrigin;
uniform vec4				u_PrimaryLightOrigin;

uniform vec4				u_Local0;

#define dir					u_Local0.rg

varying vec2   				var_TexCoords;

#define znear				u_ViewInfo.r			//camera clipping start
#define zfar				u_ViewInfo.g			//camera clipping end

#if 0
#define BLURBOX_HALFSIZE 8
#define PENUMBRA_SIZE_CONST 4
#define MAX_PENUMBRA_SIZE 8
#define DEPTH_REJECTION_EPISILON 1.0    

vec4 fragBlur(vec2 i)
{
    float2 dentisyAndOccluderDistance = tex2D(_MainTex,i.uv).rg;
    fixed dentisy = dentisyAndOccluderDistance.r;
    float occluderDistance = dentisyAndOccluderDistance.g;
    float maxOccluderDistance = 0;

    float3 uvOffset = float3(_MainTex_TexelSize.xy, 0); //convenient writing here.
    for (int j = 0; j < BLURBOX_HALFSIZE; j++) {        //search on vertical and horizontal for nearest shadowed pixel.
        float top = tex2D(_MainTex, i.uv + j * uvOffset.zy).g;
        float bot = tex2D(_MainTex, i.uv - j * uvOffset.zy).g;
        float lef = tex2D(_MainTex, i.uv + j * uvOffset.xz).g;
        float rig = tex2D(_MainTex, i.uv - j * uvOffset.xz).g;
        if (top != 0 || bot != 0 || lef != 0 || rig != 0) {
            maxOccluderDistance = max(top, max(bot, max (lef, rig)));
            break;
        }
    }

    float penumbraSize = maxOccluderDistance * PENUMBRA_SIZE_CONST;

    float camDistance = LinearEyeDepth(tex2D(_CameraDepthTexture, i.uv));

    float projectedPenumbraSize = penumbraSize / camDistance;

    projectedPenumbraSize = min(1 + projectedPenumbraSize, MAX_PENUMBRA_SIZE);

    float depthtop = LinearEyeDepth(tex2D(_CameraDepthTexture, i.uv + j * uvOffset.zy));
    float depthbot = LinearEyeDepth(tex2D(_CameraDepthTexture, i.uv - j * uvOffset.zy));
    float depthlef = LinearEyeDepth(tex2D(_CameraDepthTexture, i.uv + j * uvOffset.xz));
    float depthrig = LinearEyeDepth(tex2D(_CameraDepthTexture, i.uv - j * uvOffset.xz));

    float depthdx = min(abs(depthrig - camDistance), abs(depthlef - camDistance));
    float depthdy = min(abs(depthtop - camDistance), abs(depthbot - camDistance));

    float counter = 0;
    float accumulator = 0;
    UNITY_LOOP
    for (int j = -projectedPenumbraSize; j < projectedPenumbraSize; j++) {  //xaxis
        for (int k = -projectedPenumbraSize; k < projectedPenumbraSize; k++) {  //yaxis
            float depth = LinearEyeDepth(tex2Dlod(_CameraDepthTexture, float4(i.uv + uvOffset.xy * float2(j, k),0,0)));
            if (depthdx * abs(j) + depthdy * abs(k) + DEPTH_REJECTION_EPISILON < abs(camDistance - depth))        //depth rejection
                break;
            counter += 1;
            accumulator += tex2Dlod(_MainTex, float4(i.uv + uvOffset.xy * float2(j, k),0,0)).r;
        }
    }
    return (1 - saturate(accumulator / counter));
}
#endif

void main() 
{
	//gl_FragColor = fragBlur(var_TexCoords);
	gl_FragColor = vec4(0.0);
}
