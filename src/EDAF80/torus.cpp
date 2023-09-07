#include "torus.hpp"
#include "util.hpp"
#include "parametric_shapes.hpp"

const unsigned int Torus::MAJOR_SPLIT_COUNT = 31;
const unsigned int Torus::MINOR_SPLIT_COUNT = 31;

Torus::Torus(const glm::mat4 &transform, const float major_radius, const float minor_radius)
{
	_shape = parametric_shapes::createTorus(major_radius, minor_radius, MAJOR_SPLIT_COUNT, MINOR_SPLIT_COUNT);

	_node.set_geometry(_shape);
	glm_mat4_to_trs_transform(transform, _node.get_transform());

	_world_to_model = glm::inverse(transform);

	_major_radius = major_radius;

	_active = true;
}

void Torus::render(const glm::mat4 &view_projection, bool show_basis, float thickness_scale, float length_scale) const
{
	if (!_active) {
		return;
	}

	_node.render(view_projection);

	if (show_basis) {
		bonobo::renderBasis(thickness_scale, length_scale, view_projection, _node.get_transform().GetMatrix());
	}
}

bool Torus::intersects(const glm::vec4 &point) const
{
	auto point_local = _world_to_model * point;
	return glm::l2Norm(glm::vec3(point_local)) < 0.7f * _major_radius;
}