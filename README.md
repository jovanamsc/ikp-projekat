# ikp-projekat

Advanced Heap Manager (AHM) sample implementation for Windows. The allocator uses a configurable
number of heaps (created with `HeapCreate`) and dispatches new allocations to the heap with the
lowest number of currently allocated bytes. It also keeps a map of allocations so memory is
returned to the correct heap on `Free`.

## Folderi

- `ahm/` - jezgro AHM implementacije
- `heap_manager/` - C interfejs (inicijalizacija i ahm_malloc/ahm_free)
- `tests/test_app/` - benchmark za alokacije
- `tests/test_server/` - test server
- `tests/test_client/` - test klijent
- `tests/test_threads/` - thread test iz primera

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Test application

Allocates and frees up to 4 GB of memory across a configurable number of threads.

```bash
build/Release/test_app --threads 10
build/Release/test_app --threads 10 --malloc
```

Optional arguments:
- `--total-bytes <bytes>`
- `--block-size <bytes>`

## Thread test (AHM vs malloc/free)

Primer koji prati strukturu iz zahteva: pokrece testove sa 1, 2, 5, 10, 20 i 50 niti.

```bash
build/Release/test_threads 5
```

Argument predstavlja broj heap-ova koje AHM koristi.

## Test server/client

The server accepts multiple clients, reads a length-prefixed message, and returns a random-sized
response. The client generates random-sized messages.

```bash
build/Release/test_server --port 4000
build/Release/test_client --host 127.0.0.1 --port 4000
```

To compare with the default allocator, add `--malloc` to either executable.
