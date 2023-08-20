#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

out VS_OUT {
	vec2 texcoord;
	vec3 normal;
	vec3 position;
	mat3 tangent_to_world;
} vs_out;

void main()
{
	vec4 vertex_world = vertex_model_to_world * vec4(vertex, 1.0);
	gl_Position = vertex_world_to_clip * vertex_world;

	// Compute tangent, binormal, normal in world coordinates
	vec3 T = normalize(vec3(normal_model_to_world * vec4(tangent, 0.0)));
	vec3 B = normalize(vec3(normal_model_to_world * vec4(binormal, 0.0)));
	vec3 N = normalize(vec3(normal_model_to_world * vec4(normal, 0.0)));

	// Texture coordinates
	vs_out.texcoord = texcoord.xy;
	// Normal, in world space
	vs_out.normal = N;
	// Position, in world space
	vs_out.position = vertex_world.xyz;
	// Tangent to world space matrix
	vs_out.tangent_to_world = mat3(T, B, N);
}
