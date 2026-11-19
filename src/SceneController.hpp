#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class SceneController {
public:
    // Camera settings
    glm::vec3 cameraPosition{ 0.0f, 1000.0f, 0.0f };
    glm::vec3 cameraTarget{ 0.0f, 211.7f, 0.0f };
    glm::vec3 cameraUp{ 0.0f, 1.0f, 0.0f };

    float fovDegrees = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 3000.0f;

    // Placement settings
    bool isPlacing = false;
    bool snapToGrid = true;
    float gridSpacing = 10.0f;

    glm::vec3 previewPosition{ 0.0f };
    glm::vec3 placedPosition{ 0.0f };

    void BeginPlacement() {
        isPlacing = true;
    }

    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(cameraPosition, cameraTarget, cameraUp);
    }

    glm::mat4 GetProjectionMatrix(float aspectRatio) const {
        return glm::perspective(glm::radians(fovDegrees), aspectRatio, nearPlane, farPlane);
    }

    // Call once per frame while the model is being placed.
    // The viewport rectangle is in window coordinates.
    // For a full-window viewport, use x/y = 0 and the window width/height.
    void UpdatePlacement(GLFWwindow* window, int viewportX, int viewportY, int viewportWidth, int viewportHeight, bool mouseIsOverUI = false) 
    {
        if (!isPlacing || mouseIsOverUI || viewportWidth <= 0 || viewportHeight <= 0) 
        {
            return;
        }

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        if (windowWidth <= 0 || windowHeight <= 0)
            return;

        mouseX *= static_cast<double>(viewportWidth) / windowWidth;
        mouseY *= static_cast<double>(viewportHeight) / windowHeight;


        // Ignore cursor positions outside the 3D viewport.
        if (mouseX < viewportX || mouseX >= viewportX + viewportWidth ||
            mouseY < viewportY || mouseY >= viewportY + viewportHeight) {
            return;
        }

        glm::mat4 view = GetViewMatrix();
        glm::mat4 projection = GetProjectionMatrix(
            static_cast<float>(viewportWidth) / viewportHeight
        );

        glm::vec3 hit;
        if (CursorToGround(
            mouseX, mouseY,
            viewportX, viewportY, viewportWidth, viewportHeight,
            view, projection, hit)) {

            previewPosition = hit;

            if (snapToGrid && gridSpacing > 0.0f) {
                previewPosition.x = std::round(previewPosition.x / gridSpacing) * gridSpacing;
                previewPosition.z = std::round(previewPosition.z / gridSpacing) * gridSpacing;
            }
        }

        // Detect the press edge so holding the button doesn't repeatedly place.
        int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        if (state == GLFW_PRESS && !leftButtonWasDown) 
        {
            placedPosition = previewPosition;
            isPlacing = false;
        }
        leftButtonWasDown = (state == GLFW_PRESS);
    }

    // Use this matrix when drawing the preview or placed model.
    // modelCenter is the model's pivot/center in its imported coordinates.
    glm::mat4 GetModelMatrix(const glm::vec3& position, const glm::vec3& modelCenter, float rotationRadians = 0.0f) const 
    {
        glm::mat4 model(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotationRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, -modelCenter);
        return model;
    }

private:
    bool leftButtonWasDown = false;

    static bool CursorToGround(double mouseX, double mouseY, int viewportX, int viewportY, int viewportWidth, int viewportHeight, const glm::mat4& view, const glm::mat4& projection, glm::vec3& hit) 
    {
        float ndcX = 2.0f * float(mouseX - viewportX) / viewportWidth - 1.0f;
        float ndcY = 1.0f - 2.0f * float(mouseY - viewportY) / viewportHeight;

        glm::mat4 inverseVP = glm::inverse(projection * view);

        glm::vec4 nearPoint = inverseVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farPoint = inverseVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

        nearPoint /= nearPoint.w;
        farPoint /= farPoint.w;

        glm::vec3 rayOrigin(nearPoint);
        glm::vec3 rayDirection = glm::normalize(glm::vec3(farPoint - nearPoint));

        // Intersect the cursor ray with the y = 0 grid plane.
        if (std::abs(rayDirection.y) < 0.0001f)
            return false;

        float t = -rayOrigin.y / rayDirection.y;
        if (t < 0.0f)
            return false;

        hit = rayOrigin + t * rayDirection;
        return true;
    }
};
