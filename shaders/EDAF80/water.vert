#version 410

layout (location = 0) in vec3 vertex;
layout (location = 2) in vec3 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float time;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 normalcoord0;
	vec2 normalcoord1;
	vec2 normalcoord2;
	mat3 tangent_to_world;
} vs_out;

struct wave_par {
	float A; // Amplitude
	vec3 D; // Direction
	float f; // Frequency
	float p; // Phase
	float k; // Sharpness
};

void wave(in wave_par par, in vec3 v, in float time, out float y, out vec3 t, out vec3 b, out vec3 n) {
	float s = sin(dot(par.D, v) * par.f + par.p * time) * 0.5 + 0.5;
	y = par.A * pow(s, par.k);
	float c = cos(dot(par.D, v) * par.f + par.p * time);
	float dydx = 0.5 * par.k * par.f * par.A * pow(s, par.k - 1.0) * c * par.D.x;
	float dydz = 0.5 * par.k * par.f * par.A * pow(s, par.k - 1.0) * c * par.D.z;
	t = vec3(1.0, dydx, 0.0);
	b = vec3(0.0, dydz, 1.0);
	n = vec3(-dydx, 1.0, -dydz);
}

void main()
{
	// Define wave parameters
	const wave_par wave_par1 = wave_par(1.0, vec3(-1.0, 0.0, 0.0), 0.2, 0.5, 2.0);
	const wave_par wave_par2 = wave_par(0.5, vec3(-0.7, 0.0, 0.7), 0.4, 1.3, 2.0);

	// Evaluate wave equation
	float y1, y2;
	vec3 t1, b1, n1, t2, b2, n2;
	wave(wave_par1, vertex, time, y1, t1, b1, n1);
	wave(wave_par2, vertex, time, y2, t2, b2, n2);

	// Compute vertex position and normal
	vec4 displaced_vertex = vec4(vertex.x, y1 + y2, vertex.z, 1.0);
	vs_out.vertex = vec3(vertex_model_to_world * displaced_vertex);
	vs_out.normal = vec3(normal_model_to_world * vec4(n1 + n2, 0.0));

	// Calculate normal map coordinates
	vec2 tex_scale = vec2(8.0, 4.0);
	float normal_time = mod(time, 100.0);
	vec2 normal_speed = vec2(-0.05, 0.0);
	vs_out.normalcoord0 = texcoord.xy * tex_scale + normal_time * normal_speed;
	vs_out.normalcoord1 = texcoord.xy * tex_scale * 2.0 + normal_time * normal_speed * 4.0;
	vs_out.normalcoord2 = texcoord.xy * tex_scale * 4.0 + normal_time * normal_speed * 8.0;

	// Compute tangent space to world space matrix
	vec3 T = normalize(vec3(normal_model_to_world * vec4(t1 + t2, 0.0)));
	vec3 B = normalize(vec3(normal_model_to_world * vec4(b1 + b2, 0.0)));
	vec3 N = normalize(vec3(normal_model_to_world * vec4(n1 + n2, 0.0)));
	vs_out.tangent_to_world = mat3(T, B, N);

	gl_Position = vertex_world_to_clip * vertex_model_to_world * displaced_vertex;
}
