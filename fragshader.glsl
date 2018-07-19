#version 130

// from stack overflow tutorial:


// Light Properties (colors)
// problem: Light color must be passed from code -> vertex shader -> frag shader
in vec4 gAmbientColorFrag;
in vec4 ambientColorFrag;
in vec4 diffuseColorFrag;
in vec4 specularColorFrag;

in vec4 matAmbientFrag;
in vec4 matDiffuseFrag;
in vec4 matSpecularFrag;
in float shininessFrag;

// Material Properties NOT needed to be changed?
uniform float ambientCoeff;
uniform float diffuseCoeff;
uniform float specCoeff;

uniform vec3 vecToCamera;  // vec to viewer essentially
uniform int lightsOn;

in vec3 myColor; 
//flat in vec3 myNormal;
in vec3 myNormal;
in float attenuationFrag;

vec3 ambient = vec3( 0.1, 0.1, 0.1 );
vec3 lightVecNormalized = normalize(vec3(0, 0, 3)); // this is where the light is located

out vec4 out_frag_color;

void main()
{
  vec3 normMyNormal = normalize(myNormal);  

  vec3 halfway = normalize(lightVecNormalized + normalize(vecToCamera));


  float cosTheta = dot( normalize(vecToCamera), normalize( myNormal ) );
  //vec3 perfReflection = normalize(reflect(-lightVecNormalized, myNormal));
  vec3 perfReflection = normalize((2 * dot(lightVecNormalized, normMyNormal) * normMyNormal) - lightVecNormalized);
  float cosThetaDiff = max(cosTheta, 0.0);
  
  // to fix the specular light bleeding to the rear side of the model
  float cosThetaSpec = 0.0;
  if (cosThetaDiff > 0)
  {
   // cosThetaSpec = dot(normalize(vecToCamera), perfReflection);
    cosThetaSpec = dot(normMyNormal, halfway);
  }
    
  float specular = clamp(cosThetaSpec, 0.0, 1.0);
  float specularPow = pow(specular, shininessFrag);
	//float specularPow = matSpecular * pow(specular, 128);
	
	
	// add together to create the final Phong Model convert to -1 to 1 range:
	vec4 gAmbModel = gAmbientColorFrag; // don't attenuate gAmbModel
	gAmbModel[3] = 0.0;
	
	// use attenuationFrag for attenuation
  //vec4 ambModel = ambientColorFrag[3] * ambientColorFrag; // attenuation of the ambient light component
  vec4 ambModel = ambientColorFrag; // don't attenuate as well 
  ambModel[0] = ambModel[0] * ((matAmbientFrag[0] + 1) / 2); // multiply by ka on the red channel
  ambModel[1] = ambModel[1] * (matAmbientFrag[1] + 1) / 2;
  ambModel[2] = ambModel[2] * (matAmbientFrag[2] + 1) / 2;
  //ambModel = ((matAmbientFrag[3] + 1) / 2) * ambModel;  // include attenuation of the material to the model
  ambModel = attenuationFrag * ambModel;
  ambModel[3] = 0.0;

  //vec4 diffModel = diffuseColorFrag[3] * cosThetaDiff * diffuseColorFrag;
  vec4 diffModel = cosThetaDiff * diffuseColorFrag;
  diffModel[0] = diffModel[0] * (matDiffuseFrag[0] + 1) / 2;
  diffModel[1] = diffModel[1] * (matDiffuseFrag[1] + 1) / 2;
  diffModel[2] = diffModel[2] * (matDiffuseFrag[2] + 1) / 2;
  //diffModel = ((matDiffuseFrag[3] + 1) / 2) * diffModel;
  diffModel = attenuationFrag * diffModel;
  diffModel[3] = 0.0;
  
  //vec4 specModel = specularColorFrag[3] * specularColorFrag * specularPow;
  vec4 specModel = specularColorFrag * specularPow;
  specModel[0] = specModel[0] * (matSpecularFrag[0] + 1) / 2;
  specModel[1] = specModel[1] * (matSpecularFrag[1] + 1) / 2;
  specModel[2] = specModel[2] * (matSpecularFrag[2] + 1) / 2;
  //specModel = ((matSpecularFrag[3] + 1) / 2) * specModel;
  specModel = attenuationFrag * specModel;
  specModel[3] = 0.0;  
	
	// specular not doing anything
  if (lightsOn == 1)
  {
    out_frag_color = gAmbModel + ambModel + diffModel + specModel;
  }
  else if (lightsOn == 0)
  {
    out_frag_color = gAmbModel;
  }
  //color = vec4(myColor, 1.0); 
} 

