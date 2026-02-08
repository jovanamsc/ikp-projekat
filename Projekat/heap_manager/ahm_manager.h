#pragma once

#include <cstddef>

// Jednostavan C interfejs za AHM.
void ManagerInitialization_inicijalizuj_manager(int broj_heapova);
void ManagerInitialization_deinicijalizuj_manager();
void* ahm_malloc(size_t size);
void ahm_free(void* ptr);
