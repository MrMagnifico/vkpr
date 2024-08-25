//> includes
#include "vk_engine.h"

#include <common/constants.h>
#include <common/vk_types.h>
#include <engine/vk_images.h>
#include <engine/vk_initializers.h>
#include <engine/vk_pipelines.h>
#include <io/points.h>

// VMA needs to be included with VMA_IMPLEMENTATION definedin only one file
// Defining it includes the implementation of the included functions as well
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <nfd_sdl2.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <chrono>
#include <filesystem>
#include <thread>


vkEngine::VulkanEngine& vkEngine::VulkanEngine::getInstance() {
    if (loadedEngine == nullptr) { loadedEngine = new VulkanEngine(); }
    return *loadedEngine;
}

vkEngine::VulkanEngine::VulkanEngine()
: m_menu(m_config, m_nfdSdlWindowHandle) {
    // Initialize SDL and create a window with it
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { throw std::runtime_error("Could not initialise SDL"); }
    SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN;
    m_window = SDL_CreateWindow("vkpr",
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                m_windowExtent.width, m_windowExtent.height,
                                window_flags);
    
    // Initialise Native File Dialogue extended with this application's SDL window as the parent window
    if (NFD_Init() != NFD_OKAY) { throw std::runtime_error("Could not initialise NFDe"); };
    NFD_GetNativeWindowFromSDLWindow(m_window, &m_nfdSdlWindowHandle);

    // Initialize Vulkan resources
    initVulkan();
    initSwapchainAndDrawSurfaces();
    initCommands();
    initSyncStructures();
    initPointBuffers();
    initDescriptors();
    initPipelines();
    initImgui();

    // Everything went fine!
    m_isInitialized = true;
}

vkEngine::VulkanEngine::~VulkanEngine() {
    // Destroy initialised resources
    if (m_isInitialized) {
        // Wait until the device has finished all operations first
        vkDeviceWaitIdle(m_logicalDevice);

        // Per-frame resources
        for (vkUtils::DeletionQueue& frameResourceDeletor : m_frameResourceDeletors) { frameResourceDeletor.flush(); }

        // Global resources
        m_globalResourceDeletor.flush();
    }

    // Clear singleton
    loadedEngine = nullptr;
}

