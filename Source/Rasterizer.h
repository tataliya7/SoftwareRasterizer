#pragma once

#include "SRCommon.h"
#include "SRMath.h"
#include "Shader.h"

namespace SR
{
    enum FillMode
    {
        FILL_MODE_SOLID          = 0,
        FILL_MODE_WIREFRAME      = 1,
    };

    enum CullMode
    {
        CULL_MODE_NONE           = 0,
        CULL_MODE_BACK           = 1,
        CULL_MODE_FRONT          = 2,
        CULL_MODE_FRONT_AND_BACK = 3,
    };

    enum CompareOp
    {
        COMPARE_OP_LESS_OR_EQUAL = 0,
        COMPARE_OP_GREATER       = 1,
    };

    template <typename T>
    class RenderTarget
    {
    public:
        RenderTarget(uint32 w, uint32 h)
            : width(w)
            , height(h)
        {

        }
        uint32 GetWidth() const
        {
            return width;
        }
        uint32 GetHeight() const
        {
            return height;
        }
        void Resize(uint32 w, uint32 h)
        {
            width = w;
            height = h;
            buffer.resize(width * height);
        }
        void Clear(const T& clearValue)
        {
            for (uint32 i = 0; i < width * height; i++)
            {
                buffer[i] = clearValue;
            }
        }
        const T& Load(uint32 x, uint32 y) const
        {
            uint32 index = y * width + x;
            return buffer[index];
        }
        void Store(uint32 x, uint32 y, const T& value)
        {
            uint32 index = y * width + x;
            buffer[index] = value;
        }
        void* GetDataPtr()
        {
            return buffer.data();
        }
    private:
        uint32 width;
        uint32 height; 
        std::vector<T> buffer;
    };

    struct GraphicsPipelineState
    {
        // Shaders
        VertexShader vertexShader;
        PixelShader pixelShader;
        // Rasterization state
        FillMode fillMode;
        CullMode cullMode;
        bool frontCCW;
        // Depth state
        bool depthTestEnable;
        bool depthWriteEnable;
        CompareOp depthCompareOp;
        RenderTarget<float>* depthBuffer;
        RenderTarget<glm::u8vec4>* colorBuffer;
    };

    struct Viewport
    {
        float x;
        float y;
        float width;
        float height;
    };

    struct RasterizerStatatics
    {

    };

    class Rasterizer
    {
    public:
        std::vector<ShaderPayload> payloads;
        void SetViewport(float x, float y, float width, float height); 
        void DrawPrimitives(const GraphicsPipelineState& pipelineState, const void* pushConstants, uint32 numVertices, const std::vector<Primitive>& primitives, uint32 numPrimitives, float zNear, float zFar);
    private:
        Viewport viewport;
    };
}