#include "core/TRSTransform.h"
#include <glm/mat4x4.hpp>

/// @brief Convert glm::mat4 to TRSTransformf
/// @param mat Input matrix
/// @param transform Output transform
/// @remark The input matrix is assumed to be made up of only translation, rotation and scaling
void glm_mat4_to_trs_transform(const glm::mat4 &mat, TRSTransformf &transform);
