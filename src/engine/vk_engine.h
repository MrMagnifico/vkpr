﻿#pragma once

#include <common/config.h>
#include <common/vk_types.h>
#include <engine/vk_descriptors.h>
#include <io/points.h>
#include <ui/menu.h>
#include <ui/object_controls.h>
#include <utils/resource_management.h>

#include <glm/gtc/matrix_transform.hpp>
#include <nfd.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>
#include <VkBootstrap.h>


namespace vkEngine {
// Constants
constexpr uint32_t FRAME_OVERLAP		= 2;
constexpr size_t NUM_QUEUES     		= 3;
constexpr uint64_t BLOCKING_TIMEOUT_NS	= 100000000; // 100ms

struct FrameDataCommands {
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;
};
using FrameDataCommandsArr = std::array<FrameDataCommands, FRAME_OVERLAP>;

// Synchronization primitives follow naming convention of
// [THING_BEING_WAITED_ON]To[THING_DOING_THE_WAITING]
struct FrameDataSynchronization {
	VkSemaphore swapchainToRender, renderToPresent;
	VkFence drawEnd;
};
using FrameDataSynchronizationArr = std::array<FrameDataSynchronization, FRAME_OVERLAP>;

class VulkanEngine {
public:
	// Singletons should not be cloneable or assignable
	VulkanEngine(VulkanEngine &other) 		= delete;
    void operator=(const VulkanEngine &)	= delete;

	// Begin main loop
	void run();

	// Get singleton instance and initialise it if this is the first access
	static VulkanEngine& getInstance();

protected:
	// Constructor and destructor are hidden to prevent usage of 'new' and 'delete'
	VulkanEngine();
	~VulkanEngine();

private:
	// Internal State
	bool m_isInitialized		= {false};
	bool m_shouldRender			= {true};
	uint32_t m_frameNumber		= {0};
	vkCommon::Config m_config	= {};

	// TODO: Replace with proper camera
	glm::vec3 m_modelMin, m_modelMax;
	vkCommon::ComputePushConstants preparePushConstants() {
		vkCommon::ComputePushConstants pushConstants;
		pushConstants.numPoints = static_cast<uint32_t>(m_pointsBuffer.size);
		constexpr float fov	= 45.0f;
		glm::vec3 center	= glm::vec3(0.0f); // Model is assumed to be globally centered
		float span 			= glm::length(m_modelMin - m_modelMax);
		auto view 			= glm::lookAt(center + glm::vec3(-0.9f * span, 0, 0.15f * span), center, glm::vec3(0, 0, -1));
		auto projection 	= glm::perspective(fov, ((float) m_windowExtent.width) / m_windowExtent.height, 0.1f, 2 * span);
		pushConstants.mvp	= projection * view * m_objectControls.modelMatrix();
		return pushConstants;
	}
	
	// Window
	VkExtent2D m_windowExtent	= {1280, 720};
	SDL_Window* m_window		= {nullptr};
	nfdwindowhandle_t m_nfdSdlWindowHandle;

	// Core Vulkan resources
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;
	VkSurfaceKHR m_windowSurface;
	
	// VMA
	VmaAllocator m_vmaAllocator;

	// Swapchain resources
	VkSwapchainKHR m_swapchain;
	VkFormat m_swapchainImageFormat;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	VkExtent2D m_swapchainExtent;

	// Queue and command resources
	FrameDataCommandsArr m_framesCommandResourcesCompute, m_framesCommandResourcesGraphics, m_framesCommandResourcesTransfer;
	VkQueue m_computeQueue, m_graphicsQueue, m_transferQueue;
	uint32_t m_computeQueueFamily, m_graphicsQueueFamily, m_transferQueueFamily;

	// Synchronisation primitives
	FrameDataSynchronizationArr m_framesSynchronizationPrimitives;

	// Resource lifecycle management
	std::array<vkUtils::DeletionQueue, FRAME_OVERLAP> m_frameResourceDeletors;
	vkUtils::DeletionQueue m_globalResourceDeletor;

	// Point cloud data
	AllocatedBuffer<vkIo::Point> m_pointsBuffer, m_pointsStagingBuffer;

	// Drawing surfaces
	AllocatedImage m_packedDataImage, m_resolvedRenderImage;
	VkExtent2D m_drawExtent;

	// Descriptor sets
	DescriptorAllocator m_globalDescriptorAllocator;
	VkDescriptorSet m_pointRenderDataDescriptors;
	VkDescriptorSetLayout m_pointRenderDataDescriptorLayout;

	// Compute effect pipelines
	VkPipelineLayout m_computePipelineLayoutCommon;
	VkPipeline m_rasterizePoints, m_resolveRender;

	// Immediate submit resources. Pool and buffer use the graphics queue
	VkFence m_immediateFence;
    VkCommandPool m_immediateCommandPool;
    VkCommandBuffer m_immediateCommandBuffer;

	// UI
	vkUi::Menu m_menu;
	vkUi::ObjectControls m_objectControls;

	// Core Vulkan resource initialisation
	void initVulkan();
	void initSwapchainAndDrawSurfaces();
	void initCommands();
	void initSyncStructures();
	void initPointBuffers();
	void initDescriptors();
	void initPipelines();
	void initImgui();

	// Swapchain management
	void createSwapchain(uint32_t width, uint32_t height);
	void destroySwapchain();

	// Current frame command resources getter
	FrameDataCommands& commandGetCurrentFrame(vkb::QueueType familyType);
	// Current frame synchronization primitives getter
	FrameDataSynchronization& syncGetCurrentFrame() { return m_framesSynchronizationPrimitives[m_frameNumber % FRAME_OVERLAP]; }
	// Current frame resource deletors getter
	vkUtils::DeletionQueue& resourceDeletorGetCurrentFrame() { return m_frameResourceDeletors[m_frameNumber % FRAME_OVERLAP]; }

	// Immediate submmission
	void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& commandRegistration);

	// Point buffer management
	void createPointBuffer();
	void reloadPointData();
	void updateDescriptorPointBuffer();

	// Draw commands
	// Entry point
	void draw();
	// Fill rasterization surface with maximally positive values
	void drawClearRasterizationSurface(VkCommandBuffer graphicsCmdBuff);
	// Rasterize point clouds to packed custom data image
	void drawPointRasterizer(VkCommandBuffer computeCmdBuff);
	// Unpack the custom point depth and color data format to intermediate image
	void drawPointColorUnpack(VkCommandBuffer computeCmdBuff);
	// ImGUI UI
	void drawImgui(VkCommandBuffer graphicsCmdBuff, VkImageView targetImageView);
};

// Singleton of the engine
static VulkanEngine* loadedEngine = {nullptr};
}
