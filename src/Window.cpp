#include <limits>

#include <glm/fwd.hpp>

#include "RendererOpenGL.hpp"
#include "ModelLoader.hpp"

#include "ShaderProg.hpp"

#include "SceneController.hpp"

#include "Window.hpp"

// imgui stuff
//----------------------

const char* items[] = { "Option A", "Option B", "Option C" };
static int selectedItem = 0;

//----------------------


GLFWwindow* window = nullptr;

// model paths
std::string corridor_corner = "models/kenney_modular-dungeon-kit_1.0/Models/FBX_format/corridor-corner.fbx";

std::vector<GpuMesh> gpuMeshes;

std::vector<ModelData> loadedModels = {};

GLuint modelShader;

void windowlog(const char* prefix, const char* message)
{
	std::cout << "[" << prefix << "] " << message << std::endl;
}

void openWindow(int width, int height, const char* title)
{
    // window initializing 
    {
        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW\n";
            return;
        }

        // Request an OpenGL 3.3 Core context.
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window)
        {
            std::cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            return;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync.

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
            std::cerr << "Failed to initialize GLAD\n";
            glfwDestroyWindow(window);
            glfwTerminate();
            return;
        }
        glEnable(GL_DEPTH_TEST);
        modelShader = createModelShader();
        if (!modelShader)
        {
            windowlog("ShaderProg", "Failed to create model shader.");
            glfwDestroyWindow(window);
            glfwTerminate();
            return;
        }
    }
	// IMGUI initializing 
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }


    SceneController scene;
    scene.cameraPosition = glm::vec3(600.0f, 661.7f, 800.0f);
    scene.cameraTarget = glm::vec3(0.0f, 211.7f, 0.0f);
    scene.farPlane = 3000.0f;
    
    // Before the render loop:
    scene.snapToGrid = true;
    scene.gridSpacing = 100.0f; // Use 50 first so the snapping is easy to notice.




    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // IMGUI frame
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Hello", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            if (ImGui::BeginCombo("Choose an option", items[selectedItem]))
            {
                for (int i = 0; i < IM_ARRAYSIZE(items); ++i)
                {
                    const bool isSelected = (selectedItem == i);

                    if (ImGui::Selectable(items[i], isSelected))
                        selectedItem = i;

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }

            if (ImGui::Button("Click me"))
            {
                std::cout << "Button clicked! Selected: " << items[selectedItem] << '\n';

                std::string error;

                if (loadModel(corridor_corner, loadedModels.emplace_back(), error))
                {
                    windowlog("ModelLoader", "Model loaded successfully.");

                    //const ModelData& model = loadedModels.back();
                    //std::cout << "Model has " << model.meshes.size() << " meshes.\n";

                    ModelData& model = loadedModels.back();

                    for (MeshData& mesh : model.meshes)
                    {
                        if (mesh.vertices.empty() || mesh.indices.empty())
                            continue;

                        model.gpuMeshes.push_back(uploadMesh(mesh));
                    }

                    if (!model.gpuMeshes.empty())
                    {
                        scene.BeginPlacement();
                    }

                    scene.BeginPlacement();

                    //std::cout << "Model meshes: " << model.meshes.size() << '\n';

                    //for (const GpuMesh& mesh : gpuMeshes)
                    //    std::cout << "GPU mesh indices: " << mesh.indexCount << '\n';
                }
                else
                {
                    loadedModels.pop_back();
                    windowlog("ModelLoader", error.c_str());
                    windowlog("ModelLoader", "Failed to load model.");
                }
            }

            ImGui::End();
            ImGui::Render();
        }
        // IMGUI frame end

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (framebufferWidth > 0 && framebufferHeight > 0)
        {
            scene.UpdatePlacement(window, 0, 0, framebufferWidth, framebufferHeight, ImGui::GetIO().WantCaptureMouse);

            //std::cout << scene.previewPosition.x << ", "
            //    << scene.previewPosition.z << '\n';


            if (!loadedModels.empty())
            {
                ModelData& newest = loadedModels.back();

                if (scene.isPlacing)
                {
                    newest.position = scene.previewPosition;
                }
                else if (!newest.placed)
                {
                    newest.position = scene.placedPosition;
                    newest.placed = true;
                }
            }

            const glm::mat4 view = scene.GetViewMatrix();
            const glm::mat4 projection = scene.GetProjectionMatrix(static_cast<float>(framebufferWidth) / framebufferHeight);
            const float angle = static_cast<float>(glfwGetTime()) * 0.8f;

            glUseProgram(modelShader);

            const GLint mvpLocation = glGetUniformLocation(modelShader, "uMVP");
            const GLint textureLocation = glGetUniformLocation(modelShader, "uColorTexture");

            if (textureLocation != -1) glUniform1i(textureLocation, 0);

            for (const ModelData& modelData : loadedModels)
            {
                const glm::vec3 position = (!modelData.placed && scene.isPlacing) ? scene.previewPosition : modelData.position;



                const glm::mat4 model = scene.GetModelMatrix(position, modelData.center, angle);
                const glm::mat4 mvp = projection * view * model;

                if (mvpLocation != -1)
                {
                    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
                }

                for (const GpuMesh& mesh : modelData.gpuMeshes)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh.colorTexture);
                    glBindVertexArray(mesh.vao);
                    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
                }
            }

            glBindVertexArray(0);
        }



        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        // DEBUG
        {
            //GLenum glError = glGetError();
        //if (glError != GL_NO_ERROR)
        //{
        //    std::cerr << "OpenGL error after drawing: 0x"
        //        << std::hex << glError << std::dec << '\n';
        //}
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteProgram(modelShader);

    // Also delete each mesh's VAO and buffers here, using the actual
    // buffer field names from your GpuMesh definition.

    glfwDestroyWindow(window);
    glfwTerminate();
}
