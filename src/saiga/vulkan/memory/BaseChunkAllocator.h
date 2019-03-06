//
// Created by Peter Eichinger on 30.10.18.
//

#pragma once


#include "saiga/core/imgui/imgui.h"
#include "saiga/core/util/tostring.h"
#include "saiga/vulkan/Queue.h"
#include "saiga/vulkan/memory/MemoryStats.h"

#include "ChunkCreator.h"
#include "FitStrategy.h"
#include "MemoryStats.h"

#include <mutex>

namespace Saiga::Vulkan::Memory
{
template <typename T>
class SAIGA_VULKAN_API BaseChunkAllocator
{
   protected:
    std::mutex allocationMutex;
    void findNewMax(ChunkIterator<T>& chunkAlloc) const;
    vk::Device m_device;
    ChunkCreator* m_chunkAllocator{};

   public:
    FitStrategy<T>* strategy{};
    Queue* queue;

    vk::DeviceSize m_chunkSize{};
    vk::DeviceSize m_allocateSize{};
    ChunkContainer<T> chunks;

   protected:
    std::string gui_identifier;

    virtual ChunkIterator<T> createNewChunk() = 0;

    virtual void headerInfo() {}


    /**
     * Allocates \p size bytes
     * @tparam T Type of location to allocate
     * @param size Number of bytes to allocate
     * @remarks Function is not synchronized with a mutex. This must be done by the calling method.
     * @return A pointer to the allocated memory region. Data will not be set.
     */
    virtual T* base_allocate(vk::DeviceSize size);

    virtual std::unique_ptr<T> create_location(ChunkIterator<T>& chunk_alloc, vk::DeviceSize start,
                                               vk::DeviceSize size) = 0;

   public:
    virtual void deallocate(T* location);
    BaseChunkAllocator(vk::Device _device, ChunkCreator* chunkAllocator, FitStrategy<T>& strategy, Queue* _queue,
                       vk::DeviceSize chunkSize = 64 * 1024 * 1024)
        : m_device(_device),
          m_chunkAllocator(chunkAllocator),
          strategy(&strategy),
          queue(_queue),
          m_chunkSize(chunkSize),
          m_allocateSize(chunkSize),
          gui_identifier("")
    {
    }

    BaseChunkAllocator(BaseChunkAllocator&& other) noexcept
        : m_device(other.m_device),
          m_chunkAllocator(other.m_chunkAllocator),
          strategy(other.strategy),
          queue(other.queue),
          m_chunkSize(other.m_chunkSize),
          m_allocateSize(other.m_allocateSize),
          chunks(std::move(other.chunks)),
          gui_identifier(std::move(other.gui_identifier))
    {
    }

    BaseChunkAllocator& operator=(BaseChunkAllocator&& other) noexcept
    {
        m_device         = other.m_device;
        m_chunkAllocator = other.m_chunkAllocator;
        strategy         = other.strategy;
        queue            = other.queue;
        m_chunkSize      = other.m_chunkSize;
        m_allocateSize   = other.m_allocateSize;
        chunks           = std::move(other.chunks);
        gui_identifier   = std::move(other.gui_identifier);
        return *this;
    }

    virtual ~BaseChunkAllocator() = default;

    T* reserve_space(vk::DeviceMemory memory, FreeListEntry freeListEntry, vk::DeviceSize size);


    bool memory_is_free(vk::DeviceMemory memory, FreeListEntry free_mem);

    void destroy();

    MemoryStats collectMemoryStats();

    void showDetailStats(bool expand);

    T* allocate_in_free_space(vk::DeviceSize size, ChunkIterator<T>& chunkAlloc, FreeIterator<T>& freeSpace);

    std::pair<ChunkIterator<T>, AllocationIterator<T>> find_allocation(T* location);

    void swap(T* target, T* source);

