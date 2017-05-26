#include "DeferredShadingCommon.glsl"
#include "MaterialProcess.glsl"
#include "ViewParam.glsl"
#include "VertexProcess.glsl"

#if VERTEX_SHADER

out VertexFactoryOutputVS VFOutputVS;
void BassPassVS()
{
	VertexFactoryIntermediatesVS intermediates = GetVertexFactoryIntermediatesVS();

	MaterialInputVS materialInput = InitMaterialInputVS();
	MaterialParametersVS materialParameters = GetMaterialParameterVS( intermediates );

	CalcMaterialInputVS( materialInput , materialParameters );

	gl_Position = CalcVertexFactoryOutputVS(VFOutputVS, intermediates , materialInput , materialParameters );
	//gl_Position = ftransform();
}

#endif //VERTEX_SHADER

#if PIXEL_SHADER

#define VFInputPS VFOutputVS

in VertexFactoryIutputPS VFOutputVS;

void BasePassPS()
{

	MaterialInputPS materialInput = InitMaterialInputPS();
	MaterialParametersPS materialParameters = GetMaterialParameterPS(VFInputPS);

	CalcMaterialParameters(materialInput , materialParameters);

	//float4 color = float4( materialParameters.vectexColor * 0.1 , 1 );
	float4 color = float4(0.0);
	//color.rgb += materialInput.emissiveColor;
	color.a = 1.0;


	GBufferData GBuffer;
	GBuffer.worldPos = GetMaterialWorldPositionAndCheckDepthOffset(materialInput, materialParameters);
	//GBuffer.normal = normalize(vsOutput.normal.xyz);
	GBuffer.normal = GetMaterialWorldNormal(materialInput , materialParameters);
	//GBuffer.normal = materialParameters.worldNormal;

	GBuffer.baseColor = materialInput.baseColor;
	GBuffer.metallic = materialInput.metallic;
	GBuffer.roughness = materialInput.roughness;
	GBuffer.specular = materialInput.specular;
	GBuffer.shadingModel = materialInput.shadingModel;
	float4 svPos = View.worldToClip * float4(materialParameters.worldPos, 1);
	svPos = materialParameters.screenPos;
	GBuffer.depth = -materialParameters.screenPos.z;
	
	float4 GBufferA, GBufferB, GBufferC, GBufferD;
	EncodeGBuffer(GBuffer, GBufferA, GBufferB, GBufferC, GBufferD);

	gl_FragData[0] = color;
	gl_FragData[1] = GBufferA;
	gl_FragData[2] = GBufferB;
	gl_FragData[3] = GBufferC;
	gl_FragData[4] = GBufferD;
}


#endif //PIXEL_SHADER
