#pragma once

#include "core/node.hpp"

#include <glm/mat4x4.hpp>

/// @brief Spaceship class
class Spaceship
{
public:
	/// @brief Create spaceship
	Spaceship() : _transform(1.0f) {}

	/// @brief Render the spaceship
	/// @param view_projection World space to clip space matrix
	/// @param show_basis Show axes of local coordinate system
	/// @param thickness_scale Thickness of axes
	/// @param length_scale Length of axes
	void render(const glm::mat4 &view_projection, bool show_basis, float thickness_scale, float length_scale) const;

	/// @brief Load spaceship model from file
	/// @param path Path to model
	/// @return true if successfully loaded and false otherwise
	bool load(const std::string &path);

	/// @brief Get the nodes of the scene graph
	/// @return The vector of nodes
	std::vector<Node> &nodes() { return _nodes; }

	/// @brief Get the transform which is applied to the whole scene graph
	/// @return The root node transform
	glm::mat4 &transform() { return _transform; }

private:
	std::vector<bonobo::mesh_data> _meshes;
	std::vector<Node> _nodes;
	glm::mat4 _transform;
};
