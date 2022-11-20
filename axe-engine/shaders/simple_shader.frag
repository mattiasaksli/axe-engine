#version 460

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPositionWorld;
layout (location = 2) in vec3 fragNormalWorld;

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

layout (location = 0) out vec4 outColor;

// Blinn-Phong lighting model
void main()
{
	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 specularLight = vec3(0.0);
	vec3 surfaceNormal = normalize(fragNormalWorld);

	vec3 cameraWorldPosition = ubo.inverseViewMatrix[3].xyz;
	vec3 directionToViewer = normalize(cameraWorldPosition - fragPositionWorld);

	for (int i = 0; i < ubo.numLights; i++)
	{
		PointLight light = ubo.pointLights[i];

		vec3 directionToLight = light.position.xyz - fragPositionWorld;
		float attenuation = 1.0 / dot(directionToLight, directionToLight); // Magnitude squared
		directionToLight = normalize(directionToLight);

		// Diffuse light
		float cosAngleOfIncidence = clamp(dot(surfaceNormal, directionToLight), 0.0, 1.0);
		vec3 lightIntensity = light.color.xyz * light.color.w * attenuation;

		diffuseLight += lightIntensity * cosAngleOfIncidence;

		// Specular light
		
		vec3 halfAngleVector = normalize(directionToLight + directionToViewer);
		float specularTerm = clamp(dot(surfaceNormal, halfAngleVector), 0, 1);
		specularTerm = pow(specularTerm, 512.0);

		specularLight += lightIntensity * specularTerm;
	}
	
	outColor = vec4(diffuseLight * fragColor + specularLight * fragColor, 1.0);
}