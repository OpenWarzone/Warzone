attribute vec2 attr_TexCoord0;
attribute vec2 attr_TexCoord1;

attribute vec3 attr_Position;
attribute vec3 attr_Normal;


uniform vec4	u_Local1; // parallaxScale, haveSpecular, specularScale, materialType
uniform vec4	u_Local2; // ExtinctionCoefficient
uniform vec4	u_Local3; // RimScalar, MaterialThickness, subSpecPower, cubemapScale
uniform vec4	u_Local4; // haveNormalMap, isMetalic, 0.0, sway
uniform vec4	u_Local5; // hasRealOverlayMap, overlaySway, blinnPhong, hasSteepMap
uniform vec4	u_Local6; // useSunLightSpecular
uniform vec4	u_Local9;

uniform vec3	u_ViewOrigin;

uniform mat4	u_ModelViewProjectionMatrix;
uniform mat4	u_ViewProjectionMatrix;
uniform mat4	u_ModelMatrix;
uniform mat4	u_NormalMatrix;

uniform vec2	u_textureScale;

varying vec3	var_Normal;
varying vec3	var_vertPos;
varying vec2	var_TexCoords;

void main()
{
	vec3 position  = attr_Position;
	vec3 normal    = attr_Normal * 2.0 - 1.0;

	vec2 texCoords = attr_TexCoord0.st;
	var_TexCoords.xy = texCoords;

	if (!(u_textureScale.x == 0.0 && u_textureScale.y == 0.0) && !(u_textureScale.x == 1.0 && u_textureScale.y == 1.0))
	{
		var_TexCoords *= u_textureScale;
	}

	var_Normal = normal;
	var_vertPos = attr_Position;

	gl_Position = u_ModelViewProjectionMatrix * vec4(position, 1.0);
}
