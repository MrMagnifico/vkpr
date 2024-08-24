#include "vk_initializers.h"

//> init_cmd
VkCommandPoolCreateInfo vkEngine::commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo info = {};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext              = nullptr;
    info.queueFamilyIndex   = queueFamilyIndex;
    info.flags              = flags;
    return info;
}

VkCommandBufferAllocateInfo vkEngine::commandBufferAllocateInfo(VkCommandPool pool, uint32_t count) {
    VkCommandBufferAllocateInfo info = {};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext              = nullptr;
    info.commandPool        = pool;
    info.commandBufferCount = count;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    return info;
}

VkCommandBufferBeginInfo vkEngine::commandBufferBeginInfo(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo info = {};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext              = nullptr;
    info.pInheritanceInfo   = nullptr;
    info.flags              = flags;
    return info;
}

VkFenceCreateInfo vkEngine::fenceCreateInfo(VkFenceCreateFlags flags) {
    VkFenceCreateInfo info = {};
    info.sType  = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext  = nullptr;
    info.flags  = flags;
    return info;
}

VkSemaphoreCreateInfo vkEngine::semaphoreCreateInfo(VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo info = {};
    info.sType  = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext  = nullptr;
    info.flags  = flags;
    return info;
}

VkSemaphoreSubmitInfo vkEngine::semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {
	VkSemaphoreSubmitInfo submitInfo = {};
	submitInfo.sType        = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	submitInfo.pNext        = nullptr;
	submitInfo.semaphore    = semaphore;
	submitInfo.stageMask    = stageMask;
	submitInfo.deviceIndex  = 0;
	submitInfo.value        = 1;
	return submitInfo;
}

VkCommandBufferSubmitInfo vkEngine::commandBufferSubmitInfo(VkCommandBuffer cmd) {
	VkCommandBufferSubmitInfo info = {};
	info.sType          = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	info.pNext          = nullptr;
	info.commandBuffer  = cmd;
	info.deviceMask     = 0;
	return info;
}

VkSubmitInfo2 vkEngine::submitInfo(VkCommandBufferSubmitInfo* cmd,
                                    VkSemaphoreSubmitInfo* signalSemaphoreInfo,
                                    VkSemaphoreSubmitInfo* waitSemaphoreInfo) {
    VkSubmitInfo2 info = {};
    info.sType                      = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.pNext                      = nullptr;
    info.waitSemaphoreInfoCount     = waitSemaphoreInfo == nullptr ? 0 : 1;
    info.pWaitSemaphoreInfos        = waitSemaphoreInfo;
    info.signalSemaphoreInfoCount   = signalSemaphoreInfo == nullptr ? 0 : 1;
    info.pSignalSemaphoreInfos      = signalSemaphoreInfo;
    info.commandBufferInfoCount     = 1;
    info.pCommandBufferInfos        = cmd;
    return info;
}

VkPresentInfoKHR vkEngine::presentInfo() {
    VkPresentInfoKHR info = {};
    info.sType              =  VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.pNext              = 0;
    info.swapchainCount     = 0;
    info.pSwapchains        = nullptr;
    info.pWaitSemaphores    = nullptr;
    info.waitSemaphoreCount = 0;
    info.pImageIndices      = nullptr;
    return info;
}

//> color_info
VkRenderingAttachmentInfo vkEngine::attachmentInfo(VkImageView view, VkClearValue* clear ,VkImageLayout layout) {
    VkRenderingAttachmentInfo colorAttachment = {};
    colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.pNext       = nullptr;
    colorAttachment.imageView   = view;
    colorAttachment.imageLayout = layout;
    colorAttachment.loadOp      = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    if (clear) { colorAttachment.clearValue = *clear; }
    return colorAttachment;
}

VkRenderingAttachmentInfo vkEngine::depthAttachmentInfo(VkImageView view, VkImageLayout layout) {
    VkRenderingAttachmentInfo depthAttachment = {};
    depthAttachment.sType                           = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.pNext                           = nullptr;
    depthAttachment.imageView                       = view;
    depthAttachment.imageLayout                     = layout;
    depthAttachment.loadOp                          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp                         = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil.depth   = 0.f;
    return depthAttachment;
}

VkRenderingInfo vkEngine::renderingInfo(VkExtent2D renderExtent,
                                         VkRenderingAttachmentInfo* colorAttachment,
                                         VkRenderingAttachmentInfo* depthAttachment) {
    VkRenderingInfo renderInfo = {};
    renderInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.pNext                = nullptr;
    renderInfo.renderArea           = VkRect2D {VkOffset2D {0, 0}, renderExtent};
    renderInfo.layerCount           = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments    = colorAttachment;
    renderInfo.pDepthAttachment     = depthAttachment;
    renderInfo.pStencilAttachment   = nullptr;
    return renderInfo;
}

VkImageSubresourceRange vkEngine::imageSubresourceRange(VkImageAspectFlags aspectMask) {
    VkImageSubresourceRange subImage = {};
    subImage.aspectMask     = aspectMask;
    subImage.baseMipLevel   = 0;
    subImage.levelCount     = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    return subImage;
}

