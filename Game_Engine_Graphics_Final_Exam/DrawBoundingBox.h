#pragma once

#include "MeshInfo.h"
#include <glm/mat4x4.hpp>

void draw_bbox(MeshInfo* mesh, unsigned int shaderID, glm::mat4 meshTransform);