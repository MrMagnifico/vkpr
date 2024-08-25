#pragma once

#include <common/config.h>

#include <nfd.h>


namespace vkUi {
class Menu {
public:
    Menu(vkCommon::Config& config, const nfdwindowhandle_t& nfdSdlWindowHandle);
    void draw();

private:
    // Externally owned state
    vkCommon::Config& m_config;
    const nfdwindowhandle_t& m_nfdSdlWindowHandle;

    void drawFileControls();
    void drawObjectControls();
};
}
