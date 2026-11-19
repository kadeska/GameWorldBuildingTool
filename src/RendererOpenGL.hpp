#pragma once

#include <glad/glad.h>

#include <cstddef>
#include <vector>

#include <cstdint>
#include <iostream>
#include <string>

#include "ModelCommonData.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb/stb_image.h"




inline GLuint loadColorTexture(const std::string& path)
{
    if (path.empty())
    {
        std::cerr << "Mesh has no color texture path\n";
        return 0;
    }

    // Assimp's aiProcess_FlipUVs already flips the UV coordinates.
    stbi_set_flip_vertically_on_load(false);

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* pixels =
        stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!pixels)
    {
        std::cerr << "Failed to load color texture: " << path
            << " (" << stbi_failure_reason() << ")\n";
        return 0;
    }

    GLenum format = 0;
    if (channels == 1)      format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    if (format == 0)
    {
        std::cerr << "Unsupported channel count (" << channels
            << ") in texture: " << path << '\n';
        stbi_image_free(pixels);
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D, 0, format, width, height, 0,
        format, GL_UNSIGNED_BYTE, pixels
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Loaded color texture: " << path
        << " (" << width << "x" << height << ")\n";

    return texture;
}


inline GpuMesh uploadMesh(const MeshData& mesh)
{
    GpuMesh gpuMesh{};

    if (mesh.vertices.empty() || mesh.indices.empty())
    {
        std::cerr << "Cannot upload an empty mesh\n";
        return gpuMesh;
    }

    for (std::uint32_t index : mesh.indices)
    {
        if (index >= mesh.vertices.size())
        {
            std::cerr << "Mesh contains an out-of-range index: "
                << index << '\n';
            return gpuMesh;
        }
    }

    gpuMesh.colorTexture = loadColorTexture(mesh.colorTexturePath);

    gpuMesh.indexCount = static_cast<GLsizei>(mesh.indices.size());

    glGenVertexArrays(1, &gpuMesh.vao);
    glGenBuffers(1, &gpuMesh.vbo);
    glGenBuffers(1, &gpuMesh.ebo);

    glBindVertexArray(gpuMesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, gpuMesh.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)),
        mesh.vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            mesh.indices.size() * sizeof(std::uint32_t)
            ),
        mesh.indices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, position))
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, normal))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, uv))
    );
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return gpuMesh;
}
