#include "torus.hpp"
#include "util.hpp"
#include "parametric_shapes.hpp"

const float Torus::MAJOR_RADIUS = 1.0f;
const float Torus::MINOR_RADIUS = 0.1f;
const unsigned int Torus::MAJOR_SPLIT_COUNT = 31;
const unsigned int Torus::MINOR_SPLIT_COUNT = 31;

Torus::Torus(const glm::mat4 &matrix, const GLuint *program)
{
	_shape = parametric_shapes::createTorus(MAJOR_RADIUS, MINOR_RADIUS, MAJOR_SPLIT_COUNT, MINOR_SPLIT_COUNT);

	_node.set_geometry(_shape);
	_node.set_program(program);
	glm_mat4_to_trs_transform(matrix, _node.get_transform());

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

bool Torus::intersects(const glm::vec3 &point) const
{
	return false; // FIXME
}