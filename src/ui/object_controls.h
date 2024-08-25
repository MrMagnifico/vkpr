#pragma once

#include <common/config.h>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <SDL_events.h>

namespace vkUi {
class ObjectControls {
public:
    ObjectControls(const vkCommon::Config& config);

    void processMouseEvent(const SDL_Event& event);
    glm::mat4 modelMatrix() const;

private:
    static constexpr float ZOOM_MIN = 0.01f;
    static constexpr float ZOOM_MAX = 100.0f;

    // Externally owned state
    const vkCommon::Config& m_config;

    glm::vec3 m_eulerAnglesRadians  = {0.0f, 0.0f, 0.0f};
    float m_zoomScale               = 1.0f;
};
}
