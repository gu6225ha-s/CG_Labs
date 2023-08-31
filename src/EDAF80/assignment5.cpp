#include "assignment5.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"
#include "torus.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <assimp/Importer.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/scene.h>

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stack>
#include <stdexcept>

/// @brief Count number of nodes in the assimp scene graph
/// @param[in] node Input node
/// @return Number of nodes
size_t num_nodes(struct aiNode *node)
{
	size_t n = 1;
	for (auto i = 0u; i < node->mNumChildren; i++) {
		n += num_nodes(node->mChildren[i]);
	}
	return n;
}

/// @brief Convert assimp transform to TRS transform
/// @param[in] ai_trafo Input transform
/// @param[out] trs_trafo Output transform
void ai_transform_to_trs_transform(const aiMatrix4x4 &ai_trafo, TRSTransformf &trs_trafo)
{
	aiVector3t<ai_real> scaling;
	aiQuaterniont<ai_real> rotation;
	aiVector3t<ai_real> position;
	ai_trafo.Decompose(scaling, rotation, position);

	trs_trafo.SetScale(glm::vec3(scaling.x, scaling.y, scaling.z));
	trs_trafo.SetTranslate(glm::vec3(position.x, position.y, position.z));
	glm::quat quat(rotation.w, rotation.x, rotation.y, rotation.z);
	trs_trafo.SetRotate(glm::angle(quat), glm::axis(quat));
}

/// @brief Load scene graph using assimp
/// @param[in] path Path to file
/// @param[in] meshes Meshes loaded with bonobo::loadObjects()
/// @param[out] nodes Resulting list of nodes, with the root node at index 0
/// @return true if successful and false otherwise
bool load_scene(const std::string &path, std::vector<bonobo::mesh_data> &meshes,
                std::vector<Node> &nodes)
{
	Assimp::Importer importer;
	auto const assimp_scene = importer.ReadFile(path, 0);
	if (assimp_scene == nullptr || assimp_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || assimp_scene->mRootNode == nullptr) {
		LogError("Failed to import %s", path.c_str());
		return false;
	}

	std::stack<std::pair<struct aiNode *, Node *>> stack;
	stack.emplace(assimp_scene->mRootNode, nullptr);

	nodes.reserve(num_nodes(assimp_scene->mRootNode));

	while(!stack.empty()) {
		struct aiNode *ai_node;
		Node *parent;
		std::tie(ai_node, parent) = stack.top();
		stack.pop();

		Node node;

		if (ai_node->mNumMeshes == 1) {
			node.set_geometry(meshes.at(ai_node->mMeshes[0]));
		}
		else if (ai_node->mNumMeshes > 1) {
			LogWarning("Unsupported number of meshes (%u) for node %s",
			           ai_node->mNumMeshes, ai_node->mName.C_Str());
		}

		ai_transform_to_trs_transform(ai_node->mTransformation, node.get_transform());

		nodes.push_back(node);

		if (parent != nullptr) {
			parent->add_child(&nodes.back());
		}

		for (auto i = 0u; i < ai_node->mNumChildren; ++i) {
			stack.emplace(ai_node->mChildren[i], &nodes.back());
		}
	}

	return true;
}

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
	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f); // FIXME
	bool use_normal_mapping = true;
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto const phong_set_uniforms = [&use_normal_mapping,&light_position,&camera_position](GLuint program){
		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
	};

	// Load skybox
	auto skybox_shape = parametric_shapes::createSphere(100.0f, 100u, 100u);
	if (skybox_shape.vao == 0u) {
		LogError("Failed to retrieve the mesh for the skybox");
		return;
	}

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
	skybox.set_geometry(skybox_shape);
	skybox.set_program(&skybox_shader);
	skybox.add_texture("cubemap", cubemap, GL_TEXTURE_CUBE_MAP);

	// Load spaceship
	auto spaceship_path = config::resources_path("spaceship/scene.gltf");
	auto spaceship_meshes = bonobo::loadObjects(spaceship_path);
	if (spaceship_meshes.size() == 0) {
		LogError("Failed to load meshes for the spaceship");
		return;
	}
	std::vector<Node> spaceship_nodes;
	if (!load_scene(spaceship_path, spaceship_meshes, spaceship_nodes)) {
		LogError("Failed to load scene graph for the spaceship");
		return;
	}
	for (auto &node: spaceship_nodes) {
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


		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


		if (!shader_reload_failed) {
			skybox.render(mCamera.GetWorldToClipMatrix());

			for (const auto &torus: toruses) {
				torus.render(mCamera.GetWorldToClipMatrix(), show_basis, basis_thickness_scale, basis_length_scale);
			}

			// Traverse the scene graph and render all nodes
			std::stack<std::pair<const Node *, glm::mat4>> stack;
			glm::mat4 transform = glm::rotate(
				glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)),
				glm::half_pi<float>(),
				glm::vec3(0.0f, 1.0f, 0.0f));
			transform = glm::translate(transform, glm::vec3(x, 0.1f, 0.0f));
			stack.emplace(&spaceship_nodes[0], transform);

			while (!stack.empty()) {
				const Node *node;
				glm::mat4 parent_transform;
				std::tie(node, parent_transform) = stack.top();
				stack.pop();

				node->render(mCamera.GetWorldToClipMatrix(), parent_transform);
				parent_transform = parent_transform * node->get_transform().GetMatrix();
				for (size_t i = 0; i < node->get_children_nb(); i++) {
					stack.emplace(node->get_child(i), parent_transform);
				}
			}
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
