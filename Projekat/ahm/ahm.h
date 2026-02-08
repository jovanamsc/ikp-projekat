#pragma once

#include <cstddef>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#endif

#include "allocation_map.h"
#include "simple_array.h"

// Napredni Heap Manager (AHM) - balansira alokacije preko vise heap-ova.
// Mapiranje alokacija omogucava da se memorija vrati u heap iz kog je uzeta.
class AdvancedHeapManager {
public:
    struct Config {
        size_t heap_count = 4;
        size_t initial_size_bytes = 0;
        size_t maximum_size_bytes = 0;
    };

    explicit AdvancedHeapManager(const Config& config);
    ~AdvancedHeapManager();

    AdvancedHeapManager(const AdvancedHeapManager&) = delete;
    AdvancedHeapManager& operator=(const AdvancedHeapManager&) = delete;

    void* Malloc(size_t size);
    void Free(void* ptr);

    size_t HeapCount() const;
    size_t AllocatedBytes(size_t heap_index) const;

private:
#ifdef _WIN32
    // Informacije o alokaciji: kom heap-u pripada i kolika je velicina.
    struct AllocationInfo {
        size_t heap_index = 0;
        size_t size_bytes = 0;
    };

    // Izaberi heap sa najmanje zauzetih bajtova.
    size_t SelectHeapIndex() const;

    // Pool heap-ova i pracenje zauzeca po heap-u.
    SimpleArray<HANDLE> heaps_;
    SimpleArray<size_t> allocated_bytes_;
    // Mapa adresa -> info o heap-u radi pravilnog Free.
    AllocationMap<AllocationInfo> allocations_;
    mutable std::mutex mutex_;
#else
    // Na ne-Windows platformama pratimo samo velicinu radi simetrije API-ja.
    struct AllocationInfo {
        size_t size_bytes = 0;
    };

    AllocationMap<AllocationInfo> allocations_;
    mutable std::mutex mutex_;
#endif
};
