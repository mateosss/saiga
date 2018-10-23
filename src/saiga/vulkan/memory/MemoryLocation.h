//
// Created by Peter Eichinger on 08.10.18.
//

#pragma once
#include <vulkan/vulkan.hpp>
#include <saiga/export.h>
namespace Saiga{
namespace Vulkan{
namespace Memory{

struct SAIGA_GLOBAL MemoryLocation {
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    vk::DeviceSize offset;
    vk::DeviceSize size;

    MemoryLocation() : buffer(nullptr), memory(nullptr), offset(0), size(0){}
    MemoryLocation(vk::Buffer _buffer, vk::DeviceMemory _memory, vk::DeviceSize _offset, vk::DeviceSize _size = 0) :
        buffer(_buffer), memory(_memory), offset(_offset), size(_size) { }

    void mappedUpload(vk::Device device, const void* data ){
        void* target = device.mapMemory(memory, offset,size);
        std::memcpy(target, data, size);
        device.unmapMemory(memory);
    }

    void mappedDownload(vk::Device device, void* data) {
        void* target = device.mapMemory(memory, offset,size);
        std::memcpy(data, target, size);
        device.unmapMemory(memory);
    }

    void* map(vk::Device device) {
        return device.mapMemory(memory,offset, size);
    }

    void unmap(vk::Device device) {
        device.unmapMemory(memory);
    }

    void destroy(const vk::Device& device) {
        device.destroy(buffer);
        device.free(memory);
    }

    bool operator==(const MemoryLocation &rhs) const {
        return std::tie(buffer, memory, offset, size) == std::tie(rhs.buffer, rhs.memory, rhs.offset, rhs.size);
    }

    bool operator!=(const MemoryLocation &rhs) const {
        return !(rhs == *this);
    }
};

}
}
}