    template <typename FreeEntry>
    void add_to_free_list(const ChunkIterator<T>& chunk, const FreeEntry& location) const;
};

template <typename T>
T* BaseChunkAllocator<T>::base_allocate(vk::DeviceSize size)
{
    ChunkIterator<T> chunkAlloc;
    FreeIterator<T> freeSpace;
    std::tie(chunkAlloc, freeSpace) = strategy->findRange(chunks.begin(), chunks.end(), size);

    T* val = allocate_in_free_space(size, chunkAlloc, freeSpace);

    return val;
}

template <typename T>
T* BaseChunkAllocator<T>::allocate_in_free_space(vk::DeviceSize size, ChunkIterator<T>& chunkAlloc,
                                                 FreeIterator<T>& freeSpace)
{
    T* val;
    if (chunkAlloc == chunks.end())
    {
        chunkAlloc = createNewChunk();
        freeSpace  = chunkAlloc->freeList.begin();
    }

    auto memoryStart = freeSpace->offset;

    freeSpace->offset += size;
    freeSpace->size -= size;

    if (freeSpace->size == 0)
    {
        chunkAlloc->freeList.erase(freeSpace);
    }

    findNewMax(chunkAlloc);


    auto targetLocation = create_location(chunkAlloc, memoryStart, size);
    auto memoryEnd      = memoryStart + size;

    auto insertionPoint =
        lower_bound(chunkAlloc->allocations.begin(), chunkAlloc->allocations.end(), memoryEnd,
                    [](const auto& element, vk::DeviceSize value) { return element->offset < value; });

    val = chunkAlloc->allocations.insert(insertionPoint, move(targetLocation))->get();
    chunkAlloc->allocated += size;
    return val;
}

template <typename T>
void BaseChunkAllocator<T>::findNewMax(ChunkIterator<T>& chunkAlloc) const
{
    auto& freeList = chunkAlloc->freeList;

    if (chunkAlloc->freeList.empty())
    {
        chunkAlloc->maxFreeRange = std::nullopt;
        return;
    }


    chunkAlloc->maxFreeRange = *max_element(freeList.begin(), freeList.end(),
                                            [](auto& first, auto& second) { return first.size < second.size; });
}


template <typename T>
void BaseChunkAllocator<T>::deallocate(T* location)
{
    std::scoped_lock alloc_lock(allocationMutex);

    ChunkIterator<T> fChunk;
    AllocationIterator<T> fLoc;

    std::tie(fChunk, fLoc) = find_allocation(location);

    auto& chunkAllocs = fChunk->allocations;

    SAIGA_ASSERT(fLoc != chunkAllocs.end(), "Allocation is not part of the chunk");

    (**fLoc).destroy_owned_data(m_device);
    VLOG(1) << "Deallocating " << location->size << " bytes in chunk/offset [" << distance(chunks.begin(), fChunk)
            << "/" << (*fLoc)->offset << "]";

    add_to_free_list(fChunk, *(fLoc->get()));

    findNewMax(fChunk);

    chunkAllocs.erase(fLoc);

    fChunk->allocated -= location->size;
    while (chunks.size() >= 2)
    {
        auto last = chunks.end() - 1;
        auto stol = chunks.end() - 2;

        if (!last->allocations.empty() || !stol->allocations.empty())
        {
            break;
        }

        m_device.destroy(last->buffer);
        m_chunkAllocator->deallocate(last->chunk);

        last--;
        stol--;

        chunks.erase(last + 1, chunks.end());
    }
}

template <typename T>
void BaseChunkAllocator<T>::destroy()
{
    for (auto& alloc : chunks)
    {
        m_device.destroy(alloc.buffer);
    }
}

template <typename T>
bool BaseChunkAllocator<T>::memory_is_free(vk::DeviceMemory memory, FreeListEntry free_mem)
{
    std::scoped_lock lock(allocationMutex);
    auto chunk = std::find_if(chunks.begin(), chunks.end(),
                              [&](const auto& chunk_entry) { return chunk_entry.chunk->memory == memory; });

    SAIGA_ASSERT(chunk != chunks.end(), "Wrong allocator");

    if (chunk->freeList.empty())
    {
        return false;
    }
    auto found =
        std::lower_bound(chunk->freeList.begin(), chunk->freeList.end(), free_mem,
                         [](const auto& free_entry, const auto& value) { return free_entry.offset < value.offset; });

    return found->offset == free_mem.offset && found->size == free_mem.size;
}


template <typename T>
T* BaseChunkAllocator<T>::reserve_space(vk::DeviceMemory memory, FreeListEntry freeListEntry, vk::DeviceSize size)
{
    std::scoped_lock lock(allocationMutex);
    auto chunk = std::find_if(chunks.begin(), chunks.end(),
                              [&](const auto& chunk_entry) { return chunk_entry.chunk->memory == memory; });

    SAIGA_ASSERT(chunk != chunks.end(), "Wrong allocator");

    auto free = std::find(chunk->freeList.begin(), chunk->freeList.end(), freeListEntry);

    SAIGA_ASSERT(free != chunk->freeList.end(), "Free space not found");

    return allocate_in_free_space(size, chunk, free);
}

template <typename T>
void BaseChunkAllocator<T>::swap(T* target, T* source)
{
    std::scoped_lock lock(allocationMutex);

    SafeAccessor acc(*target, *source);



    //    std::scoped_lock lock(allocationMutex);
    //    const auto size = source->size;
    //
    //    FreeListEntry future_entry{source->offset, source->size};
    //
    ChunkIterator<T> target_chunk, source_chunk;
    AllocationIterator<T> target_alloc, source_alloc;

    std::tie(target_chunk, target_alloc) = find_allocation(target);
    std::tie(source_chunk, source_alloc) = find_allocation(source);


    vk::DeviceSize source_offset, target_offset;
    vk::DeviceMemory source_mem, target_mem;

    source_offset = source->offset;
    target_offset = target->offset;

    source_mem = source->memory;
    target_mem = target->memory;


    source->offset = target_offset;
    source->memory = target_mem;

    target->offset = source_offset;
    target->memory = source_mem;


    std::swap(source->data, target->data);

    // auto old = std::move(*target_alloc);

    std::iter_swap(source_alloc, target_alloc);

    source->modified();
}

template <typename T>
template <typename FreeEntry>
void BaseChunkAllocator<T>::add_to_free_list(const ChunkIterator<T>& chunk, const FreeEntry& location) const
{
    auto& freeList = chunk->freeList;
    auto found = lower_bound(freeList.begin(), freeList.end(), location, [](const auto& free_entry, const auto& value) {
        return free_entry.offset < value.offset;
    });

    FreeIterator<T> free;

    auto previous = prev(found);
    if (found != freeList.begin() && previous->end() == location.offset)
    {
        previous->size += location.size;
        free = previous;
    }
    else
    {
        free  = freeList.insert(found, FreeListEntry{location.offset, location.size});
        found = next(free);
    }
    if (found != freeList.end() && free->end() == found->offset)
    {
        free->size += found->size;
        freeList.erase(found);
    }
}


template <typename T>
std::pair<ChunkIterator<T>, AllocationIterator<T>> BaseChunkAllocator<T>::find_allocation(T* location)
{
    auto fChunk = std::find_if(chunks.begin(), chunks.end(), [&](ChunkAllocation<T> const& alloc) {
        return alloc.chunk->memory == location->memory;
    });

    SAIGA_ASSERT(fChunk != chunks.end(), "Allocation was not done with this allocator!");

    auto& chunkAllocs = fChunk->allocations;
    auto fLoc =
        std::lower_bound(chunkAllocs.begin(), chunkAllocs.end(), location,
                         [](const auto& element, const auto& value) { return element->offset < value->offset; });

    return std::make_pair(fChunk, fLoc);
}

template <typename T>
void BaseChunkAllocator<T>::showDetailStats(bool expand)
{
    using BarColor = ImGui::ColoredBar::BarColor;
    static const BarColor alloc_color_static{{1.00f, 0.447f, 0.133f, 1.0f}, {0.133f, 0.40f, 0.40f, 1.0f}};
    static const BarColor alloc_color_dynamic{{1.00f, 0.812f, 0.133f, 1.0f}, {0.133f, 0.40f, 0.40f, 1.0f}};

    static std::vector<ImGui::ColoredBar> allocation_bars;


    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_None;
    if (expand)
    {
        node_flags = ImGuiTreeNodeFlags_DefaultOpen;
    }
    if (ImGui::CollapsingHeader(gui_identifier.c_str(), node_flags))
    {
        std::scoped_lock lock(allocationMutex);
        ImGui::Indent();

        headerInfo();

        allocation_bars.resize(
            chunks.size(), ImGui::ColoredBar({0, 40}, {{0.1f, 0.1f, 0.1f, 1.0f}, {0.4f, 0.4f, 0.4f, 1.0f}}, true, 1));

        int numAllocs           = 0;
        uint64_t usedSpace      = 0;
        uint64_t innerFreeSpace = 0;
        uint64_t totalFreeSpace = 0;
        for (auto i = 0U; i < allocation_bars.size(); ++i)
        {
            auto& bar   = allocation_bars[i];
            auto& chunk = chunks[i];

            std::stringstream ss;
            ss << "Mem " << std::hex << chunk.chunk->memory << " Buffer " << chunk.buffer;

            ImGui::Text("Chunk %d (%s, %s) %s", i + 1, sizeToString(chunk.getFree()).c_str(),
                        sizeToString(chunk.allocated).c_str(), ss.str().c_str());
            ImGui::Indent();
            bar.renderBackground();
            int j = 0;
            ConstAllocationIterator<T> allocIter;
            ConstFreeIterator<T> freeIter;
            for (allocIter = chunk.allocations.cbegin(), j = 0; allocIter != chunk.allocations.cend(); ++allocIter, ++j)
            {
                auto& color = (*allocIter)->is_static() ? alloc_color_static : alloc_color_dynamic;
                bar.renderArea(static_cast<float>((*allocIter)->offset) / m_chunkSize,
                               static_cast<float>((*allocIter)->offset + (*allocIter)->size) / m_chunkSize, color,
                               false);
                usedSpace += (*allocIter)->size;
            }
            numAllocs += j;

            if (!chunk.freeList.empty())
            {
                auto freeEnd = --chunk.freeList.cend();
                for (freeIter = chunk.freeList.cbegin(); freeIter != freeEnd; freeIter++)
                {
                    innerFreeSpace += freeIter->size;
                    totalFreeSpace += freeIter->size;
                }

                totalFreeSpace += chunk.freeList.back().size;
            }
            ImGui::Unindent();
        }
        ImGui::LabelText("Number of allocations", "%d", numAllocs);
        auto totalSpace = m_chunkSize * chunks.size();


        ImGui::LabelText("Usage", "%s / %s (%.2f%%)", sizeToString(usedSpace).c_str(), sizeToString(totalSpace).c_str(),
                         100 * static_cast<float>(usedSpace) / totalSpace);
        ImGui::LabelText("Free Space (total / fragmented)", "%s / %s", sizeToString(totalFreeSpace).c_str(),
                         sizeToString(innerFreeSpace).c_str());


        ImGui::Unindent();
    }
}

template <typename T>
MemoryStats BaseChunkAllocator<T>::collectMemoryStats()
{
    std::scoped_lock lock(allocationMutex);
    int numAllocs                = 0;
    uint64_t usedSpace           = 0;
    uint64_t fragmentedFreeSpace = 0;
    uint64_t totalFreeSpace      = 0;
    for (auto& chunk : chunks)
    {
        int j = 0;
        ConstAllocationIterator<T> allocIter;
        ConstFreeIterator<T> freeIter;
        for (allocIter = chunk.allocations.cbegin(), j = 0; allocIter != chunk.allocations.cend(); ++allocIter, ++j)
        {
            usedSpace += (*allocIter)->size;
        }
        numAllocs += j;

        if (!chunk.freeList.empty())
        {
            auto freeEnd = --chunk.freeList.cend();

            for (freeIter = chunk.freeList.cbegin(); freeIter != freeEnd; freeIter++)
            {
                fragmentedFreeSpace += freeIter->size;
                totalFreeSpace += freeIter->size;
            }

            totalFreeSpace += chunk.freeList.back().size;
        }
    }
    auto totalSpace = m_chunkSize * chunks.size();
    //
    return MemoryStats{totalSpace, usedSpace, fragmentedFreeSpace};
}

}  // namespace Saiga::Vulkan::Memory
