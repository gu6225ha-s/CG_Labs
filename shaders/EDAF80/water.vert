#version 410

layout (location = 0) in vec3 vertex;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float time;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
} vs_out;

struct wave_par {
	float A; // Amplitude
	vec3 D; // Direction
	float f; // Frequency
	float p; // Phase
	float k; // Sharpness
};

void wave(in wave_par par, in vec3 v, in float t, out float y, out vec3 n) {
	float s = sin(dot(par.D, v) * par.f + par.p * t) * 0.5 + 0.5;
	y = par.A * pow(s, par.k);
	float c = cos(dot(par.D, v) * par.f + par.p * t);
	float dydx = 0.5 * par.k * par.f * par.A * pow(s, par.k - 1.0) * c * par.D.x;
	float dydz = 0.5 * par.k * par.f * par.A * pow(s, par.k - 1.0) * c * par.D.z;
	n = vec3(-dydx, 1.0, -dydz);
}

void main()
{
	const wave_par wave_par1 = wave_par(1.0, vec3(-1.0, 0.0, 0.0), 0.2, 0.5, 2.0);
	const wave_par wave_par2 = wave_par(0.5, vec3(-0.7, 0.0, 0.7), 0.4, 1.3, 2.0);

	float y1, y2;
	vec3 n1, n2;
	wave(wave_par1, vertex, time, y1, n1);
	wave(wave_par2, vertex, time, y2, n2);

	vec4 displaced_vertex = vec4(vertex.x, y1 + y2, vertex.z, 1.0);
	vec4 normal = vec4(n1 + n2, 0.0);

	vs_out.vertex = vec3(vertex_model_to_world * displaced_vertex);
	vs_out.normal = vec3(normal_model_to_world * normal);

	gl_Position = vertex_world_to_clip * vertex_model_to_world * displaced_vertex;
}
