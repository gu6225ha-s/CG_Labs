#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

out VS_OUT {
	vec2 texcoord;
	vec3 normal;
	vec3 position;
} vs_out;

void main()
{
	vec4 vertex_world = vertex_model_to_world * vec4(vertex, 1.0);
	gl_Position = vertex_world_to_clip * vertex_world;

	// Texture coordinates
	vs_out.texcoord = texcoord.xy;
	// Normal, in world space
	vs_out.normal = normalize(normal_model_to_world * vec4(normal, 1.0)).xyz;
	// Position, in world space
	vs_out.position = vertex_world.xyz;
}
