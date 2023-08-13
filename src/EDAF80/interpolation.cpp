#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	return (1.0f - x) * p0 + x * p1;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	glm::mat4x3 pmat(p0, p1, p2, p3);
	glm::mat4x4 tmat(0, 1, 0, 0,
	                 -t, 0, t, 0,
	                 2 * t, t - 3, 3 - 2 * t, -t,
	                 -t, 2 - t, t - 2, t);
	glm::vec4 xvec(1.0f, x, x * x, x * x * x);
	return pmat * tmat * xvec;
}
