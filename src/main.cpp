#include <engine/vk_engine.h>

int main(int argc, char* argv[]) {
	vkEngine::VulkanEngine& engine = vkEngine::VulkanEngine::getInstance();
	engine.run();
	return EXIT_SUCCESS;
}
