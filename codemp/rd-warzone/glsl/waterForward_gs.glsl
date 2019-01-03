layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4 u_ModelViewProjectionMatrix;

in vec3 FragPos[];

out vec3 WorldPos_FS_in;
out vec3 Normal_FS_in;
 
void main()
{
    vec3 A = FragPos[2] - FragPos[0];
    vec3 B = FragPos[1] - FragPos[0];
    vec3 normal = normalize(cross(A, B));

    for (int i = 0; i < 3; i++)
    {
      WorldPos_FS_in = FragPos[i].xyz;
      Normal_FS_in = normal;

      gl_Position = u_ModelViewProjectionMatrix * vec4(gl_in[i].gl_Position.xyz, 1);
      EmitVertex();
    }
 
    EndPrimitive();
}
