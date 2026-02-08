#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "../../heap_manager/ahm_manager.h"

CRITICAL_SECTION cs;
int brojac = 0;

DWORD WINAPI test_ahm_funkcije(LPVOID lpParam);
DWORD WINAPI test_ugradjene_funkcije(LPVOID lpParam);

// Inicijalizacija AHM testova
void test_ahm_init(int broj_niti) {
    InitializeCriticalSection(&cs);

    // 200000 * 20000 = 4gb (20000 alokacija iz testa)
    int broj_bajta = 200000 / broj_niti;

    printf("\n\tAHM malloc i free funkcije\n");

    HANDLE* niti = static_cast<HANDLE*>(std::malloc(broj_niti * sizeof(HANDLE)));
    if (!niti) {
        printf("Neuspesna alokacija za niz niti.\n");
        DeleteCriticalSection(&cs);
        return;
    }

    clock_t start_time = clock();
    for (int i = 0; i < broj_niti; i++) {
        niti[i] = CreateThread(NULL, 0, &test_ahm_funkcije, reinterpret_cast<LPVOID>(broj_bajta), 0, NULL);
    }

    for (int i = 0; i < broj_niti; i++) {
        WaitForSingleObject(niti[i], INFINITE);
    }
    clock_t end_time = clock();

    double cpu_time_used = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("\nBroj niti: %d\nPotrebno vreme za zauzimanje i oslobadjanje memorije: %f sekundi\n", broj_niti, cpu_time_used);
    printf("\n----------------------------------------------------------------------\n");

    for (int i = 0; i < broj_niti; i++) {
        CloseHandle(niti[i]);
    }
    std::free(niti);

    printf("NIJE NULL (uspesna alokacija): %d elemenata!\n", brojac);
    brojac = 0;
    DeleteCriticalSection(&cs);
}

// Inicijalizacija obicnih testova
void test_obican_init(int broj_niti) {
    // 200000 * 20000 = 4gb (20000 alokacija iz testa)
    int broj_bajta = 200000 / broj_niti;

    printf("\n\tUgradjene malloc i free funkcije\n");

    HANDLE* niti = static_cast<HANDLE*>(std::malloc(broj_niti * sizeof(HANDLE)));
    if (!niti) {
        printf("Neuspesna alokacija za niz niti.\n");
        return;
    }

    clock_t start_time = clock();
    for (int i = 0; i < broj_niti; i++) {
        niti[i] = CreateThread(NULL, 0, &test_ugradjene_funkcije, reinterpret_cast<LPVOID>(broj_bajta), 0, NULL);
    }

    for (int i = 0; i < broj_niti; i++) {
        WaitForSingleObject(niti[i], INFINITE);
    }
    clock_t end_time = clock();

    double cpu_time_used = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("\nBroj niti: %d\nPotrebno vreme za zauzimanje i oslobadjanje memorije: %f sekundi\n", broj_niti, cpu_time_used);
    printf("\n----------------------------------------------------------------------\n");

    for (int i = 0; i < broj_niti; i++) {
        CloseHandle(niti[i]);
    }
    std::free(niti);
}

// TESTOVI AHM FUNKCIJA
DWORD WINAPI test_ahm_funkcije(LPVOID lpParam) {
    void* objekti[20000];
    int broj_bajta = static_cast<int>(reinterpret_cast<intptr_t>(lpParam));

    for (int i = 0; i < 20000; i++) {
        objekti[i] = ahm_malloc(broj_bajta);
    }

    for (int i = 0; i < 20000; i++) {
        if (objekti[i] != NULL) {
            ahm_free(objekti[i]);
            EnterCriticalSection(&cs);
            brojac++;
            LeaveCriticalSection(&cs);
        }
    }

    return 0;
}

// TESTOVI UGRADJENIH FUNKCIJA
DWORD WINAPI test_ugradjene_funkcije(LPVOID lpParam) {
    void* objekti[20000];
    int broj_bajta = static_cast<int>(reinterpret_cast<intptr_t>(lpParam));

    for (int i = 0; i < 20000; i++) {
        objekti[i] = std::malloc(broj_bajta);
    }

    for (int i = 0; i < 20000; i++) {
        if (objekti[i] != NULL) {
            std::free(objekti[i]);
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Morate uneti broj heap-ova kao argument komandne linije!\n");
        return 0;
    }

    ManagerInitialization_inicijalizuj_manager(std::atoi(argv[1]));

    // AHM testovi
    test_ahm_init(1);
    test_ahm_init(2);
    test_ahm_init(5);
    test_ahm_init(10);
    test_ahm_init(20);
    test_ahm_init(50);

    ManagerInitialization_deinicijalizuj_manager();

    // Ugradjene funkcije testovi
    test_obican_init(1);
    test_obican_init(2);
    test_obican_init(5);
    test_obican_init(10);
    test_obican_init(20);
    test_obican_init(50);

    printf("\nPritisnite bilo sta za izlazak iz programa...\n");
    getchar();
    return 0;
}
