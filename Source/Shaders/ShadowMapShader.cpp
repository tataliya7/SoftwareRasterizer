#include "ShadowMapShader.h"
#include "SRMath.h"
#include "Scene.h"
#include "Texture.h"

namespace SR
{
	void ShaderMapShaderMainVS(uint32 SV_VertexID, ShaderPayload& output, const void* pushConstants)
	{
		const SMShaderPushConstants& pc = *(SMShaderPushConstants*)pushConstants;

		Vector4 localPosition = Vector4(((Vector3*)pc.vertices)[SV_VertexID], 1.0f);
		output.clipPosition = pc.mvp * localPosition;
	}

	Vector4 ShaderMapShaderMainPS(const ShaderPayload& input, const void* pushConstants)
	{
		return Vector4(1.0f);
	}
}