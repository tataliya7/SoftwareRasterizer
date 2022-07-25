#pragma once

#include "Shaders/ShaderCommon.h"
#include "Shader.h"

namespace SR
{
	struct SMShaderPushConstants
	{
		Matrix4x4 mvp; 
		BufferAddres vertices;
	};

	extern void ShaderMapShaderMainVS(uint32 SV_VertexID, ShaderPayload& output, const void* pushConstants);
	extern Vector4 ShaderMapShaderMainPS(const ShaderPayload& input, const void* pushConstants);
}