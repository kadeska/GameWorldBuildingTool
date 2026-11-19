#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct Vertex
{
    float position[3]{};
    float normal[3]{};
    float uv[2]{};
};

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    // Empty if the mesh has no color map.
    // A value such as "*0" means the texture is embedded in the FBX.
    std::string colorTexturePath;
};

struct GpuMesh
{
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
    GLuint colorTexture = 0;

};

struct ModelData
{
    int ID = -1;
    std::vector<MeshData> meshes;       // Existing CPU-side meshes
    std::vector<GpuMesh> gpuMeshes;     // Uploaded meshes for this model

    glm::vec3 position{ 0.0f };
    glm::vec3 center{ 0.0f, 211.7f, 0.0f };
    bool placed = false;
};