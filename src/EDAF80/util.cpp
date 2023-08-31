#include "util.hpp"
#include <glm/gtx/matrix_decompose.hpp>

void glm_mat4_to_trs_transform(const glm::mat4 &mat, TRSTransformf &transform)
{
	glm::vec3 scale, translation, skew;
	glm::quat rotation;
	glm::vec4 perspective;

	glm::decompose(mat, scale, rotation, translation, skew, perspective);

	transform.SetTranslate(translation);
	transform.SetScale(scale);
	transform.SetRotate(glm::angle(rotation), glm::axis(rotation));
}