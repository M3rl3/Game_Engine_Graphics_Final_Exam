// Vertex shader
#version 420

mat4 MVP;
uniform mat4 Model;
uniform mat4 ModelInverse;
uniform mat4 View;
uniform mat4 Projection;

in vec3 vPosition;
in vec3 vColour;
in vec3 vNormal;

out vec3 colour;
out vec3 normal;
out vec3 worldlocation;

void main()
{
	vec3 vertPosition = vPosition;

	MVP = Projection * View * Model;
	
	gl_Position = MVP * vec4(vertPosition, 1.f);

	worldlocation.xyz = (Model * vec4(vPosition, 1.f)).xyz;
	normal.xyz = (ModelInverse * vec4(vNormal, 1.f)).xyz;

	colour = vColour;
}
