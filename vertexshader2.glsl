#version 130

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 NormalMatrix; // to transform normals, pre-perspective

// SENT TO THE FRAGMENT SHADER:
uniform vec4 gAmbientColor; // global ambient color
uniform vec4 ambientColor;
uniform vec4 diffuseColor;
uniform vec4 specularColor;
uniform vec4 matAmbient;
uniform vec4 matDiffuse;
uniform vec4 matSpecular;
uniform float shininess;

uniform vec3 inputColor;

in vec3 vertPosition; // this is in model space
in vec3 vertColor;
in vec3 VertexNormal; // we now need a surface normal

//flat vec3 VertexNormal;

out vec3 myColor;  
flat out vec3 myNormal;  // interpolate the normalized surface normal
//out vec3 myNormal;
out vec3 vecToViewer;

// from the other tutorial, NOT NEEDED
out vec3 LightDirection_cameraspace;
out vec3 Normal_cameraspace;

out vec4 gAmbientColorFrag;
out vec4 ambientColorFrag;
out vec4 diffuseColorFrag;
out vec4 specularColorFrag;

out vec4 matAmbientFrag;
out vec4 matDiffuseFrag;
out vec4 matSpecularFrag;
out float shininessFrag;
out float attenuationFrag;


void main()
{
  vec3 lightVec = vec3(0, 0, 3); // light's location, see Assignment3.cpp
  float a = 0.5;
  float b = 0.5;
  float c = 0.5;
  vec3 diff = lightVec - vertPosition;
  float d = sqrt(diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2]);
  
  attenuationFrag = 1 / (a + (b * d) + (c * d * d));
  
  gl_Position = projectionMatrix * modelViewMatrix * vec4(vertPosition, 1.0);
  
  //convert to -1 to 1 range: 
  ambientColorFrag[0] = (ambientColor[0] + 1) / 2;
  ambientColorFrag[1] = (ambientColor[1] + 1) / 2;
  ambientColorFrag[2] = (ambientColor[2] + 1) / 2;
  ambientColorFrag[3] = (ambientColor[3] + 1) / 2;
  
  diffuseColorFrag[0] = (diffuseColor[0] + 1) / 2;
  diffuseColorFrag[1] = (diffuseColor[1] + 1) / 2;
  diffuseColorFrag[1] = (diffuseColor[1] + 1) / 2;
  diffuseColorFrag[1] = (diffuseColor[1] + 1) / 2;
 
  specularColorFrag[0] = (specularColorFrag[0] + 1) / 2;
  specularColorFrag[1] = (specularColorFrag[1] + 1) / 2;
  specularColorFrag[2] = (specularColorFrag[2] + 1) / 2;
  specularColorFrag[3] = (specularColorFrag[3] + 1) / 2;
 
  gAmbientColorFrag = gAmbientColor;
  ambientColorFrag = ambientColor;
  diffuseColorFrag = diffuseColor;
  specularColorFrag = specularColor;

  matAmbientFrag = matAmbient;
  matDiffuseFrag = matDiffuse;
  matSpecularFrag = matSpecular;
  shininessFrag = shininess;
 
  // pass to fragment shaders
  myNormal = VertexNormal;
  //vecToViewer = vecToCamera; // used for specular component calculation  
  myColor = inputColor;
}
