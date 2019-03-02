﻿/*
 * Vulkan Example base class
 *
 * Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "saiga/core/util/assert.h"
#include "saiga/core/util/math.h"
#include "saiga/vulkan/CommandPool.h"
#include "saiga/vulkan/FrameSync.h"
#include "saiga/vulkan/Queue.h"
#include "saiga/vulkan/Renderer.h"
#include "saiga/vulkan/buffer/DepthBuffer.h"
#include "saiga/vulkan/buffer/Framebuffer.h"
#include "saiga/vulkan/window/Window.h"


namespace Saiga
{
namespace Vulkan
{
class SAIGA_VULKAN_API VulkanForwardRenderingInterface : public RenderingInterfaceBase
{
   public:
    VulkanForwardRenderingInterface(RendererBase& parent) : RenderingInterfaceBase(parent) {}
    virtual ~VulkanForwardRenderingInterface() {}

    virtual void init(Saiga::Vulkan::VulkanBase& base) = 0;
    virtual void transfer(vk::CommandBuffer cmd) {}
    virtual void render(vk::CommandBuffer cmd) {}
    virtual void renderGUI() {}
};



class SAIGA_VULKAN_API VulkanForwardRenderer : public Saiga::Vulkan::VulkanRenderer
{
   public:
    // Queue graphicsQueue;
    //    Queue presentQueue;
    //    Queue transferQueue;

    CommandPool renderCommandPool;
    VkRenderPass renderPass;

    VulkanForwardRenderer(Saiga::Vulkan::VulkanWindow& window, VulkanParameters vulkanParameters);
    virtual ~VulkanForwardRenderer() override;

    void initChildren();
    virtual void render2(FrameSync& sync, int currentImage) override;
    //    virtual void render(Camera* cam) override;



    virtual void createFrameBuffers(int numImages, int w, int h) override;
    virtual void createDepthBuffer(int w, int h) override;
    virtual void setupRenderPass() override;

   protected:
    int maxFramesInFlight = 10;
    DepthBuffer depthBuffer;

    //    CommandPool cmdPool;

    std::vector<vk::CommandBuffer> drawCmdBuffers;
    std::vector<Framebuffer> frameBuffers;



    std::shared_ptr<ImGuiVulkanRenderer> imGui;
};

}  // namespace Vulkan
}  // namespace Saiga
