#version 460

const float M_PI = 3.1415926538;

layout (location = 0) in vec2 fragOffset;

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

layout (location = 0) out vec4 outColor;

void main()
{
	float distanceFromLight = sqrt(dot(fragOffset, fragOffset));
	if (distanceFromLight >= 1.0)
	{
		discard;
	}

	float cosDistance = 0.5 * (cos(distanceFromLight * M_PI) + 1.0);
	outColor = vec4(push.color.xyz + cosDistance, cosDistance);
}