void vkEngine::VulkanEngine::initVulkan() {
    // Figure out if validation layers should be used
    #ifdef NDEBUG
    constexpr bool useValidationLayers = false;
    #else
    constexpr bool useValidationLayers = true;
    #endif

    // Construct application instance builder with optional debug features
    vkb::InstanceBuilder builder;
	auto instanceResult = builder
        .set_app_name("vkpr")
        .set_engine_name("MonkEngine")
		.request_validation_layers(useValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

	// Grab the instance and debug messenger 
    VKB_CHECK(instanceResult);
	vkb::Instance vkbInstance   = instanceResult.value();
	m_instance                  = vkbInstance.instance;
	m_debugMessenger            = vkbInstance.debug_messenger;

    // Create surface to render to
    SDL_Vulkan_CreateSurface(m_window, m_instance, &m_windowSurface);

    // Define required atomic features
    VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT imageAtomicFeatures = {};
    imageAtomicFeatures.sType                   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT;
    imageAtomicFeatures.shaderImageInt64Atomics = true;

	// Define desired Vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13 = {};
    features13.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = true;
	features13.synchronization2 = true;

	// Define desired Vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12 = {};
    features12.sType                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.bufferDeviceAddress      = true;
	features12.descriptorIndexing       = true;
    features12.timelineSemaphore        = true;
    features12.shaderBufferInt64Atomics = true;
    features12.shaderSharedInt64Atomics = true;

    // Define require core physical features
    VkPhysicalDeviceFeatures featuresCore = {};
    featuresCore.shaderInt64 = true;

	// Select a gpu that can write to the SDL surface and supports Vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector selector { vkbInstance };
    auto deviceSelectResult = selector
        .set_minimum_version(1, 3)
        .add_required_extension("VK_EXT_shader_image_atomic_int64")
        .add_required_extension_features(imageAtomicFeatures)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
        .set_required_features(featuresCore)
		.set_surface(m_windowSurface)
		.select();
    VKB_CHECK(deviceSelectResult);
    vkb::PhysicalDevice physicalDevice = deviceSelectResult.value();

	// Create physical and logical devices then acauire their handles
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    auto deviceBuildResult  = deviceBuilder.build();
    VKB_CHECK(deviceBuildResult);
	vkb::Device vkbDevice   = deviceBuildResult.value();
	m_physicalDevice        = physicalDevice.physical_device;
	m_logicalDevice         = vkbDevice.device;

    // Acquire dedicated compute, graphics, and transfer queues
    // Compute
    vkb::Result<VkQueue> computeQueueResult         = vkbDevice.get_queue(vkb::QueueType::compute);
    VKB_CHECK(computeQueueResult);
    m_computeQueue                                  = computeQueueResult.value();
    vkb::Result<uint32_t> computeQueueIdxResult     = vkbDevice.get_queue_index(vkb::QueueType::compute);
    VKB_CHECK(computeQueueIdxResult);
	m_computeQueueFamily                            = computeQueueIdxResult.value();
    // Graphics
    vkb::Result<VkQueue> graphicsQueueResult        = vkbDevice.get_queue(vkb::QueueType::graphics);
    VKB_CHECK(graphicsQueueResult);
    m_graphicsQueue                                 = graphicsQueueResult.value();
    vkb::Result<uint32_t> graphicsQueueIdxResult    = vkbDevice.get_queue_index(vkb::QueueType::graphics);
    VKB_CHECK(graphicsQueueIdxResult);
	m_graphicsQueueFamily                           = graphicsQueueIdxResult.value();
    // Transfer
    vkb::Result<VkQueue> transferQueueResult        = vkbDevice.get_queue(vkb::QueueType::transfer);
    VKB_CHECK(transferQueueResult);
    m_transferQueue                                 = transferQueueResult.value();
    vkb::Result<uint32_t> transferQueueIdxResult    = vkbDevice.get_queue_index(vkb::QueueType::transfer);
    VKB_CHECK(transferQueueIdxResult);
	m_transferQueueFamily                           = transferQueueIdxResult.value();

    // Initialise VMA allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice    = m_physicalDevice;
    allocatorInfo.device            = m_logicalDevice;
    allocatorInfo.instance          = m_instance;
    allocatorInfo.flags             = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // Allows for usage of GPU memory pointers
    vmaCreateAllocator(&allocatorInfo, &m_vmaAllocator);

    // Register created resources with global destruction queue
    m_globalResourceDeletor.pushFunction([&]() {
        // SDL window
        SDL_DestroyWindow(m_window);

        // Core Vulkan
        vkDestroyInstance(m_instance, nullptr);
        vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
        vkDestroyDevice(m_logicalDevice, nullptr);
        vkDestroySurfaceKHR(m_instance, m_windowSurface, nullptr);
        destroySwapchain();

        // VMA
        vmaDestroyAllocator(m_vmaAllocator);
    });
}

void vkEngine::VulkanEngine::initSwapchainAndDrawSurfaces() { 
    // Swapchain creation
    createSwapchain(m_windowExtent.width, m_windowExtent.height);

    // Common initialisation data
    VkExtent3D drawImageExtent = { m_windowExtent.width, m_windowExtent.height, 1};
    VmaAllocationCreateInfo imageAllocationCreateInfo = {};
	imageAllocationCreateInfo.usage         = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	imageAllocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // Packed data image
    m_packedDataImage.imageFormat = VK_FORMAT_R64_SINT;
	m_packedDataImage.imageExtent = drawImageExtent;
	VkImageUsageFlags packedDataImageUsages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	VkImageCreateInfo packedDataInfo = imageCreateInfo(m_packedDataImage.imageFormat, packedDataImageUsages, drawImageExtent);
    vmaCreateImage(m_vmaAllocator, &packedDataInfo, &imageAllocationCreateInfo, &m_packedDataImage.image, &m_packedDataImage.allocation, nullptr);
    VkImageViewCreateInfo packedDataViewInfo = imageViewCreateInfo(m_packedDataImage.imageFormat, m_packedDataImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
	VK_CHECK(vkCreateImageView(m_logicalDevice, &packedDataViewInfo, nullptr, &m_packedDataImage.imageView));
	
    // Render resolve image
    m_resolvedRenderImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    m_resolvedRenderImage.imageExtent = drawImageExtent;
    VkImageUsageFlags resolvedRenderImageUsages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    VkImageCreateInfo resolvedRenderInfo = imageCreateInfo(m_resolvedRenderImage.imageFormat, resolvedRenderImageUsages, drawImageExtent);
    vmaCreateImage(m_vmaAllocator, &resolvedRenderInfo, &imageAllocationCreateInfo, &m_resolvedRenderImage.image, &m_resolvedRenderImage.allocation, nullptr);
    VkImageViewCreateInfo resolvedRenderViewInfo = imageViewCreateInfo(m_resolvedRenderImage.imageFormat, m_resolvedRenderImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(m_logicalDevice, &resolvedRenderViewInfo, nullptr, &m_resolvedRenderImage.imageView));

    // Register created resources with global destruction queue
    m_globalResourceDeletor.pushFunction([&]() {
		vkDestroyImageView(m_logicalDevice, m_packedDataImage.imageView, nullptr);
		vmaDestroyImage(m_vmaAllocator, m_packedDataImage.image, m_packedDataImage.allocation);
        vkDestroyImageView(m_logicalDevice, m_resolvedRenderImage.imageView, nullptr);
		vmaDestroyImage(m_vmaAllocator, m_resolvedRenderImage.image, m_resolvedRenderImage.allocation);
	});
}

void vkEngine::VulkanEngine::initCommands() {
    // Create command pool for each queue. Allow allocated buffers to be reset after creation
    std::array<VkQueue*, NUM_QUEUES> queues                 = {&m_computeQueue, &m_graphicsQueue, &m_transferQueue};
    std::array<uint32_t, NUM_QUEUES> queueFamilies          = {m_computeQueueFamily, m_graphicsQueueFamily, m_transferQueueFamily};
    std::array<FrameDataCommandsArr*, NUM_QUEUES> commandResources  = {&m_framesCommandResourcesCompute, &m_framesCommandResourcesGraphics, &m_framesCommandResourcesTransfer}; 
    for (size_t queueTypeIdx = 0; queueTypeIdx < NUM_QUEUES; queueTypeIdx++) {
        VkQueue& queue                              = *queues[queueTypeIdx];
        uint32_t queueFamily                        = queueFamilies[queueTypeIdx];
        FrameDataCommandsArr& queueCommandResources = *commandResources[queueTypeIdx];
        VkCommandPoolCreateInfo commandPoolInfo     = commandPoolCreateInfo(queueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        for (uint32_t frame = 0; frame < FRAME_OVERLAP; frame++) {
            // Create a command pool
            VK_CHECK(vkCreateCommandPool(m_logicalDevice, &commandPoolInfo, nullptr, &queueCommandResources[frame].commandPool));

            // Allocate the default command buffer that we will use for rendering
            VkCommandBufferAllocateInfo cmdAllocInfo = commandBufferAllocateInfo(queueCommandResources[frame].commandPool, 1);
            VK_CHECK(vkAllocateCommandBuffers(m_logicalDevice, &cmdAllocInfo, &queueCommandResources[frame].mainCommandBuffer));

            // Register created command pool with global destruction queue
            // Buffers belonging to a pool are automatically destroyed upon its destruction
            m_globalResourceDeletor.pushFunction([&, frame]() { vkDestroyCommandPool(m_logicalDevice, queueCommandResources[frame].commandPool, nullptr); });
        }
    }

    // Allocate a pool and buffer for immediate commands and register pool destruction with deletion queue.
    // Pool and buffer use the graphics queue
    VkCommandPoolCreateInfo commandPoolInfo     = commandPoolCreateInfo(m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(m_logicalDevice, &commandPoolInfo, nullptr, &m_immediateCommandPool));
	VkCommandBufferAllocateInfo cmdAllocInfo    = vkEngine::commandBufferAllocateInfo(m_immediateCommandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(m_logicalDevice, &cmdAllocInfo, &m_immediateCommandBuffer));
	m_globalResourceDeletor.pushFunction([&]() { vkDestroyCommandPool(m_logicalDevice, m_immediateCommandPool, nullptr); });
}

void vkEngine::VulkanEngine::initSyncStructures() {
	// One fence to control when the gpu has finished rendering the frame
	// 2 semaphores to synchronize rendering with swapchain
	// The fence starts signalled so we can wait on it on the first frame
	VkFenceCreateInfo fenceInfoFrameData    = fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreInfo     = semaphoreCreateInfo();
    for (uint32_t frame = 0; frame < FRAME_OVERLAP; frame++) {
        // Acquire handle for current frame sync data
        FrameDataSynchronization& syncData = m_framesSynchronizationPrimitives[frame];

        // Create primitives
		VK_CHECK(vkCreateFence(m_logicalDevice, &fenceInfoFrameData, nullptr, &syncData.drawEnd));
		VK_CHECK(vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &syncData.swapchainToRender));
		VK_CHECK(vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &syncData.renderToPresent));

        // Register primitives with global destruction queue
        m_globalResourceDeletor.pushFunction([&, frame]() {
            vkDestroyFence(m_logicalDevice, syncData.drawEnd, nullptr);
            vkDestroySemaphore(m_logicalDevice, syncData.swapchainToRender, nullptr);
            vkDestroySemaphore(m_logicalDevice, syncData.renderToPresent, nullptr);    
        });
	}

    // Immediate submission fence creation and deletion queue registration
    VkFenceCreateInfo fenceInfoImmediate = fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(m_logicalDevice, &fenceInfoImmediate, nullptr, &m_immediateFence));
	m_globalResourceDeletor.pushFunction([&]() { vkDestroyFence(m_logicalDevice, m_immediateFence, nullptr); });
}

