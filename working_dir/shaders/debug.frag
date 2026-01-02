#version 430
/*
		Basic fragment shader that just outputs
		color to the FBO
*/

// Definitions
// These locations must match between vertex/fragment shaders
#define IN_UV		layout(location = 0)
#define IN_NORMAL	layout(location = 1)
#define IN_COLOR	layout(location = 2)
#define IN_WORLD_POS layout(location = 3)

// This output must match to the COLOR_ATTACHMENTi (where 'i' is this location)
#define OUT_FBO		layout(location = 0)

// This must match GL_TEXTUREi (where 'i' is this binding)
#define T_ALBEDO	layout(binding = 0)
#define T_SPECULAR  layout(binding = 1)
#define T_NIGHT     layout(binding = 2)
#define T_SHADOW    layout(binding = 3)

// This must match the first parameter of glUniform...() calls
#define U_MODE		layout(location = 0)
#define U_CAM_POS layout(location = 4)
#define U_LIGHT_DIR layout(location = 5)
#define U_LIGHT_VP layout(location = 6)


// Input
in IN_UV	 vec2 fUV;
in IN_NORMAL vec3 fNormal;
in IN_WORLD_POS vec3 fWorld_Pos;

// Output
// This parameter goes to the framebuffer
out OUT_FBO vec4 fboColor;

// Uniforms
U_MODE uniform uint uMode;
U_CAM_POS uniform vec3 uCam_Pos;
U_LIGHT_DIR uniform vec3 uLightDir;
U_LIGHT_VP uniform mat4 uLightVP;

// Textures
uniform T_ALBEDO sampler2D tAlbedo;
uniform T_SPECULAR sampler2D tSpecular;
uniform T_NIGHT sampler2D tNight;
uniform T_SHADOW sampler2D tShadow;

float CalculateShadow(vec3 worldPos, vec3 N, vec3 L)
{
	vec4 lightSpacePos = uLightVP * vec4(worldPos,1.0);
	vec3 projCoordinates = lightSpacePos.xyz / lightSpacePos.w;
	projCoordinates *= 0.5;
	projCoordinates += 0.5;
	if(projCoordinates.z > 1.0)
	{
		return 1.0;
	}

	float closestDepth = texture(tShadow, projCoordinates.xy).r;
	float currentDepth = projCoordinates.z;
	float epsilon = max(0.005 * (1.0 - dot(N, L)), 0.005);
	float shadow = (currentDepth - epsilon > closestDepth) ? 0.0f : 1.0f;
	return shadow; 
}
void main(void)
{
	uint mode = uMode;
	switch(mode)
	{
		// Pure Red
		case 0: fboColor = vec4(1.0, 1.0, 1.0, 1.0); break;
		// Vertex Normals. Normal axes by definition is between [-1, 1])
		// Color is in between [0, 1]) so we adjust here for that
		case 1: fboColor = vec4((fNormal + 1) * 0.5, 1); break;
		// UV
		case 2: fboColor = vec4(fUV, 0, 1); break;
		// Texture Mapping without shading.
		case 3: fboColor = texture2D(tAlbedo, fUV); break;
		// If mode is wrong, put pure white.
		case 4: fboColor = vec4(gl_FragCoord.z,0.0,0.0,1.0); break;
		// Non Earth Planets mode
		case 5: 
		{
			vec3 N = normalize(fNormal);
			vec3 L = normalize(uLightDir);
			vec3 V = normalize(uCam_Pos - fWorld_Pos);
			vec3 R = reflect(-L,N);
			float shininess = 12.0f;
			vec3 texColor = texture(tAlbedo, fUV).rgb;
			float shadow = CalculateShadow(fWorld_Pos, N, L);
			vec3 ambient = 0.1 * texColor;

			float diff = max(dot(N, L),0.0);
			vec3 diffuse = diff * texColor;

			float spec = pow(max(dot(R,V),0.0),shininess);
			if(diff <= 0.0)
			{
				spec = 0.0;
			}
			vec3 specular = vec3(0.5) * spec;
			fboColor = vec4(ambient + shadow * (diffuse + specular), 1.0); break;
		}
		// Earth mode
		case 6:
		{
			vec3 N = normalize(fNormal);
			vec3 L = normalize(uLightDir);
			vec3 V = normalize(uCam_Pos - fWorld_Pos);
			vec3 R = reflect(-L,N);
			float shininess = 48.0f; 
			vec3 day_texColor = texture(tAlbedo, fUV).rgb;
			vec3 night_texColor = texture(tNight, fUV).rgb * 2.5;
			float specMapValue = texture(tSpecular, fUV).r;

			vec3 ambient = 0.05 * day_texColor;
			float shadow = CalculateShadow(fWorld_Pos, N, L);
			float diff = max(dot(N, L),0.0);
			vec3 diffuse = diff * day_texColor;
			float mixFactor = smoothstep(0.1, 0.4, diff);

			float spec = pow(max(dot(R,V),0.0),shininess);
			if(diff <= 0.0)
			{
				spec = 0.0;
			}
			vec3 specular_color = mix(vec3(1.0), day_texColor, 0.90);
			vec3 specular = specular_color * spec * specMapValue;
			vec3 day_color = ambient + shadow * (diffuse + specular); 
			vec3 finalColor = mix(night_texColor, day_color, mixFactor);
			fboColor = vec4(finalColor, 1.0); break;
		}
		// Cloud mode
		case 7:
		{
			vec3 N = normalize(fNormal);
			vec3 L = normalize(uLightDir);
			float diff = max(dot(N,L),0.0);
			float cloud_tex_alpha = texture(tAlbedo, fUV).a;
			float shadow = CalculateShadow(fWorld_Pos, N, L);
			vec3 cloudColor = vec3(1.0);
			vec3 finalColor = cloudColor * (diff * shadow + 0.1);
			fboColor = vec4(finalColor, cloud_tex_alpha); break;
		}
		default: fboColor = vec4(1); break;
		
	}
}
