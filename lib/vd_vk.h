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

typedef struct {
    VkPipelineShaderStageCreateInfo             *stages;
    int                                         num_stages;
    VkPrimitiveTopology                         topology;
    VkPolygonMode                               polygon_mode;
    VkCullModeFlags                             cull_mode;
    VkFrontFace                                 front_face;

    struct {
        int                                     on;
    } multisample;

    struct {
        int                                     on;
    } blend;

    struct {
        int                                     on;
        int                                     write;
        VkCompareOp                             cmp_op;
    } depth_test;

    VkFormat                                    color_format;
    VkFormat                                    depth_format;
    VkPipelineLayout                            layout;
} VD_VK_PipelineBuildInfo;

VkResult vd_vk_build_pipeline(
    VkDevice device,
    VD_VK_PipelineBuildInfo *info,
    VkPipeline *out_pipeline);

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

/**
 * @brief Copy an image to another
 * @param cmd       The command buffer
 * @param src_image The source image
 * @param dst_image The destination image
 * @param src_size  The source size
 * @param dst_size  The destination size
 */
void vd_vk_image_copy(
    VkCommandBuffer cmd,
    VkImage src_image,
    VkImage dst_image,
    VkExtent2D src_size,
    VkExtent2D dst_size);

VkResult vd_vk_create_shader_module(
    VkDevice device,
    void *code,
    size_t code_size, 
    VkShaderModule *out_module);

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

VkResult vd_vk_build_pipeline(
    VkDevice device,
    VD_VK_PipelineBuildInfo *info,
    VkPipeline *out_pipeline)
{
    VkPipelineInputAssemblyStateCreateInfo      input_assembly = {0};
    VkPipelineRasterizationStateCreateInfo      rasterization = {0};
    VkPipelineColorBlendAttachmentState         color_blend_attachments = {0};
    VkPipelineMultisampleStateCreateInfo        multisample = {0};
    VkPipelineDepthStencilStateCreateInfo       depth_stencil = {0};

    for (int i = 0; i < info->num_stages; ++i) {
        info->stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    }

    input_assembly.sType  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    rasterization.sType   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    multisample.sType     = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    depth_stencil.sType   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    input_assembly.topology = info->topology;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    rasterization.polygonMode = info->polygon_mode;
    rasterization.lineWidth = 1.0f;

    rasterization.cullMode = info->cull_mode;
    rasterization.frontFace = info->front_face;

    if (info->multisample.on) {
        multisample.sampleShadingEnable = VK_TRUE;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample.minSampleShading = 1.0f;
        multisample.pSampleMask = 0;
        multisample.alphaToCoverageEnable = VK_FALSE;
        multisample.alphaToOneEnable = VK_FALSE;
    } else {
        multisample.sampleShadingEnable = VK_FALSE;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample.minSampleShading = 1.0f;
        multisample.pSampleMask = 0;
        multisample.alphaToCoverageEnable = VK_FALSE;
        multisample.alphaToOneEnable = VK_FALSE;
    }

    color_blend_attachments.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    if (info->blend.on) {
        color_blend_attachments.blendEnable = VK_TRUE;
    } else {
        color_blend_attachments.blendEnable = VK_FALSE;
    }

    if (info->depth_test.on) {
        depth_stencil.depthTestEnable = VK_TRUE;
        depth_stencil.depthWriteEnable = info->depth_test.write;
        depth_stencil.depthCompareOp = info->depth_test.cmp_op;
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.stencilTestEnable = VK_FALSE;
        depth_stencil.minDepthBounds = 0.0f;
        depth_stencil.maxDepthBounds = 1.0f;
    } else {
        depth_stencil.depthTestEnable = VK_FALSE;
        depth_stencil.depthWriteEnable = VK_FALSE;
        depth_stencil.depthCompareOp = VK_COMPARE_OP_NEVER;
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.stencilTestEnable = VK_FALSE;
        depth_stencil.minDepthBounds = 0.0f;
        depth_stencil.maxDepthBounds = 1.0f;
    }

    VkPipelineViewportStateCreateInfo viewport = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount  = 1,
        .scissorCount   = 1,
    };

    VkPipelineColorBlendStateCreateInfo color_blend = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments   = &color_blend_attachments,
        .logicOpEnable  = VK_FALSE,
        .logicOp        = VK_LOGIC_OP_COPY,
    };

    return vkCreateGraphicsPipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &(VkGraphicsPipelineCreateInfo)
        {
            .sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount             = info->num_stages,
            .pStages                = info->stages,
            .pVertexInputState      = & (VkPipelineVertexInputStateCreateInfo)
            {
                .sType              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            },
            .pInputAssemblyState    = &input_assembly,
            .pViewportState         = &viewport,
            .pRasterizationState    = &rasterization,
            .pMultisampleState      = &multisample,
            .pColorBlendState       = &color_blend,
            .pDepthStencilState     = &depth_stencil,
            .layout                 = info->layout,
            .pDynamicState          = & (VkPipelineDynamicStateCreateInfo)
            {
                .sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount  = 2,
                .pDynamicStates     = (VkDynamicState[])
                {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR,
                },
            },
            .pNext = & (VkPipelineRenderingCreateInfo)
            {
                .sType                      = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount       = 1,
                .pColorAttachmentFormats    = &info->color_format,
                .depthAttachmentFormat      = info->depth_format,
            }
        },
        0,
        out_pipeline);
}

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
                .dstAccessMask              = VK_ACCESS_2_MEMORY_WRITE_BIT | 
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

VkResult vd_vk_create_shader_module(
    VkDevice device,
    void *code,
    size_t code_size, 
    VkShaderModule *out_module)
{
    return vkCreateShaderModule(
        device,
        &(VkShaderModuleCreateInfo)
        {
            .sType      = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize   = code_size,
            .pCode      = code,
        },
        0,
        out_module);
}

#endif // VD_VK_IMPLEMENTATION
#endif // !VD_VK_H
