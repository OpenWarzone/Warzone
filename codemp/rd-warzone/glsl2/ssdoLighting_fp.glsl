int g_num_lights;
float g_time;
float g_occlusion_radius;
float g_occlusion_max_distance;

float4x4 g_mat_view_inv;
float2 g_resolution;

sampler2D smp_position;
sampler2D smp_normal;
sampler2D smp_occlusion;

float4 ps_main(float2 tex : TEXCOORD0) : COLOR0
{      
   float4 res = 1;

   float4 albedo    = 1.0;
   float4 position  = tex2D(smp_position, tex);
   float4 normal    = tex2D(smp_normal, tex);
   float4 occlusion = tex2D(smp_occlusion, tex);
   float4 lighting = 0.0;

   float3 point_light_positions[3];

   float time = g_time*0.75;

   float light_rad = 2;
   
   point_light_positions[0] = float3(light_rad*sin(time),   light_rad*cos(time),    1);
   point_light_positions[1] = float3(light_rad*sin(time+1), light_rad*cos(-time+2), 1);
   point_light_positions[2] = float3(light_rad*sin(time+2), light_rad*cos(-time+1), 1);

   float3 point_light_colors[3];
   
   point_light_colors[0] = 2 * float3(1,0.2,0.2);
   point_light_colors[1] = 2 * float3(0.2,1,0.2);
   point_light_colors[2] = 2 * float3(0.2,0.2,1);

   for(int i=0; i<g_num_lights; i++ )
   {
      float3 light_pos = point_light_positions[i];
      float3 to_light = position.xyz - light_pos;
      float to_light_dist = length(to_light);
      float3 to_light_norm = to_light / to_light_dist;
      float light_occlusion = 1-saturate(dot(float4(-to_light_norm,1), occlusion));

      float dist_attenuation = 1 / (1+to_light_dist*to_light_dist);
      float ndl = max(0, dot(normal.xyz, -to_light_norm));
      lighting.rgb += point_light_colors[i]*light_occlusion*ndl* dist_attenuation;
   }

   //lighting += 0.25f * pow(1-saturate(occlusion.w),2); // ambient

   res.rgb = lighting.rgb * albedo.rgb;
      
   return res;
}
