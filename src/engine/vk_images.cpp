#include "vk_images.h"

#include <engine/vk_initializers.h>


void vkEngine::transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout) {
    // TODO: This is "the simplest way" according to vk-guide. Research other ways

    // Define transition barrier controlling layout changes and the operations to be paused and allowed after the barrier
    VkImageMemoryBarrier2 imageBarrier {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    imageBarrier.pNext          = nullptr;
    imageBarrier.srcStageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // TODO: Not this
    imageBarrier.srcAccessMask  = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // TODO: Not this
    imageBarrier.dstAccessMask  = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
    imageBarrier.oldLayout      = currentLayout;
    imageBarrier.newLayout      = newLayout;

    // Define components and range of the image that will be operated on
    VkImageAspectFlags aspectMask   = newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ?
                                      VK_IMAGE_ASPECT_DEPTH_BIT                             :
                                      VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange   = imageSubresourceRange(aspectMask);
    imageBarrier.image              = image;

    // Pack barrier into a dependency struct for command buffer submission
    VkDependencyInfo depInfo {};
    depInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext                   = nullptr;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers    = &imageBarrier;

    // Record image transition barrier into command buffer
    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void vkEngine::copyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize) {
    // Define which parts to copy from and transfer to
    VkImageBlit2 blitRegion = {};
    blitRegion.sType                            = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
    blitRegion.pNext                            = nullptr;
	blitRegion.srcOffsets[1].x                  = srcSize.width;
	blitRegion.srcOffsets[1].y                  = srcSize.height;
	blitRegion.srcOffsets[1].z                  = 1;
	blitRegion.dstOffsets[1].x                  = dstSize.width;
	blitRegion.dstOffsets[1].y                  = dstSize.height;
	blitRegion.dstOffsets[1].z                  = 1;
	blitRegion.srcSubresource.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer    = 0;
	blitRegion.srcSubresource.layerCount        = 1;
	blitRegion.srcSubresource.mipLevel          = 0;
	blitRegion.dstSubresource.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer    = 0;
	blitRegion.dstSubresource.layerCount        = 1;
	blitRegion.dstSubresource.mipLevel          = 0;

    // Connect blit region to images 
	VkBlitImageInfo2 blitInfo = {};
    blitInfo.sType          = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blitInfo.pNext          = nullptr;
	blitInfo.dstImage       = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage       = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter         = VK_FILTER_NEAREST;
	blitInfo.regionCount    = 1;
	blitInfo.pRegions       = &blitRegion;

    // Record image blit command to command buffer
    vkCmdBlitImage2(cmd, &blitInfo);
}
