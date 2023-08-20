#version 410

uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;
uniform vec3 light_position;
uniform vec3 camera_position;
uniform float shininess_value;
uniform int has_diffuse_texture;
uniform sampler2D diffuse_texture;
uniform int has_specular_map;
uniform sampler2D specular_map;
uniform int has_normal_map;
uniform sampler2D normal_map;
uniform int use_normal_mapping;

in VS_OUT {
	vec2 texcoord;
	vec3 normal;
	vec3 position;
	mat3 tangent_to_world;
} fs_in;

out vec4 frag_color;

void main()
{
	// Get normal vector
	vec3 n;
	if (has_normal_map != 0 && use_normal_mapping != 0)
	{
		n = fs_in.tangent_to_world * (texture(normal_map, fs_in.texcoord).rgb * 2.0 - 1.0);
	}
	else
	{
		n = fs_in.normal;
	}

	// Compute normalized vectors
	n = normalize(n);
	vec3 L = normalize(light_position - fs_in.position);
	vec3 V = normalize(camera_position - fs_in.position);

	// Ambient
	frag_color = vec4(ambient_colour, 1.0);

	// Diffuse
	vec4 diffuse_color;
	if (has_diffuse_texture != 0)
	{
		diffuse_color = texture(diffuse_texture, fs_in.texcoord);
	}
	else
	{
		diffuse_color = vec4(diffuse_colour, 1.0);
	}
	float diffuse_factor = max(dot(n, L), 0.0);
	frag_color += diffuse_factor * diffuse_color;

	// Specular
	vec4 specular_color;
	if (has_specular_map != 0)
	{
		specular_color = texture(specular_map, fs_in.texcoord);
	}
	else
	{
		specular_color = vec4(specular_colour, 1.0);
	}
	float specular_factor = max(pow(dot(reflect(-L, n), V), shininess_value), 0.0);
	frag_color += specular_factor * specular_color;
}
