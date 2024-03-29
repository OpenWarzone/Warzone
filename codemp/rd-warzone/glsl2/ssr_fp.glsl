uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_GlowMap;
uniform sampler2D	u_ScreenDepthMap;

uniform vec2		u_Dimensions;
uniform vec4		u_Local1; // r_ssr, r_sse, r_ssrStrength, r_sseStrength
uniform vec4		u_Local3; // r_testShaderValue1, r_testShaderValue2, r_testShaderValue3, r_testShaderValue4
uniform vec4		u_ViewInfo; // znear, zfar, zfar / znear, fov

varying vec2		var_TexCoords;

vec2 pixel = vec2(1.0) / u_Dimensions;

#define u_DiffuseMapWidth		u_Dimensions.x
#define u_DiffuseMapHeight		u_Dimensions.y

#define width					u_Dimensions.x
#define height					u_Dimensions.y

//#######################################
//these MUST match your current settings
float znear = u_ViewInfo.r;//1.0;                      //camera clipping start
float zfar = u_ViewInfo.g;//50.0;                      //camera clipping end
float fov = (90.0 / u_ViewInfo.a);                //check your camera settings, set this to (90.0 / fov) (make sure you put a ".0" after your number)
float aspectratio = (width / height) * 2.0;//16.0/9.0;           //width / height (make sure you put a ".0" after your number) -- UQ1: wierd, * 2.0 makes this accurate...
vec3 skycolor = vec3(0.0,0.0,0.0);   // use the horizon color under world properties, fallback when reflections fail

//tweak these to your liking
//float reflectStrength = 0.04;    //reflectivity of surfaced that you face head-on
const float stepSize = 0.03;//0.003;//u_Local3.b;      //reflection choppiness, the lower the better the quality, and worse the performance 
#define samples 24//100          //reflection distance, the higher the better the quality, and worse the performance
const float startScale = 4.0;//8.0;//4.0;     //first value for variable scale calculations, the higher this value is, the faster the filter runs but it gets you staircase edges, make sure it is a power of 2

//#######################################

float getDepth(vec2 coord){
    float zdepth = texture(u_ScreenDepthMap, coord).x;
#if 0
    return -zfar * znear / (zdepth * (zfar - znear) - zfar);
	//return 1.0 / mix(u_ViewInfo.z, 1.0, zdepth);
#else
	return zdepth;
#endif
}

vec3 getViewPosition(vec2 coord){
    vec3 pos = vec3((coord.s * 2.0 - 1.0) / fov, (coord.t * 2.0 - 1.0) / aspectratio / fov, 1.0);
    return (pos * getDepth(coord));
}

vec3 getViewNormal(vec2 coord){
    
    vec3 p0 = getViewPosition(coord);
    vec3 p1 = getViewPosition(coord + vec2(1.0 / width, 0.0));
    vec3 p2 = getViewPosition(coord + vec2(0.0, 1.0 / height));
  
    vec3 dx = p1 - p0;
    vec3 dy = p2 - p0;
    return normalize(cross( dy , dx ));
}

vec2 getViewCoord(vec3 pos) {
    vec3 norm = pos / pos.z;
    vec2 view = vec2((norm.x / fov + 1.0) / 2.0, (norm.y / fov * aspectratio + 1.0) / 2.0);
    return view;
}

float lenCo(vec3 vector){
    return pow(pow(vector.x,2.0) + pow(vector.y,2.0) + pow(vector.z,2.0), 0.5);
}

vec3 rayTrace(vec3 startpos, vec3 dir){
    vec3 pos = startpos;
    float olz = pos.z;      //previous z
    float scl = startScale; //step scale
    vec2 psc;               // Pixel Space Coordinate of the ray's' current viewspace position 
    vec3 ssg;               // Screen Space coordinate of the existing Geometry at that pixel coordinate
    
//#pragma unroll samples
    for(int i = 0; i < samples; i++){
        olz = pos.z; //previous z
        pos = pos + dir * stepSize * pos.z * scl;
        psc = getViewCoord(pos); 
        ssg = getViewPosition(psc); 
        if(psc.x < 0.0 || psc.x > 1.0 || psc.y < 0.0 || psc.y > 1.0 || pos.z < 0.0 || pos.z >= zfar){
            //out of bounds
            break;
        }
        if(scl == 1 && lenCo(pos) > lenCo(ssg) && lenCo(pos) - lenCo(ssg) < stepSize * 40){
            //collided
            return pos;
        }
        if(scl > 1 && lenCo(pos) - lenCo(ssg) > stepSize * scl * -1){
            //lower step scale
            pos = pos - dir * stepSize * olz * scl;
            scl = scl * 0.5;
        }
    }
    // this will only run if loop ends before return or after break statement
    return vec3(0.0, 0.0, 0.0);
}

float schlick(float r0, vec3 n, vec3 i){
    return r0 + (1.0 - r0) * pow(1.0 - dot(-i,n),5.0);
}

vec3 AddScreenSpaceReflection(vec2 coord, vec3 inColor)
{
    
    //fragment color data
    vec3 reflection = inColor;
    
    //fragment geometry data
    vec3 position = getViewPosition(var_TexCoords);
    vec3 normal   = getViewNormal(var_TexCoords);
    vec3 viewVec  = normalize(position);
    vec3 reflect  = reflect(viewVec,normal);
    
    //raytrace collision
    vec3 collision = rayTrace(position, reflect);
    
    //choose method
    if (collision.z != 0.0) {
        vec2 sampler = getViewCoord(collision);
        vec3 samColor  = clamp(texture(u_DiffuseMap, sampler).rgb, 0.0, 1.0);

		if (length(samColor.rgb) > 0.0)
		{
			if (u_Local1.r > 0.0)
			{// ssr
				reflection += samColor.rgb * u_Local1.b;
			}

			if (u_Local1.g > 0.0)
			{// sse
				vec3 glow  = clamp(texture(u_GlowMap, sampler).rgb, 0.0, 1.0);

				if (length(glow.rgb) > 0.0)
				{// Glow marked in this location, emission...
					samColor.rgb = clamp(samColor.rgb - 0.3, 0.0, 1.0);
					samColor.rgb *= 1.4285;

					if (length(samColor.rgb) > 0.0)
					{
						reflection += samColor.rgb * u_Local1.a;
					}
				}
			}
		}
    } else {
		return reflection;
    }
    
	return mix(inColor, inColor + reflection, schlick(10.0, normal, viewVec)).rgb;
	//return inColor + reflection;
}

void main(void)
{
	//vec4 color = textureLod(u_DiffuseMap, var_TexCoords, 0.0);
	gl_FragColor.rgb = AddScreenSpaceReflection(var_TexCoords, vec3(0.0));
}
