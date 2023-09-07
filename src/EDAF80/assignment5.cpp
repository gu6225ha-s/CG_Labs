#include "assignment5.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"
#include "spaceship.hpp"
#include "torus.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment5::~Assignment5()
{
	bonobo::deinit();
}

void
edaf80::Assignment5::run()
{
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h

	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram("Fallback",
	                                         { { ShaderType::vertex, "common/fallback.vert" },
	                                           { ShaderType::fragment, "common/fallback.frag" } },
	                                         fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram("Skybox",
	                                         { { ShaderType::vertex, "EDAF80/skybox.vert" },
	                                           { ShaderType::fragment, "EDAF80/skybox.frag" } },
	                                         skybox_shader);
	if (skybox_shader == 0u) {
		LogError("Failed to load skybox shader");
		return;
	}

	GLuint default_shader = 0u;
	program_manager.CreateAndRegisterProgram("Default",
	                                         { { ShaderType::vertex, "EDAF80/default.vert" },
	                                           { ShaderType::fragment, "EDAF80/default.frag" } },
	                                         default_shader);
	if (default_shader == 0u) {
		LogError("Failed to load default shader");
		return;
	}

	GLuint phong_shader = 0u;
	program_manager.CreateAndRegisterProgram("Phong",
	                                         { { ShaderType::vertex, "EDAF80/phong.vert" },
	                                           { ShaderType::fragment, "EDAF80/phong.frag" } },
	                                         phong_shader);
	if (phong_shader == 0u) {
		LogError("Failed to load phong shader");
		return;
	}

	// Shader uniforms
	auto light_position = glm::vec3(-20.0f, 40.0f, 20.0f);
	bool use_normal_mapping = true;
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto const phong_set_uniforms = [&use_normal_mapping,&light_position,&camera_position](GLuint program){
		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
	};

	auto sphere = parametric_shapes::createSphere(1.0f, 100u, 100u);
	if (sphere.vao == 0u) {
		LogError("Failed to create sphere mesh");
		return;
	}

	// Load skybox
	GLuint cubemap = bonobo::loadTextureCubeMap(
		config::resources_path("cubemaps/Space/right.png"),
		config::resources_path("cubemaps/Space/left.png"),
		config::resources_path("cubemaps/Space/top.png"),
		config::resources_path("cubemaps/Space/bottom.png"),
		config::resources_path("cubemaps/Space/front.png"),
		config::resources_path("cubemaps/Space/back.png"));
	if (cubemap == 0u) {
		LogError("Failed to load the textures for the skybox");
		return;
	}

	Node skybox;
	skybox.set_geometry(sphere);
	skybox.get_transform().SetScale(100.0f);
	skybox.set_program(&skybox_shader);
	skybox.add_texture("cubemap", cubemap, GL_TEXTURE_CUBE_MAP);

	// Load sun
	GLuint const sun_texture = bonobo::loadTexture2D(config::resources_path("planets/2k_sun.jpg"));
	if (sun_texture == 0u) {
		LogError("Failed to load the texture for the sun");
		return;
	}

	Node sun;
	sun.set_geometry(sphere);
	sun.get_transform().SetScale(10.0f);
	sun.get_transform().SetTranslate(light_position);
	sun.add_texture("diffuse_texture", sun_texture, GL_TEXTURE_2D);
	sun.set_program(&default_shader);

	// Load spaceship
	Spaceship spaceship;
	if (!spaceship.load(config::resources_path("spaceship/scene.gltf"))) {
		LogError("Failed to load the spaceship model");
		return;
	}
	// Set velocity (x-fwd, y-up, z-right)
	spaceship.velocity() = glm::vec3(0.25f, 0.0f, 0.0f);
	spaceship.angular_velocity() = glm::vec3(glm::radians<float>(1.0f), 0.0f, glm::radians<float>(0.5f));
	// Translate the root node so that the model is centered around the origin in the local frame
	spaceship.nodes()[0].get_transform().SetTranslate(glm::vec3(0.45f, 0.0f, 0.0f));
	// Scale the spaceship
	spaceship.nodes()[0].get_transform().SetScale(glm::vec3(0.01f));
	for (auto &node: spaceship.nodes()) {
		node.set_program(&phong_shader, phong_set_uniforms);
	}

	// Create toruses
	// FIXME: Set proper control point locations
	std::array<glm::vec3, 9> control_point_locations = {
		glm::vec3( 0.0f,  0.0f,  0.0f),
		glm::vec3( 1.0f,  1.8f,  1.0f),
		glm::vec3( 2.0f,  1.2f,  2.0f),
		glm::vec3( 3.0f,  3.0f,  3.0f),
		glm::vec3( 3.0f,  0.0f,  3.0f),
		glm::vec3(-2.0f, -1.0f,  3.0f),
		glm::vec3(-3.0f, -3.0f, -3.0f),
		glm::vec3(-2.0f, -1.2f, -2.0f),
		glm::vec3(-1.0f, -1.8f, -1.0f),
	};
	std::vector<Torus> toruses;
	float catmull_rom_tension = 0.5f;
	size_t control_point_index = 1u;
	float x = 0.0f;
	while (control_point_index < control_point_locations.size() - 2) {
		const glm::vec3 &p0 = control_point_locations.at(control_point_index - 1);
		const glm::vec3 &p1 = control_point_locations.at(control_point_index);
		const glm::vec3 &p2 = control_point_locations.at(control_point_index + 1);
		const glm::vec3 &p3 = control_point_locations.at(control_point_index + 2);
		const glm::vec3 pos = interpolation::evalCatmullRom(p0, p1, p2, p3, catmull_rom_tension, x);
		const glm::vec3 dir = glm::normalize(interpolation::evalCatmullRom(p0, p1, p2, p3, catmull_rom_tension, x + 0.01f) - pos);
		const glm::mat4 mat = glm::translate(pos) *
		                      glm::orientation(dir, glm::vec3(0.0f, 1.0f, 0.0f)) *
		                      glm::scale(glm::vec3(0.25f));

		toruses.emplace_back(mat, 1.0f, 0.1f);
		auto &node = toruses.back().node();
		node.set_program(&phong_shader, phong_set_uniforms);
		bonobo::material_data material_constants;
		material_constants.ambient = glm::vec3(0.8f, 0.8f, 0.0f);
		material_constants.diffuse = glm::vec3(1.0f, 1.0f, 0.0f);
		material_constants.specular = glm::vec3(1.0f, 1.0f, 1.0f);
		material_constants.shininess = 10.0f;
		node.set_material_constants(material_constants);

		x += 0.3f;
		if (x > 1.0f) {
			x -= 1.0f;
			control_point_index++;
		}
	}


	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);


	auto lastTime = std::chrono::high_resolution_clock::now();

	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 0.25f;
	float basis_length_scale = 0.5f;

	// Camera and look at position in spaceship local frame
	glm::vec4 camera_translation_local(-1.5f, 0.5f, 0.0f, 1.0f), camera_look_at_local(0.0f, 0.2f, 0.0f, 1.0f);
	// Camera and look at position + up vector in world frame
	glm::vec4 camera_translation(0.0f), camera_look_at(0.0f), camera_up(0.0f);
	// Camera trajectory forget factor
	float camera_forget_factor = 0.05f;

	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(deltaTimeUs, inputHandler);

		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);


		// Retrieve the actual framebuffer size: for HiDPI monitors,
		// you might end up with a framebuffer larger than what you
		// actually asked for. For example, if you ask for a 1920x1080
		// framebuffer, you might get a 3840x2160 one instead.
		// Also it might change as the user drags the window between
		// monitors with different DPIs, or if the fullscreen status is
		// being toggled.
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);


		//
		// Todo: If you need to handle inputs, you can do it here
		//

		spaceship.update(inputHandler, std::chrono::duration<float>(deltaTimeUs).count());

		auto spaceship_position = spaceship.transform() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		auto spaceship_normal = spaceship.transform() * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		for (auto &torus: toruses) {
			if (torus.active() && torus.intersects(spaceship_position, spaceship_normal)) {
				torus.inactivate();
			}
		}

		camera_translation = interpolation::evalLERP(camera_translation, spaceship.transform() * camera_translation_local, camera_forget_factor);
		camera_look_at = interpolation::evalLERP(camera_look_at, spaceship.transform() * camera_look_at_local, camera_forget_factor);
		camera_up = interpolation::evalLERP(camera_up, spaceship.transform() * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), camera_forget_factor);

		mCamera.mWorld.SetTranslate(camera_translation);
		mCamera.mWorld.LookAt(camera_look_at, camera_up);


		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


		if (!shader_reload_failed) {
			skybox.render(mCamera.GetWorldToClipMatrix());

			sun.render(mCamera.GetWorldToClipMatrix());

			for (const auto &torus: toruses) {
				torus.render(mCamera.GetWorldToClipMatrix(), show_basis, basis_thickness_scale, basis_length_scale);
			}

			spaceship.render(mCamera.GetWorldToClipMatrix(), show_basis, 0.4f * basis_thickness_scale, 0.4f * basis_length_scale);
		}


		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//
		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
		if (opened) {
			ImGui::Checkbox("Show basis", &show_basis);
			ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 1.0f);
			ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 1.0f);
			ImGui::SliderFloat("Camera forget factor", &camera_forget_factor, 0.0f, 1.0f);
			ImGui::SliderFloat3("Camera translation", glm::value_ptr(camera_translation_local), -2.0f, 2.0f);
			ImGui::SliderFloat3("Camera look at", glm::value_ptr(camera_look_at_local), -2.0f, 2.0f);
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -20.0f, 20.0f);
			ImGui::SliderFloat3("Spaceship velocity", glm::value_ptr(spaceship.velocity()), 0.0f, 2.0f);
			ImGui::SliderFloat3("Spaceship angular velocity", glm::value_ptr(spaceship.angular_velocity()), 0.0f, glm::radians<float>(5.0f));
		}
		ImGui::End();

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
	}
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try {
		edaf80::Assignment5 assignment5(framework.GetWindowManager());
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
