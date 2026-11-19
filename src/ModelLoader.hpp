#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/material.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "ModelCommonData.hpp"



bool loadModel(const std::filesystem::path& modelPath ,ModelData& output, std::string& error)
{
    std::cout << "Working directory: " << std::filesystem::current_path().string() << '\n';


	std::string path = std::filesystem::current_path().string() + "/" + modelPath.string();

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_PreTransformVertices |
        aiProcess_FlipUVs
    );

    if (!scene || !scene->mRootNode ||
        (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
    {
        error = importer.GetErrorString();
        return false;
    }

    ModelData loaded;
    const std::filesystem::path modelDirectory = modelPath.parent_path();

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
    {
        const aiMesh* sourceMesh = scene->mMeshes[meshIndex];
        MeshData mesh;

        // Copy vertex attributes.
        for (unsigned int i = 0; i < sourceMesh->mNumVertices; ++i)
        {
            const aiVector3D& p = sourceMesh->mVertices[i];

            Vertex vertex{};
            vertex.position[0] = p.x;
            vertex.position[1] = p.y;
            vertex.position[2] = p.z;

            if (sourceMesh->HasNormals())
            {
                const aiVector3D& n = sourceMesh->mNormals[i];
                vertex.normal[0] = n.x;
                vertex.normal[1] = n.y;
                vertex.normal[2] = n.z;
            }

            if (sourceMesh->HasTextureCoords(0))
            {
                const aiVector3D& uv = sourceMesh->mTextureCoords[0][i];
                vertex.uv[0] = uv.x;
                vertex.uv[1] = uv.y;
            }

            mesh.vertices.push_back(vertex);
        }

        // Copy triangle indices.
        for (unsigned int i = 0; i < sourceMesh->mNumFaces; ++i)
        {
            const aiFace& face = sourceMesh->mFaces[i];

            for (unsigned int j = 0; j < face.mNumIndices; ++j)
                mesh.indices.push_back(face.mIndices[j]);
        }

        // Find the mesh material's base-color map, falling back to diffuse.
        if (sourceMesh->mMaterialIndex < scene->mNumMaterials)
        {
            const aiMaterial* material =
                scene->mMaterials[sourceMesh->mMaterialIndex];

            aiString textureName;
            aiReturn result = material->GetTexture(
                aiTextureType_BASE_COLOR, 0, &textureName);

            if (result != AI_SUCCESS)
            {
                result = material->GetTexture(
                    aiTextureType_DIFFUSE, 0, &textureName);
            }

            if (result == AI_SUCCESS)
            {
                std::string path = textureName.C_Str();

                // Relative external texture path: resolve it beside the FBX.
                // Paths beginning with '*' refer to textures embedded in the FBX.
                if (!path.empty() && path[0] != '*')
                {
                    mesh.colorTexturePath =
                        (modelDirectory / path).lexically_normal().string();
                }
                else
                {
                    mesh.colorTexturePath = path;
                }
            }
        }

        //if (!mesh.vertices.empty())
        //{
        //    const Vertex& v = mesh.vertices.front();
        //    std::cout << "First vertex: "
        //        << v.position[0] << ", "
        //        << v.position[1] << ", "
        //        << v.position[2] << '\n';
        //}

        //std::cout << "Vertices: " << mesh.vertices.size()
        //    << ", indices: " << mesh.indices.size() << '\n';

        loaded.meshes.push_back(std::move(mesh));

        


    }

    output = std::move(loaded);
    return true;
}