void vkEngine::VulkanEngine::initPointBuffers() {
    // Create buffers and register device-side buffer with global destruction queue
    createPointBuffer();
    m_globalResourceDeletor.pushFunction([&]() {
		vmaDestroyBuffer(m_vmaAllocator, m_pointsBuffer.buffer, m_pointsBuffer.allocation);
	});
}

void vkEngine::VulkanEngine::initDescriptors() {
	// Create a descriptor pool that will hold 1 set with 2 images and 1 buffer each
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  2},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};
	m_globalDescriptorAllocator.initPool(m_logicalDevice, 1, sizes);

	// Define the bindings of the descriptor set used by our compute shaders
	DescriptorLayoutBuilder builder;
    builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    builder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    builder.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    m_pointRenderDataDescriptorLayout = builder.build(m_logicalDevice, VK_SHADER_STAGE_COMPUTE_BIT);

    // Connect our descriptor set bindings to our point buffer and two draw images
    // Common
	m_pointRenderDataDescriptors = m_globalDescriptorAllocator.allocate(m_logicalDevice, m_pointRenderDataDescriptorLayout);	
	// Point buffer
    updateDescriptorPointBuffer();
    // Packed data image
    VkDescriptorImageInfo packedDataImgInfo = {};
	packedDataImgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	packedDataImgInfo.imageView   = m_packedDataImage.imageView;
	VkWriteDescriptorSet packedDataImageWrite = {};
	packedDataImageWrite.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	packedDataImageWrite.pNext              = nullptr;
	packedDataImageWrite.dstBinding         = 1;
	packedDataImageWrite.dstSet             = m_pointRenderDataDescriptors;
	packedDataImageWrite.descriptorCount    = 1;
	packedDataImageWrite.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	packedDataImageWrite.pImageInfo         = &packedDataImgInfo;
	vkUpdateDescriptorSets(m_logicalDevice, 1, &packedDataImageWrite, 0, nullptr);
    // Resolved render image
    VkDescriptorImageInfo resolvedRenderImgInfo = {};
	resolvedRenderImgInfo.imageLayout   = VK_IMAGE_LAYOUT_GENERAL;
	resolvedRenderImgInfo.imageView     = m_resolvedRenderImage.imageView;
	VkWriteDescriptorSet resolvedRenderImageWrite = {};
	resolvedRenderImageWrite.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	resolvedRenderImageWrite.pNext              = nullptr;
	resolvedRenderImageWrite.dstBinding         = 2;
	resolvedRenderImageWrite.dstSet             = m_pointRenderDataDescriptors;
	resolvedRenderImageWrite.descriptorCount    = 1;
	resolvedRenderImageWrite.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	resolvedRenderImageWrite.pImageInfo         = &resolvedRenderImgInfo;
	vkUpdateDescriptorSets(m_logicalDevice, 1, &resolvedRenderImageWrite, 0, nullptr);

	// Register the global descriptor allocator and created descriptor layout with global destruction queue 
	m_globalResourceDeletor.pushFunction([&]() {
		m_globalDescriptorAllocator.destroyPool(m_logicalDevice);
		vkDestroyDescriptorSetLayout(m_logicalDevice, m_pointRenderDataDescriptorLayout, nullptr);
	});
}

