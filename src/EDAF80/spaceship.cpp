#include "spaceship.hpp"
#include "core/helpers.hpp"
#include "core/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/scene.h>

#include <stack>

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

bool Spaceship::load(const std::string &path)
{
	_meshes.clear();
	_nodes.clear();

	// Load meshes
	_meshes = bonobo::loadObjects(path);
	if (_meshes.size() == 0) {
		LogError("Failed to load meshes from '%s'", path.c_str());
		return false;
	}

	// Load scene graph
	Assimp::Importer importer;
	auto const assimp_scene = importer.ReadFile(path, 0);
	if (assimp_scene == nullptr || assimp_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || assimp_scene->mRootNode == nullptr) {
		LogError("Failed to import scene from '%s'", path.c_str());
		return false;
	}

	std::stack<std::pair<struct aiNode *, Node *>> stack;
	stack.emplace(assimp_scene->mRootNode, nullptr);

	_nodes.reserve(num_nodes(assimp_scene->mRootNode));

	while(!stack.empty()) {
		struct aiNode *ai_node;
		Node *parent;
		std::tie(ai_node, parent) = stack.top();
		stack.pop();

		Node node;

		if (ai_node->mNumMeshes == 1) {
			node.set_geometry(_meshes.at(ai_node->mMeshes[0]));
		}
		else if (ai_node->mNumMeshes > 1) {
			LogWarning("Unsupported number of meshes (%u) for node %s",
			           ai_node->mNumMeshes, ai_node->mName.C_Str());
		}

		ai_transform_to_trs_transform(ai_node->mTransformation, node.get_transform());

		_nodes.push_back(node);

		if (parent != nullptr) {
			parent->add_child(&_nodes.back());
		}

		for (auto i = 0u; i < ai_node->mNumChildren; ++i) {
			stack.emplace(ai_node->mChildren[i], &_nodes.back());
		}
	}

	return true;
}

void Spaceship::render(const glm::mat4 &view_projection, bool show_basis, float thickness_scale, float length_scale) const
{
	std::stack<std::pair<const Node *, glm::mat4>> stack;
	stack.emplace(&_nodes.at(0), _transform);

	while (!stack.empty()) {
		const Node *node;
		glm::mat4 parent_transform;
		std::tie(node, parent_transform) = stack.top();
		stack.pop();

		node->render(view_projection, parent_transform);
		parent_transform = parent_transform * node->get_transform().GetMatrix();
		for (size_t i = 0; i < node->get_children_nb(); i++) {
			stack.emplace(node->get_child(i), parent_transform);
		}
	}

	if (show_basis) {
		bonobo::renderBasis(thickness_scale, length_scale, view_projection, _transform);
	}
}

void Spaceship::update(const float elapsed_time_s)
{
	_transform = glm::translate(_transform, elapsed_time_s * _velocity);
}
