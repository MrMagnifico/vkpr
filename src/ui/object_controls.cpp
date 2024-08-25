#include "object_controls.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>


vkUi::ObjectControls::ObjectControls(const vkCommon::Config& config)
: m_config(config)
{}

void vkUi::ObjectControls::processMouseEvent(const SDL_Event& event) {
    // Rotation
    if (event.type == SDL_MOUSEMOTION && event.motion.state & SDL_BUTTON_LMASK) {
        m_eulerAnglesRadians.z -= m_config.rotationSensitivity * event.motion.xrel;
        m_eulerAnglesRadians.y -= m_config.rotationSensitivity * event.motion.yrel;
    }

    // Zoom
    if (event.type == SDL_MOUSEWHEEL) {
        m_zoomScale += m_config.zoomSensitivity * event.wheel.y;
        m_zoomScale = std::clamp(m_zoomScale, ZOOM_MIN, ZOOM_MAX);
    }
}

glm::mat4 vkUi::ObjectControls::modelMatrix() const {
    glm::quat quaternion    = glm::quat(m_eulerAnglesRadians);
    glm::mat4 rotation      = glm::toMat4(quaternion);
    glm::mat4 scaleRotate   = glm::scale(rotation, glm::vec3(m_zoomScale));
    return scaleRotate;
}
