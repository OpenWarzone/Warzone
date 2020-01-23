layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4				u_ModelViewProjectionMatrix;

uniform vec4				u_Settings1; // useVertexAnim, useSkeletalAnim, useFog, is2D

#define USE_IS2D			u_Settings1.a

uniform vec4				u_Local9;
uniform vec3				u_ViewOrigin;

in vec4 WorldPos_CS_in[];
in vec3 Normal_CS_in[];
in vec2 TexCoord_CS_in[];
in vec3 ViewDir_CS_in[];
in vec4 Tangent_CS_in[];
in vec4 Bitangent_CS_in[];
in vec4 Color_CS_in[];
in vec4 PrimaryLightDir_CS_in[];
in vec2 TexCoord2_CS_in[];
in vec3 Blending_CS_in[];
in float Slope_CS_in[];

out vec3 WorldPos_FS_in;
out vec2 TexCoord_FS_in;
out vec3 Normal_FS_in;
out vec3 ViewDir_FS_in;
out vec4 Tangent_FS_in;
out vec4 Bitangent_FS_in;
out vec4 Color_FS_in;
out vec4 PrimaryLightDir_FS_in;
out vec2 TexCoord2_FS_in;
out vec3 Blending_FS_in;
flat out float Slope_FS_in;


//
// Normally this is meant to be done in a pre-pass, but that would be slower...
//
bool InstanceCloudReductionCulling(vec4 InstancePosition, vec3 ObjectExtent) 
{
	/* create the bounding box of the object */
	vec4 BoundingBox[8];
	BoundingBox[0] = u_ModelViewProjectionMatrix * ( InstancePosition + vec4( ObjectExtent.x, ObjectExtent.y, ObjectExtent.z, 1.0) );
	BoundingBox[1] = u_ModelViewProjectionMatrix * ( InstancePosition + vec4(-ObjectExtent.x, ObjectExtent.y, ObjectExtent.z, 1.0) );
	BoundingBox[2] = u_ModelViewProjectionMatrix * ( InstancePosition + vec4( ObjectExtent.x,-ObjectExtent.y, ObjectExtent.z, 1.0) );
	BoundingBox[3] = u_ModelViewProjectionMatrix * ( InstancePosition + vec4(-ObjectExtent.x,-ObjectExtent.y, ObjectExtent.z, 1.0) );
	BoundingBox[4] = u_ModelViewProjectionMatrix * ( InstancePosition + vec4( ObjectExtent.x, ObjectExtent.y,-ObjectExtent.z, 1.0) );
	BoundingBox[5] = u_ModelViewProjectionMatrix * ( InstancePosition + vec4(-ObjectExtent.x, ObjectExtent.y,-ObjectExtent.z, 1.0) );
	BoundingBox[6] = u_ModelViewProjectionMatrix * ( InstancePosition + vec4( ObjectExtent.x,-ObjectExtent.y,-ObjectExtent.z, 1.0) );
	BoundingBox[7] = u_ModelViewProjectionMatrix * ( InstancePosition + vec4(-ObjectExtent.x,-ObjectExtent.y,-ObjectExtent.z, 1.0) );
   
	/* check how the bounding box resides regarding to the view frustum */   
	int outOfBound[6];// = int[6]( 0, 0, 0, 0, 0, 0 );

	for (int i=0; i<6; i++)
		outOfBound[i] = 0;

	for (int i=0; i<8; i++)
	{
		if ( BoundingBox[i].x >  BoundingBox[i].w ) outOfBound[0]++;
		if ( BoundingBox[i].x < -BoundingBox[i].w ) outOfBound[1]++;
		if ( BoundingBox[i].y >  BoundingBox[i].w ) outOfBound[2]++;
		if ( BoundingBox[i].y < -BoundingBox[i].w ) outOfBound[3]++;
		if ( BoundingBox[i].z >  BoundingBox[i].w ) outOfBound[4]++;
		if ( BoundingBox[i].z < -BoundingBox[i].w ) outOfBound[5]++;
	}

	bool inFrustum = true;
   
	for (int i=0; i<6; i++)
	{
		if ( outOfBound[i] == 8 ) 
		{
			inFrustum = false;
			break;
		}
	}

	return !inFrustum;
}

void main (void)
{
	//face center------------------------
	vec3 Vert1 = gl_in[0].gl_Position.xyz;
	vec3 Vert2 = gl_in[1].gl_Position.xyz;
	vec3 Vert3 = gl_in[2].gl_Position.xyz;

	vec3 Pos = (Vert1+Vert2+Vert3) / 3.0;   //Center of the triangle - copy for later

	// Do some ICR culling on the base surfaces... Save us looping through extra surfaces...
	vec3 maxs = max(gl_in[0].gl_Position.xyz - Pos, max(gl_in[1].gl_Position.xyz - Pos, gl_in[2].gl_Position.xyz - Pos));

	float dist = distance(Pos.xyz, u_ViewOrigin.xyz);

	if (dist > 0.0 && USE_IS2D <= 0.0)
	{
		float maxLength = max(maxs.x, max(maxs.y, maxs.z)) / dist;

		if (maxLength < 0.0002/*u_Local9.r*/)
		{// Too small on screen... Skip...
			return;
		}
	}

	if (InstanceCloudReductionCulling(vec4(Pos, 0.0), maxs*2.0))
	{
		return;
	}

	for(int i = 0; i < 3; i++) 
	{
		gl_Position				= u_ModelViewProjectionMatrix * gl_in[i].gl_Position;
		
		TexCoord_FS_in			= TexCoord_CS_in[i];
		Normal_FS_in			= Normal_CS_in[i];
		WorldPos_FS_in			= WorldPos_CS_in[i].xyz;
		ViewDir_FS_in			= ViewDir_CS_in[i];
		Color_FS_in				= Color_CS_in[i];
		Tangent_FS_in			= Tangent_CS_in[i];
		Bitangent_FS_in			= Bitangent_CS_in[i];
		PrimaryLightDir_FS_in	= PrimaryLightDir_CS_in[i];
		TexCoord2_FS_in			= TexCoord2_CS_in[i];
		Blending_FS_in			= Blending_CS_in[i];
		Slope_FS_in				= Slope_CS_in[i];

		EmitVertex();
	}
	
	EndPrimitive();
}
