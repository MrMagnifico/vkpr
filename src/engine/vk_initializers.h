// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <common/vk_types.h>

namespace vkEngine {
// Command pools and buffers
VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = {});
VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1);
VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = {});
VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd);

// Synchronization primitives
VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = {});
VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = {});

// Swapchain
VkSubmitInfo2 submitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);
VkPresentInfoKHR presentInfo();

// Render attachment and info
VkRenderingAttachmentInfo attachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
VkRenderingAttachmentInfo depthAttachmentInfo(VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
VkRenderingInfo renderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment);

// Image subresource
VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask);

// Commands
VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

// Descriptor sets
VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount);
VkWriteDescriptorSet writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);
VkWriteDescriptorSet writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
VkDescriptorBufferInfo bufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

// Buffer creation
VkBufferCreateInfo bufferConcurrentCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, std::vector<uint32_t> queueFamilyIndices,
                                              VkBufferCreateFlags flags = {});
VkBufferCreateInfo bufferExclusiveCreateInfo(VkDeviceSize size, VkBufferUsageFlags usages, VkBufferCreateFlags flags = {});

// Image creation
VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

// Pipeline creation
VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo();
VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry = "main");
}
