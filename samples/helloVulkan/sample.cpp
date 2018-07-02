﻿/*
* Vulkan Example - imGui (https://github.com/ocornut/imgui)
*
* Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "sample.h"
#include <saiga/imgui/imgui.h>


VulkanExample::VulkanExample(Saiga::Vulkan::SDLWindow &window, VulkanForwardRenderer &renderer)
 : renderer(renderer)
{
    float aspect = (float)window.width / (float)window.height;
    camera.setProj(60.0f,aspect,0.1f,50.0f);
    camera.setView(vec3(0,5,10),vec3(0,0,0),vec3(0,1,0));
}

VulkanExample::~VulkanExample()
{
}

void VulkanExample::init()
{

    assetRenderer.prepareUniformBuffers(renderer.vulkanDevice);

    assetRenderer.setupLayoutsAndDescriptors(renderer.device);
    assetRenderer.preparePipelines(renderer.device,renderer.pipelineCache,renderer.renderPass);


    teapot.load("objs/teapot.obj", renderer.vulkanDevice, renderer.queue);


}



void VulkanExample::update()
{
    assetRenderer.updateUniformBuffers(camera.view,camera.proj);
    camera.update(1.0/60);
    camera.interpolate(1.0/60,0);
}


void VulkanExample::render(VkCommandBuffer cmd)
{
    assetRenderer.bind(cmd);
    if(displayModels)
        teapot.render(cmd);
}

void VulkanExample::renderGUI()
{

    ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Example settings");
    ImGui::Checkbox("Render models", &displayModels);
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
    ImGui::ShowTestWindow();
}
