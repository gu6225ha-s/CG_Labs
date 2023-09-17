#pragma once

#include "core/InputHandler.h"
#include "core/node.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

/// @brief Spaceship class
class Spaceship
{
public:
	/// @brief Create spaceship
	Spaceship() : _transform(1.0f), _velocity(0.0f), _angular_velocity(0.0f) {}

	/// @brief Load spaceship model from file
	/// @param path Path to model
	/// @return true if successfully loaded and false otherwise
	bool load(const std::string &path);

	/// @brief Render the spaceship
	/// @param view_projection World space to clip space matrix
	/// @param show_basis Show axes of local coordinate system
	/// @param thickness_scale Thickness of axes
	/// @param length_scale Length of axes
	void render(const glm::mat4 &view_projection, bool show_basis, float thickness_scale, float length_scale) const;

	/// @brief Update the position of the spaceship
	/// @param input_handler Input handler
	/// @param elapsed_time_s Elapsed time in seconds since last update
	/// @return true if the spaceship is boosting and false otherwise
	bool update(InputHandler &input_handler, const float elapsed_time_s);

	/// @brief Get the nodes of the scene graph, with the root node at index 0
	/// @return The vector of nodes
	std::vector<Node> &nodes() { return _nodes; }

	/// @brief Get the transform which is applied to the whole scene graph (model -> world)
	/// @return The root node transform
	glm::mat4 &transform() { return _transform; }

	/// @brief Get velocity vector, in model local coordinates
	/// @return The velocity vector
	glm::vec3 &velocity() { return _velocity; }

	/// @brief Get angular velocity vector, in model local coordinates
	/// @return The angular velocity vector
	glm::vec3 &angular_velocity() { return _angular_velocity; }

private:
	std::vector<bonobo::mesh_data> _meshes;
	std::vector<Node> _nodes;
	glm::mat4 _transform;
	glm::vec3 _velocity;
	glm::vec3 _angular_velocity;
};
