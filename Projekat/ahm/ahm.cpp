#include "ahm.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

AdvancedHeapManager::AdvancedHeapManager(const Config& config) {
#ifdef _WIN32
    if (config.heap_count == 0) {
        throw std::invalid_argument("heap_count must be greater than zero");
    }

    heaps_.Reset(config.heap_count);
    allocated_bytes_.Reset(config.heap_count);

    // Kreiraj konfigurabilan broj heap-ova (HeapCreate).
    for (size_t i = 0; i < config.heap_count; ++i) {
        HANDLE heap = HeapCreate(0, config.initial_size_bytes, config.maximum_size_bytes);
        if (!heap) {
            throw std::runtime_error("HeapCreate failed");
        }
        heaps_[i] = heap;
        allocated_bytes_[i] = 0;
    }
#else
    (void)config;
#endif
}

AdvancedHeapManager::~AdvancedHeapManager() {
#ifdef _WIN32
    for (size_t i = 0; i < heaps_.Size(); ++i) {
        if (heaps_[i]) {
            HeapDestroy(heaps_[i]);
        }
    }
#endif
}

void* AdvancedHeapManager::Malloc(size_t size) {
    if (size == 0) {
        size = 1;
    }

#ifdef _WIN32
    // Balanser bira heap sa najmanje zauzetih bajtova.
    std::lock_guard<std::mutex> lock(mutex_);
    size_t heap_index = SelectHeapIndex();
    void* ptr = HeapAlloc(heaps_[heap_index], 0, size);
    if (!ptr) {
        return nullptr;
    }
    allocated_bytes_[heap_index] += size;
    // Sacuvaj vlasnistvo alokacije za pravilan Free.
    allocations_.Insert(ptr, AllocationInfo{heap_index, size});
    return ptr;
#else
    void* ptr = std::malloc(size);
    if (!ptr) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    allocations_.Insert(ptr, AllocationInfo{size});
    return ptr;
#endif
}

void AdvancedHeapManager::Free(void* ptr) {
    if (!ptr) {
        return;
    }

#ifdef _WIN32
    // Pronadji heap iz kog je alocirano i vrati memoriju u isti heap.
    std::lock_guard<std::mutex> lock(mutex_);
    AllocationInfo info{};
    if (!allocations_.Find(ptr, info)) {
        return;
    }
    HeapFree(heaps_[info.heap_index], 0, ptr);
    allocated_bytes_[info.heap_index] -= info.size_bytes;
    allocations_.Erase(ptr);
#else
    std::lock_guard<std::mutex> lock(mutex_);
    AllocationInfo info{};
    if (!allocations_.Find(ptr, info)) {
        return;
    }
    allocations_.Erase(ptr);
    std::free(ptr);
#endif
}

size_t AdvancedHeapManager::HeapCount() const {
#ifdef _WIN32
    return heaps_.Size();
#else
    return 1;
#endif
}

size_t AdvancedHeapManager::AllocatedBytes(size_t heap_index) const {
#ifdef _WIN32
    std::lock_guard<std::mutex> lock(mutex_);
    if (heap_index >= allocated_bytes_.Size()) {
        return 0;
    }
    return allocated_bytes_[heap_index];
#else
    (void)heap_index;
    return 0;
#endif
}

#ifdef _WIN32
size_t AdvancedHeapManager::SelectHeapIndex() const {
    size_t min_index = 0;
    size_t min_value = allocated_bytes_.Size() > 0 ? allocated_bytes_[0] : 0;
    for (size_t i = 1; i < allocated_bytes_.Size(); ++i) {
        if (allocated_bytes_[i] < min_value) {
            min_value = allocated_bytes_[i];
            min_index = i;
        }
    }
    return min_index;
}
#endif
