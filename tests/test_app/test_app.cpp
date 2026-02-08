#include "../../ahm/ahm.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <thread>

namespace {
struct Options {
    size_t threads = 1;
    size_t total_bytes = 4ull * 1024ull * 1024ull * 1024ull;
    size_t block_size = 1024 * 1024;
    bool use_ahm = true;
};

Options ParseArgs(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--threads" && i + 1 < argc) {
            options.threads = static_cast<size_t>(std::stoull(argv[++i]));
        } else if (arg == "--total-bytes" && i + 1 < argc) {
            options.total_bytes = static_cast<size_t>(std::stoull(argv[++i]));
        } else if (arg == "--block-size" && i + 1 < argc) {
            options.block_size = static_cast<size_t>(std::stoull(argv[++i]));
        } else if (arg == "--malloc") {
            options.use_ahm = false;
        }
    }
    return options;
}
}

int main(int argc, char** argv) {
    Options options = ParseArgs(argc, argv);
    AdvancedHeapManager::Config config;
    config.heap_count = 8;
    AdvancedHeapManager ahm(config);

    const size_t bytes_per_thread = options.total_bytes / options.threads;
    const size_t blocks_per_thread = bytes_per_thread / options.block_size;

    auto start = std::chrono::high_resolution_clock::now();
    std::thread* workers = new std::thread[options.threads];

    for (size_t t = 0; t < options.threads; ++t) {
        workers[t] = std::thread([&, t]() {
            void** allocations = new void*[blocks_per_thread];
            size_t allocation_count = 0;

            for (size_t i = 0; i < blocks_per_thread; ++i) {
                void* ptr = nullptr;
                if (options.use_ahm) {
                    ptr = ahm.Malloc(options.block_size);
                } else {
                    ptr = std::malloc(options.block_size);
                }

                if (!ptr) {
                    break;
                }
                allocations[allocation_count++] = ptr;
            }

            for (size_t i = 0; i < allocation_count; ++i) {
                if (options.use_ahm) {
                    ahm.Free(allocations[i]);
                } else {
                    std::free(allocations[i]);
                }
            }
            delete[] allocations;
        });
    }

    for (size_t i = 0; i < options.threads; ++i) {
        workers[i].join();
    }
    delete[] workers;

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Threads: " << options.threads << "\n";
    std::cout << "Total bytes: " << options.total_bytes << "\n";
    std::cout << "Block size: " << options.block_size << "\n";
    std::cout << "Allocator: " << (options.use_ahm ? "AHM" : "malloc/free") << "\n";
    std::cout << "Duration (ms): " << duration_ms << "\n";

    return 0;
}