VkDescriptorSetLayoutBinding vkEngine::descriptorSetLayoutBinding(VkDescriptorType type,
                                                                    VkShaderStageFlags stageFlags,
                                                                    uint32_t binding) {
    VkDescriptorSetLayoutBinding setbind = {};
    setbind.binding             = binding;
    setbind.descriptorCount     = 1;
    setbind.descriptorType      = type;
    setbind.pImmutableSamplers  = nullptr;
    setbind.stageFlags          = stageFlags;
    return setbind;
}

VkDescriptorSetLayoutCreateInfo vkEngine::descriptorSetLayoutCreateInfo(VkDescriptorSetLayoutBinding* bindings,
                                                                           uint32_t bindingCount) {
    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext          = nullptr;
    info.pBindings      = bindings;
    info.bindingCount   = bindingCount;
    info.flags          = 0;
    return info;
}

VkWriteDescriptorSet vkEngine::writeDescriptorImage(VkDescriptorType type,
                                                      VkDescriptorSet dstSet,
                                                      VkDescriptorImageInfo* imageInfo,
                                                      uint32_t binding) {
    VkWriteDescriptorSet write = {};
    write.sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext             = nullptr;
    write.dstBinding        = binding;
    write.dstSet            = dstSet;
    write.descriptorCount   = 1;
    write.descriptorType    = type;
    write.pImageInfo        = imageInfo;
    return write;
}

VkWriteDescriptorSet vkEngine::writeDescriptorBuffer(VkDescriptorType type,
                                                       VkDescriptorSet dstSet,
                                                       VkDescriptorBufferInfo* bufferInfo,
                                                       uint32_t binding) {
    VkWriteDescriptorSet write = {};
    write.sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext             = nullptr;
    write.dstBinding        = binding;
    write.dstSet            = dstSet;
    write.descriptorCount   = 1;
    write.descriptorType    = type;
    write.pBufferInfo       = bufferInfo;
    return write;
}

VkDescriptorBufferInfo vkEngine::bufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {
    VkDescriptorBufferInfo binfo = {};
    binfo.buffer    = buffer;
    binfo.offset    = offset;
    binfo.range     = range;
    return binfo;
}

VkBufferCreateInfo vkEngine::bufferConcurrentCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, std::vector<uint32_t> queueFamilyIndices,
                                                        VkBufferCreateFlags flags) {
    VkBufferCreateInfo binfo = {};
    binfo.sType         = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    binfo.flags         = flags;
    binfo.size          = size;
    binfo.usage         = usage;
    binfo.sharingMode   = VK_SHARING_MODE_CONCURRENT;
    binfo.queueFamilyIndexCount = queueFamilyIndices.size();
    binfo.pQueueFamilyIndices   = queueFamilyIndices.data();
    return binfo;
}

VkBufferCreateInfo vkEngine::bufferExclusiveCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkBufferCreateFlags flags) {
    VkBufferCreateInfo binfo = {};
    binfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    binfo.flags                 = flags;
    binfo.size                  = size;
    binfo.usage                 = usage;
    binfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    return binfo;
}

VkImageCreateInfo vkEngine::imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent) {
    VkImageCreateInfo info = {};
    info.sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext          = nullptr;
    info.imageType      = VK_IMAGE_TYPE_2D;
    info.format         = format;
    info.extent         = extent;
    info.mipLevels      = 1;
    info.arrayLayers    = 1;
    info.samples        = VK_SAMPLE_COUNT_1_BIT;    // For MSAA. we will not be using it by default, so default it to 1 sample per pixel.
    info.tiling         = VK_IMAGE_TILING_OPTIMAL;  // Optimal tiling, which means the image is stored on the best GPU format
    info.usage          = usageFlags;
    return info;
}

VkImageViewCreateInfo vkEngine::imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo info = {};
    info.sType                              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext                              = nullptr;
    info.viewType                           = VK_IMAGE_VIEW_TYPE_2D;
    info.image                              = image;
    info.format                             = format;
    info.subresourceRange.baseMipLevel      = 0;
    info.subresourceRange.levelCount        = 1;
    info.subresourceRange.baseArrayLayer    = 0;
    info.subresourceRange.layerCount        = 1;
    info.subresourceRange.aspectMask        = aspectFlags;
    return info;
}

VkPipelineLayoutCreateInfo vkEngine::pipelineLayoutCreateInfo() {
    VkPipelineLayoutCreateInfo info = {};
    info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext                  = nullptr;
    info.flags                  = 0;
    info.setLayoutCount         = 0;
    info.pSetLayouts            = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges    = nullptr;
    return info;
}

VkPipelineShaderStageCreateInfo vkEngine::pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
                                                                            VkShaderModule shaderModule,
                                                                            const char * entry) {
    VkPipelineShaderStageCreateInfo info = {};
    info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext  = nullptr;
    info.stage  = stage;
    info.module = shaderModule;
    info.pName  = entry;
    return info;
}
