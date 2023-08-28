#version 410

uniform vec3 camera_position;
uniform vec4 color_deep;
uniform vec4 color_shallow;
uniform samplerCube cubemap;
uniform int has_normal_map;
uniform sampler2D normal_map;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 normalcoord0;
	vec2 normalcoord1;
	vec2 normalcoord2;
	mat3 tangent_to_world;
} fs_in;

out vec4 frag_color;

void main()
{
	// Get normal vector
	vec3 n;
	if (has_normal_map != 0 )
	{
		vec3 n0 = texture(normal_map, fs_in.normalcoord0).rgb * 2.0 - 1.0;
		vec3 n1 = texture(normal_map, fs_in.normalcoord1).rgb * 2.0 - 1.0;
		vec3 n2 = texture(normal_map, fs_in.normalcoord2).rgb * 2.0 - 1.0;
		n = fs_in.tangent_to_world * (n0 + n1 + n2);
	}
	else
	{
		n = fs_in.normal;
	}

	// Compute normalized vectors
	vec3 V = normalize(camera_position - fs_in.vertex);
	n = normalize(n);

	// Basic water color
	float facing = 1.0 - max(dot(V, n), 0.0);
	vec4 color_water = mix(color_deep, color_shallow, facing);

	// Reflection
	vec3 R = reflect(-V, n);
	vec4 reflection = texture(cubemap, R);

	// Fresnel refraction
	float R0 = 0.02037;
	float fresnel = R0 + (1.0 - R0) * pow(1.0 - dot(V, n), 5.0);
	float eta = 1.0 / 1.33;
	vec3 F = refract(-V, n, eta);
	vec4 refraction = texture(cubemap, F);

	frag_color = color_water + reflection * fresnel + refraction * (1.0 - fresnel);
}
