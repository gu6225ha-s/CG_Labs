#pragma once

#include "core/helpers.hpp"
#include "core/node.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

/// @brief Torus class
class Torus
{
public:
	/// @brief Create torus
	/// @param transform Model space to world space matrix
	/// @param major_radius Major radius
	/// @param minor_radius Minor radius
	Torus(const glm::mat4 &transform, const float major_radius, const float minor_radius);

	/// @brief Render the torus
	/// @param view_projection World space to clip space matrix
	/// @param show_basis Show axes of local coordinate system
	/// @param thickness_scale Thickness of axes
	/// @param length_scale Length of axes
	void render(const glm::mat4 &view_projection, bool show_basis, float thickness_scale, float length_scale) const;

	/// @brief Check if point is passing through the torus
	/// @param point Query point, in world coordinates
	/// @param normal Query normal, in world coordinates
	/// @return true if point intersects with the torus and false otherwise
	bool intersects(const glm::vec4 &point, const glm::vec4 &normal) const;

	/// @brief Get the node of the torus
	/// @return A reference to the node
	Node &node() { return _node; }

	/// @brief Check if torus is active
	/// @return true if torus is active and false otherwise
	bool active() const { return _active; }

	/// @brief Inactivate the torus
	void inactivate() { _active = false; }

	static const unsigned int MAJOR_SPLIT_COUNT;
	static const unsigned int MINOR_SPLIT_COUNT;

private:
	bonobo::mesh_data _shape;
	Node _node;
	glm::mat4 _world_to_model;
	float _major_radius;
	bool _active;
};
