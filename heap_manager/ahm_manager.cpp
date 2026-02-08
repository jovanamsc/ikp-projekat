#include "ahm_manager.h"

#include "../ahm/ahm.h"

// Globalni menadzer, inicijalizuje se jednom u testovima.
static AdvancedHeapManager* g_manager = nullptr;

void ManagerInitialization_inicijalizuj_manager(int broj_heapova) {
    if (g_manager != nullptr) {
        return;
    }

    AdvancedHeapManager::Config config;
    config.heap_count = broj_heapova > 0 ? static_cast<size_t>(broj_heapova) : 1;
    g_manager = new AdvancedHeapManager(config);
}

void ManagerInitialization_deinicijalizuj_manager() {
    delete g_manager;
    g_manager = nullptr;
}

void* ahm_malloc(size_t size) {
    if (!g_manager) {
        return nullptr;
    }
    return g_manager->Malloc(size);
}

void ahm_free(void* ptr) {
    if (!g_manager) {
        return;
    }
    g_manager->Free(ptr);
}