void vkEngine::VulkanEngine::initPipelines() {
    // Create default compute pipeline layout
    VkPipelineLayoutCreateInfo computeLayout = {};
	computeLayout.sType             = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext             = nullptr;
	computeLayout.pSetLayouts       = &m_pointRenderDataDescriptorLayout;
	computeLayout.setLayoutCount    = 1;
    // Define structure of push constants
    VkPushConstantRange pushConstant = {};
    pushConstant.offset                     = 0;
    pushConstant.size                       = sizeof(vkCommon::ComputePushConstants);
    pushConstant.stageFlags                 = VK_SHADER_STAGE_COMPUTE_BIT;
    computeLayout.pPushConstantRanges       = &pushConstant;
    computeLayout.pushConstantRangeCount    = 1;
	VK_CHECK(vkCreatePipelineLayout(m_logicalDevice, &computeLayout, nullptr, &m_computePipelineLayoutCommon));

    // Load shader code
    VkShaderModule rasterizePointsShader;
    std::filesystem::path rasterizePointsShaderPath = vkCommon::constants::SHADERS_DIR_PATH / "rasterize_points.comp.spv";
	if (!vkEngine::loadShaderModule(rasterizePointsShaderPath.string().c_str(), m_logicalDevice, &rasterizePointsShader)) {
		throw std::runtime_error("Error loading point rasterization compute shader");
	}
    VkShaderModule resolveRenderShader;
    std::filesystem::path resolveRenderShaderPath = vkCommon::constants::SHADERS_DIR_PATH / "resolve_render.comp.spv";
	if (!vkEngine::loadShaderModule(resolveRenderShaderPath.string().c_str(), m_logicalDevice, &resolveRenderShader)) {
		throw std::runtime_error("Error loading render resolve compute shader");
	}

    // Compute pipeline has only a single stage. We create it here and specify the shader to be used on a per effect basis 
	VkPipelineShaderStageCreateInfo stageinfo = {};
	stageinfo.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext     = nullptr;
	stageinfo.stage     = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.pName     = "main";

    // Define the layout of our pipelines
	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.sType     = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext     = nullptr;
	computePipelineCreateInfo.layout    = m_computePipelineLayoutCommon;
	computePipelineCreateInfo.stage     = stageinfo;

    // Create our pipelines and associated data (name and push constant data)
    // Point rasterization
    computePipelineCreateInfo.stage.module = rasterizePointsShader;
    VK_CHECK(vkCreateComputePipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_rasterizePoints));
    // Render resolve
    computePipelineCreateInfo.stage.module = resolveRenderShader;
    VK_CHECK(vkCreateComputePipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_resolveRender));

    // Destroy the loaded shaders (as they have already been embedded in their respective pipelines)
    // and register the created pipelines and their layouts with the global destruction queue
    vkDestroyShaderModule(m_logicalDevice, rasterizePointsShader, nullptr);
    vkDestroyShaderModule(m_logicalDevice, resolveRenderShader, nullptr);
	m_globalResourceDeletor.pushFunction([=]() {
		vkDestroyPipelineLayout(m_logicalDevice, m_computePipelineLayoutCommon, nullptr);
		vkDestroyPipeline(m_logicalDevice, m_rasterizePoints, nullptr);
        vkDestroyPipeline(m_logicalDevice, m_resolveRender, nullptr);
    });
}

