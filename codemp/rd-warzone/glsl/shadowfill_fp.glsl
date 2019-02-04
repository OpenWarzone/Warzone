uniform vec4  u_LightOrigin;
uniform float u_LightRadius;

//uniform vec4  u_Local0;

varying vec3  var_Position;

void main()
{
	//if (var_Position.z >= u_Local0.z)
	//	gl_FragColor = vec4(0, 0, 0, 0);
	//else
		gl_FragColor = vec4(0, 0, 0, 1);
}
