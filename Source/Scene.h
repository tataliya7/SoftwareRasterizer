#pragma once

#include "SRCommon.h"
#include "SRMath.h"

namespace SR
{
    class Texture;

    struct Camera
    {
        float fieldOfView;
        float aspectRatio;
        float zNear;
        float zFar;
        Vector3 position;
        Vector3 euler;
    };

    struct Transform
    {
        Vector3 position;
        Vector3 rotation;
        Vector3 scale;
        Matrix4x4 world;
    };

    struct DirectionalLight
    {
        Vector3 color;
        float intensity;
        Vector3 direction;
    };

    struct PBRMaterial
    {
        Vector4 baseColor;
        float metallic;
        float roughness;
        Vector4 emission;
        std::shared_ptr<Texture> baseColorMap = nullptr;
        std::shared_ptr<Texture> normalMap = nullptr;
        std::shared_ptr<Texture> metallicRoughnessMap = nullptr;
    };

    struct Mesh
    {
        //uint32 numIndices;
        uint32 numVertices;
        uint32 numPrimitives;

        std::vector<Vector3> positions;
        std::vector<Vector3> tangents;
        std::vector<Vector3> normals;
        std::vector<Vector2> texCoords;
        std::vector<Primitive> primitives;

        PBRMaterial material;
    };
    
    extern bool ImportGLTF2(const char* filename, Mesh* mesh);
};