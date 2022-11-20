#version 460

const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

layout (push_constant) uniform Push
{
	vec4 position;
	vec4 color;
	float radius;
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

layout (location = 0) out vec2 fragOffset;

void main()
{
	fragOffset = OFFSETS[gl_VertexIndex];

	// Camera space version
	vec4 lightInCameraSpace = ubo.viewMartix * push.position;
	vec4 positionInCameraSpace = lightInCameraSpace + push.radius * vec4(fragOffset, 0.0, 0.0);

	gl_Position = ubo.projectionMartix * positionInCameraSpace;

	// World space version
	/*vec3 cameraRightWorld = {ubo.viewMartix[0][0], ubo.viewMartix[1][0], ubo.viewMartix[2][0]};
	vec3 cameraUpWorld = {ubo.viewMartix[0][1], ubo.viewMartix[1][1], ubo.viewMartix[2][1]};

	vec3 positionWorld = push.position.xyz
		+ push.radius * fragOffset.x * cameraRightWorld
		+ push.radius * fragOffset.y * cameraUpWorld;

	gl_Position = ubo.projectionMartix * ubo.viewMartix * vec4(positionWorld, 1.0);*/
}