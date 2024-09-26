// vd_vk.h - A C vulkan helper library
// 
// -------------------------------------------------------------------------------------------------
// MIT License
// 
// Copyright (c) 2024 Michael Dodis
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#if defined(__INTELLISENSE__) || defined(__CLANGD__)
#define VD_VK_IMPLEMENTATION
#endif
#ifndef VD_VK_H
#define VD_VK_H

#ifdef VD_VK_OPTION_INCLUDE_VULKAN_CUSTOM
#define VD_VK_INCLUDE_VULKAN 0
#endif

#ifndef VD_VK_INCLUDE_VULKAN
#define VD_VK_INCLUDE_VULKAN 1
#endif

#ifndef VD_VK_CUSTOM_CHECK
#define VD_VK_CUSTOM_CHECK(x) x
#endif

#if VD_VK_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#endif

/**
 * @brief Transition an image from one layout to another
 * @param cmd        The command buffer
 * @param image      The image to transition
 * @param old_layout The old layout
 * @param new_layout The new layout
 */
void vd_vk_image_transition(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout);

void vd_vk_image_copy(
    VkCommandBuffer cmd,
    VkImage src_image,
    VkImage dst_image,
    VkExtent2D src_size,
    VkExtent2D dst_size);

static inline VkImageSubresourceRange vd_vk_subresource_range(VkImageAspectFlags aspect_mask)
{
    return (VkImageSubresourceRange)
    {
        .aspectMask     = aspect_mask,
        .baseMipLevel   = 0,
        .levelCount     = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount     = VK_REMAINING_ARRAY_LAYERS,
    };
}

#ifdef VD_VK_IMPLEMENTATION
void vd_vk_image_transition(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout)
{
    vkCmdPipelineBarrier2(
        cmd,
        & (VkDependencyInfo)
        {
            .sType                          = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount        = 1,
            .pImageMemoryBarriers = & (VkImageMemoryBarrier2)
            {
                .sType                      = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .image                      = image,
                .srcStageMask               = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                .srcAccessMask              = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .dstStageMask               = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                .dstAccessMask              = VK_ACCESS_2_MEMORY_READ_BIT | 
                                              VK_ACCESS_2_MEMORY_READ_BIT,
                .oldLayout                  = old_layout,
                .newLayout                  = new_layout,
                .subresourceRange           = (VkImageSubresourceRange)
                {
                    .aspectMask             = new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL 
                                                        ? VK_IMAGE_ASPECT_DEPTH_BIT
                                                        : VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel           = 0,
                    .levelCount             = VK_REMAINING_MIP_LEVELS,
                    .baseArrayLayer         = 0,
                    .layerCount             = VK_REMAINING_ARRAY_LAYERS,
                },
            },
        });
}

void vd_vk_image_copy(
    VkCommandBuffer cmd,
    VkImage src_image,
    VkImage dst_image,
    VkExtent2D src_size,
    VkExtent2D dst_size)
{
    vkCmdBlitImage2(
        cmd,
        & (VkBlitImageInfo2)
        {
            .sType                      = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
            .srcImage                   = src_image,
            .srcImageLayout             = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .dstImage                   = dst_image,
            .dstImageLayout             = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .filter                     = VK_FILTER_LINEAR,
            .regionCount                = 1,
            .pRegions = & (VkImageBlit2)
            {
                .sType                  = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                .srcOffsets =
                {
                    {0, 0, 0},
                    {src_size.width, src_size.height, 1},
                },
                .srcSubresource = (VkImageSubresourceLayers)
                {
                    .aspectMask         = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel           = 0,
                    .baseArrayLayer     = 0,
                    .layerCount         = 1,
                },
                .dstOffsets =
                {
                    {0, 0, 0},
                    {dst_size.width, dst_size.height, 1},
                },
                .dstSubresource = (VkImageSubresourceLayers)
                {
                    .aspectMask         = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel           = 0,
                    .baseArrayLayer     = 0,
                    .layerCount         = 1,
                },
            }
        });
}

#endif // VD_VK_IMPLEMENTATION
#endif // !VD_VK_H
