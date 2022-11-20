#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;

layout (push_constant) uniform Push 
{
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

struct PointLight
{
	vec4 position; // ignore w
	vec4 color; // w is intensity
};

layout (set = 0, binding = 0) uniform GlobalUBO 
{
	mat4 projectionMartix;
	mat4 viewMartix;
	mat4 inverseViewMatrix;
	vec4 ambientLightColor; // w is intensity
	PointLight pointLights[10];
	int numLights;
} ubo;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragPositionWorld;
layout (location = 2) out vec3 fragNormalWorld;

void main()
{
	vec4 positionWorld = push.modelMatrix * vec4(position, 1.0f);
	fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
	fragPositionWorld = positionWorld.xyz;
	fragColor = color;

	gl_Position = ubo.projectionMartix * ubo.viewMartix * positionWorld;
}