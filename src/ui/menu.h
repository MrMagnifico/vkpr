#pragma once

#include <common/config.h>


namespace vkUi {
class Menu {
public:
    Menu(vkCommon::Config& config);
    void draw();

private:
    vkCommon::Config& m_config;

    void drawDrawControls();
};
}