void vkEngine::VulkanEngine::initImgui() {
    // Define descriptor pool sizes for all kinds of resources ImGUI might need. It's a bit oversized
    // TODO: Potential sources of bugs if enough UI elements are used and more resources are required
	VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 10 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10 } };

    // Create the descriptor pool
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags          = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets        = 10;
	poolInfo.poolSizeCount  = static_cast<uint32_t>(std::size(poolSizes));
	poolInfo.pPoolSizes     = poolSizes;
	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(m_logicalDevice, &poolInfo, nullptr, &imguiPool));

	// Init ImGUI
    // TODO: Maybe use a different queue for ImGUI submissions
	ImGui::CreateContext();                 // Init core structures
	ImGui_ImplSDL2_InitForVulkan(m_window); // Init SDL and Vulkan implementation
	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance       = m_instance;
	initInfo.PhysicalDevice = m_physicalDevice;
	initInfo.Device         = m_logicalDevice;
	initInfo.Queue          = m_graphicsQueue; // Use main graphics queue for command
	initInfo.DescriptorPool = imguiPool;
	initInfo.MinImageCount  = 3;
	initInfo.ImageCount     = 3;
	// Dynamic rendering usage and parameters
	initInfo.UseDynamicRendering                                    = true;
	initInfo.PipelineRenderingCreateInfo                            = {};
    initInfo.PipelineRenderingCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	initInfo.PipelineRenderingCreateInfo.colorAttachmentCount       = 1;
	initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats    = &m_swapchainImageFormat;
	initInfo.MSAASamples                                            = VK_SAMPLE_COUNT_1_BIT;
	ImGui_ImplVulkan_Init(&initInfo);
	ImGui_ImplVulkan_CreateFontsTexture();

	// Register ImGUI shutdown function and descriptor pool with destruction queue
	m_globalResourceDeletor.pushFunction([&]() {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(m_logicalDevice, imguiPool, nullptr);
	});
}

void vkEngine::VulkanEngine::createSwapchain(uint32_t width, uint32_t height) {
	// Build swapchain
    m_swapchainImageFormat      = VK_FORMAT_B8G8R8A8_UNORM;
    vkb::SwapchainBuilder swapchainBuilder{m_physicalDevice, m_logicalDevice, m_windowSurface};
	auto swapchainBuildResult   = swapchainBuilder
		.set_desired_format(VkSurfaceFormatKHR{.format = m_swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // TODO: Make adjustable
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();
    VKB_CHECK(swapchainBuildResult);

    // Acquire handle to swapchain info and resources
    vkb::Swapchain vkbSwapchain = swapchainBuildResult.value();
	m_swapchainExtent           = vkbSwapchain.extent;
	m_swapchain                 = vkbSwapchain.swapchain;
	m_swapchainImages           = vkbSwapchain.get_images().value();
	m_swapchainImageViews       = vkbSwapchain.get_image_views().value();
}

void vkEngine::VulkanEngine::destroySwapchain() {
	for (VkImageView& imageView : m_swapchainImageViews) { vkDestroyImageView(m_logicalDevice, imageView, nullptr); }
    vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);
}

vkEngine::FrameDataCommands& vkEngine::VulkanEngine::commandGetCurrentFrame(vkb::QueueType familyType) {
    switch (familyType) {
        case vkb::QueueType::compute:  { return m_framesCommandResourcesCompute[m_frameNumber % FRAME_OVERLAP]; } break;
        case vkb::QueueType::graphics: { return m_framesCommandResourcesGraphics[m_frameNumber % FRAME_OVERLAP]; } break;
        case vkb::QueueType::transfer: { return m_framesCommandResourcesTransfer[m_frameNumber % FRAME_OVERLAP]; } break;
        default: throw std::runtime_error("Attempted to acquire frame command resources of unsupported queue type");
    }
}

void vkEngine::VulkanEngine::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& commandRegistration) {
    // Reset immediate resources
    VK_CHECK(vkResetFences(m_logicalDevice, 1, &m_immediateFence));
	VK_CHECK(vkResetCommandBuffer(m_immediateCommandBuffer, 0));

    // Record immediate command buffer
	VkCommandBufferBeginInfo cmdBeginInfo = vkEngine::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(m_immediateCommandBuffer, &cmdBeginInfo));
	commandRegistration(m_immediateCommandBuffer);
	VK_CHECK(vkEndCommandBuffer(m_immediateCommandBuffer));

    // Submit immediate command buffer to graphics queue and wait for its submission fence
	VkCommandBufferSubmitInfo cmdInfo   = vkEngine::commandBufferSubmitInfo(m_immediateCommandBuffer);
	VkSubmitInfo2 submitInfo            = vkEngine::submitInfo(&cmdInfo, nullptr, nullptr);
	VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submitInfo, m_immediateFence));
	VK_CHECK(vkWaitForFences(m_logicalDevice, 1, &m_immediateFence, true, BLOCKING_TIMEOUT_NS));
}

