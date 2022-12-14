#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Particle.h"
#include "cVAOManager/cVAOManager.h"

class cMeshInfo {

public:

	cMeshInfo();
	~cMeshInfo();

	std::string meshName;

	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 target;
	glm::vec3 rotation;
	glm::vec4 colour;
	glm::vec4 RGBAColour;
	
	float scale;
	bool isWireframe;
	bool isVisible;
	bool drawBBox;
	bool useRGBAColour;
	bool teleport;

	int nTriangles;
	int nIndices;
	int nVertices;

	std::string textures[8];
	float textureRatios[8];

	Particle* particle;

	std::vector <glm::vec3> vertices;
	std::vector <glm::vec3> indices;

	void CopyVertices(sModelDrawInfo model);
	void CopyIndices(sModelDrawInfo model);

	glm::vec3 min;
	glm::vec3 max;
};