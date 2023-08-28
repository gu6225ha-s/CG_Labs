#version 410

uniform vec3 camera_position;
uniform vec4 color_deep;
uniform vec4 color_shallow;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 V = normalize(camera_position - fs_in.vertex);
	vec3 n = normalize(fs_in.normal);
	float facing = 1.0 - max(dot(V, n), 0.0);
	frag_color = mix(color_deep, color_shallow, facing);
}