void vkEngine::VulkanEngine::createPointBuffer() {
    // Load point cloud data
    std::vector<vkIo::Point> pointData = vkIo::readPointCloud(m_config.currentPointFile.string(), m_modelMin, m_modelMax);

    // Init buffers
    // Common
    m_pointsBuffer.size = m_pointsStagingBuffer.size = pointData.size();
    // Device buffer
    VkBufferCreateInfo devBuffCreateInfo = vkEngine::bufferExclusiveCreateInfo(m_pointsBuffer.sizeBytes(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VmaAllocationCreateInfo devBuffAllocCreateInfo = {};
	devBuffAllocCreateInfo.usage            = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	devBuffAllocCreateInfo.requiredFlags    = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    vmaCreateBuffer(m_vmaAllocator, &devBuffCreateInfo, &devBuffAllocCreateInfo, &m_pointsBuffer.buffer, &m_pointsBuffer.allocation, nullptr);
    // Host/Staging buffer
    VkBufferCreateInfo stagingBuffCreateInfo = vkEngine::bufferExclusiveCreateInfo(m_pointsStagingBuffer.sizeBytes(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    VmaAllocationCreateInfo stagingBuffAllocCreateInfo = {};
    stagingBuffAllocCreateInfo.usage            = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    stagingBuffAllocCreateInfo.flags            = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    stagingBuffAllocCreateInfo.requiredFlags    = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vmaCreateBuffer(m_vmaAllocator, &stagingBuffCreateInfo, &stagingBuffAllocCreateInfo, &m_pointsStagingBuffer.buffer, &m_pointsStagingBuffer.allocation, nullptr);

    // Copy point data to staging buffer
    vkIo::Point* pointsMapped;
    vmaMapMemory(m_vmaAllocator, m_pointsStagingBuffer.allocation, (void**) &pointsMapped);
    std::memcpy(pointsMapped, pointData.data(), m_pointsStagingBuffer.sizeBytes());
    vmaUnmapMemory(m_vmaAllocator, m_pointsStagingBuffer.allocation);
    vmaFlushAllocation(m_vmaAllocator, m_pointsStagingBuffer.allocation, 0U, VK_WHOLE_SIZE);

    // Copy point data from staging buffer to device memory buffer using immediate submit and wait for operation completion host-side
    VkBufferCopy stagingCopy    = {};
    stagingCopy.srcOffset       = 0;
    stagingCopy.dstOffset       = 0;
    stagingCopy.size            = m_pointsStagingBuffer.sizeBytes();
    VK_CHECK(vkResetFences(m_logicalDevice, 1, &m_immediateFence));
    VK_CHECK(vkResetCommandBuffer(m_immediateCommandBuffer, 0));
	VkCommandBufferBeginInfo cmdBeginInfo = commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); // This buffer will be submitted only once
	VK_CHECK(vkBeginCommandBuffer(m_immediateCommandBuffer, &cmdBeginInfo));
    vkCmdCopyBuffer(m_immediateCommandBuffer, m_pointsStagingBuffer.buffer, m_pointsBuffer.buffer, 1, &stagingCopy);
    VK_CHECK(vkEndCommandBuffer(m_immediateCommandBuffer));
    VkCommandBufferSubmitInfo cmdinfo   = commandBufferSubmitInfo(m_immediateCommandBuffer);	
	VkSubmitInfo2 submit                = submitInfo(&cmdinfo, nullptr, nullptr);	
    VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, m_immediateFence));
    VK_CHECK(vkWaitForFences(m_logicalDevice, 1, &m_immediateFence, true, vkIo::TRANSFER_TIMEOUT_NS));

    // Destroy staging buffer
    vmaDestroyBuffer(m_vmaAllocator, m_pointsStagingBuffer.buffer, m_pointsStagingBuffer.allocation);
}

void vkEngine::VulkanEngine::reloadPointData() {
    // Create new buffers, regiter it with descriptor set and flip reload flag
    vmaDestroyBuffer(m_vmaAllocator, m_pointsBuffer.buffer, m_pointsBuffer.allocation);
    createPointBuffer();
    updateDescriptorPointBuffer();
    m_config.loadNewFile = false;
}

void vkEngine::VulkanEngine::updateDescriptorPointBuffer() {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer   = m_pointsBuffer.buffer;
    bufferInfo.offset   = 0;
    bufferInfo.range    = VK_WHOLE_SIZE;
    VkWriteDescriptorSet pointBufferWrite = {};
    pointBufferWrite.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	pointBufferWrite.pNext              = nullptr;
	pointBufferWrite.dstBinding         = 0;
	pointBufferWrite.dstSet             = m_pointRenderDataDescriptors;
	pointBufferWrite.descriptorCount    = 1;
	pointBufferWrite.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pointBufferWrite.pBufferInfo        = &bufferInfo;
	vkUpdateDescriptorSets(m_logicalDevice, 1, &pointBufferWrite, 0, nullptr);
}

void vkEngine::VulkanEngine::draw() {
    // Acquire current frame command and sync primitives handles
    FrameDataCommands& currentFrameGraphicsCommandResources         = commandGetCurrentFrame(vkb::QueueType::graphics);
    FrameDataSynchronization& currentFrameSynchronizationPrimitives = syncGetCurrentFrame();
    VkCommandBuffer graphicsCmdBuff                                 = currentFrameGraphicsCommandResources.mainCommandBuffer;
    vkUtils::DeletionQueue& frameResourcesDeletor                   = resourceDeletorGetCurrentFrame(); 

    // Wait until the GPU has finished rendering the last frame and delet objects created for that specific frame
	VK_CHECK(vkWaitForFences(m_logicalDevice, 1, &currentFrameSynchronizationPrimitives.drawEnd, true, BLOCKING_TIMEOUT_NS));
	VK_CHECK(vkResetFences(m_logicalDevice, 1, &currentFrameSynchronizationPrimitives.drawEnd));
    frameResourcesDeletor.flush();

    // Request image from the swapchain
	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(m_logicalDevice, m_swapchain, BLOCKING_TIMEOUT_NS,
                                   currentFrameSynchronizationPrimitives.swapchainToRender, nullptr, &swapchainImageIndex));

    // Define drawing surface extent
    m_drawExtent.width  = m_packedDataImage.imageExtent.width;
	m_drawExtent.height = m_packedDataImage.imageExtent.height;

    // Begin graphics command buffer recording
    VK_CHECK(vkResetCommandBuffer(graphicsCmdBuff, 0));
	VkCommandBufferBeginInfo cmdBeginInfo = commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); // This buffer will be submitted only once
	VK_CHECK(vkBeginCommandBuffer(graphicsCmdBuff, &cmdBeginInfo));	

    // Main draw
    // TODO: Use more efficient barrier because transitionImage waits on EVERYTHING
    // Clear packed data image with maximum positive signed integer value
    transitionImage(graphicsCmdBuff, m_packedDataImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    drawClearRasterizationSurface(graphicsCmdBuff);
	// Draw points to packed data image
    transitionImage(graphicsCmdBuff, m_packedDataImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    drawPointRasterizer(graphicsCmdBuff);
    // Resolve packed data to colored image
    transitionImage(graphicsCmdBuff, m_packedDataImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    transitionImage(graphicsCmdBuff, m_resolvedRenderImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    drawPointColorUnpack(graphicsCmdBuff);
	// Transition the draw image and the swapchain image into transfer layouts
	transitionImage(graphicsCmdBuff, m_resolvedRenderImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	transitionImage(graphicsCmdBuff, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	// Execute a copy from the resolved render image to the swapchain image
	copyImageToImage(graphicsCmdBuff, m_resolvedRenderImage.image, m_swapchainImages[swapchainImageIndex], m_drawExtent, m_swapchainExtent);
    // Transition the swapchain image to Draw Optimal so it can be rendered to by ImGUI
    transitionImage(graphicsCmdBuff, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    // Draw ImGUI to the swapchain image
	drawImgui(graphicsCmdBuff,  m_swapchainImageViews[swapchainImageIndex]);
	// Set swapchain image layout to Present so we can show it on the screen
	transitionImage(graphicsCmdBuff, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// Finalize the command buffer
	VK_CHECK(vkEndCommandBuffer(graphicsCmdBuff));

    // Prepare the submission to the graphics queue 
	// We wait on swapchainToRender as that semaphore is signaled when the swapchain is ready
	// We signal renderToPresent to signal that rendering has finished
	VkCommandBufferSubmitInfo cmdinfo   = commandBufferSubmitInfo(graphicsCmdBuff);	
	VkSemaphoreSubmitInfo waitInfo      = semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, currentFrameSynchronizationPrimitives.swapchainToRender);
	VkSemaphoreSubmitInfo signalInfo    = semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, currentFrameSynchronizationPrimitives.renderToPresent);	
	VkSubmitInfo2 submit                = submitInfo(&cmdinfo, &signalInfo, &waitInfo);	

	// Submit command buffer to the graphics queue and execute it.
	// drawEnd will be signalled once the command buffer has finished execution
	VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, currentFrameSynchronizationPrimitives.drawEnd));

    // Present the image via the graphics queue once the command buffer has finished its work
    VkPresentInfoKHR presentInfo = {};
	presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext               = nullptr;
	presentInfo.pSwapchains         = &m_swapchain;
	presentInfo.swapchainCount      = 1;
	presentInfo.pWaitSemaphores     = &currentFrameSynchronizationPrimitives.renderToPresent;
	presentInfo.waitSemaphoreCount  = 1;
	presentInfo.pImageIndices       = &swapchainImageIndex;
	VK_CHECK(vkQueuePresentKHR(m_graphicsQueue, &presentInfo));

    // Increment current frame counter
    m_frameNumber++;
}

void vkEngine::VulkanEngine::drawClearRasterizationSurface(VkCommandBuffer graphicsCmdBuff) {
    VkClearColorValue clearValue = {};
    clearValue.int32[1] = 0x7FFFFFFF;
	clearValue.int32[0] = 0xFFFFFFFF;
	VkImageSubresourceRange clearRange  = imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(graphicsCmdBuff, m_packedDataImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &clearRange);
}

void vkEngine::VulkanEngine::drawPointRasterizer(VkCommandBuffer computeCmdBuff) {
    vkCommon::ComputePushConstants pushConstants = preparePushConstants();
    vkCmdBindPipeline(computeCmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_rasterizePoints);
	vkCmdBindDescriptorSets(computeCmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayoutCommon, 0, 1, &m_pointRenderDataDescriptors, 0, nullptr);
    vkCmdPushConstants(computeCmdBuff, m_computePipelineLayoutCommon, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(vkCommon::ComputePushConstants), &pushConstants);
    vkCmdDispatch(computeCmdBuff, static_cast<uint32_t>(std::ceil(m_pointsBuffer.size / 64.0)), 1, 1);
}

void vkEngine::VulkanEngine::drawPointColorUnpack(VkCommandBuffer computeCmdBuff) {
    vkCommon::ComputePushConstants pushConstants = preparePushConstants();
    vkCmdBindPipeline(computeCmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_resolveRender);
	vkCmdBindDescriptorSets(computeCmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayoutCommon, 0, 1, &m_pointRenderDataDescriptors, 0, nullptr);
    vkCmdPushConstants(computeCmdBuff, m_computePipelineLayoutCommon, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(vkCommon::ComputePushConstants), &pushConstants);
    vkCmdDispatch(computeCmdBuff,
                  static_cast<uint32_t>(std::ceil(m_windowExtent.width / 8.0)),
                  static_cast<uint32_t>(std::ceil(m_windowExtent.height / 8.0)),
                  1);
}

void vkEngine::VulkanEngine::drawImgui(VkCommandBuffer graphicsCmdBuff, VkImageView targetImageView) {
    VkRenderingAttachmentInfo colorAttachment   = vkEngine::attachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo                  = vkEngine::renderingInfo(m_swapchainExtent, &colorAttachment, nullptr);
	vkCmdBeginRendering(graphicsCmdBuff, &renderInfo);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), graphicsCmdBuff);
	vkCmdEndRendering(graphicsCmdBuff);
}

void vkEngine::VulkanEngine::run() {
    SDL_Event event;
    bool shouldRun = true;

    while (shouldRun) {
        // Handle events in SDL queue
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_QUIT: {
                    shouldRun = false;
                } break;
                case SDL_WINDOWEVENT: {
                    if (event.window.event == SDL_WINDOWEVENT_MINIMIZED)        { m_shouldRender = false; }
                    else if (event.window.event == SDL_WINDOWEVENT_RESTORED)    { m_shouldRender = true; }
                } break;
                case SDL_KEYUP: {
                    if (event.key.keysym.sym == SDLK_ESCAPE) { shouldRun = false; }
                } break;
            }

            // Send event to ImGUI for its processing
            ImGui_ImplSDL2_ProcessEvent(&event);
        }

        // Load new point file if requested from GUI
        if (m_config.loadNewFile) { reloadPointData(); }

        // Do not draw if we are minimized
        if (!m_shouldRender) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Throttle the speed to avoid the endless spinning
            continue;
        }

        // Make ImGUI prepare the structures needed for drawing
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        m_menu.draw();
        ImGui::Render();

        // Main draw entrypoint
        draw();
    }
}
