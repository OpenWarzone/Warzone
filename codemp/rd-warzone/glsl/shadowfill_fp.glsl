uniform vec4  u_LightOrigin;
uniform float u_LightRadius;

//uniform vec4  u_Local0;

#if defined(USE_TESSELLATION) || defined(USE_TESSELLATION_3D)
in precise vec3						WorldPos_FS_in;
#define var_Position				WorldPos_FS_in
#else //!defined(USE_TESSELLATION) && !defined(USE_TESSELLATION_3D)
varying vec3  var_Position;
#endif //defined(USE_TESSELLATION) || defined(USE_TESSELLATION_3D)

void main()
{
	//gl_FragColor = vec4(0, 0, 0, 1);
	gl_FragColor = vec4(gl_FragCoord.z, gl_FragCoord.z, gl_FragCoord.z, 1.0);
}
