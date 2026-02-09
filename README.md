# ikp-projekat

Advanced Heap Manager (AHM) – primer implementacije za **Windows**. Alokator koristi konfigurabilan broj heap-ova (kreiranih pomoæu `HeapCreate`) i rasporeðuje nove alokacije na heap sa trenutno **najmanje zauzetih bajtova**. Takoðe vodi mapu alokacija kako bi se memorija prilikom `Free` vratila u **taèan heap** iz kog je uzeta.

---

## Struktura projekta

* `ahm/` – jezgro AHM implementacije
* `heap_manager/` – C interfejs (inicijalizacija + `ahm_malloc` / `ahm_free`)
* `tests/test_app/` – benchmark za alokacije
* `tests/test_server/` – test server
* `tests/test_client/` – test klijent
* `tests/test_threads/` – thread test (AHM vs malloc/free)

---

## Build (Windows)

```bat
cmake -S . -B build
cmake --build build --config Release
```

> Nakon build-a, izvršni fajlovi se nalaze u folderu `build\Release\` (ili zavisno od generatora u `x64\Release`).

---

## Test aplikacija (benchmark)

Pokretanje AHM varijante:

```bat
.\build\Release\test_app.exe --threads 10
```

Pokretanje sa standardnim `malloc/free`:

```bat
.\build\Release\test_app.exe --threads 10 --malloc
```

### Opcionalni argumenti

* `--threads <n>` – broj thread-ova
* `--total-bytes <bytes>` – ukupna kolièina memorije
* `--block-size <bytes>` – velièina pojedinaènog bloka

---

## Thread test (AHM vs malloc/free)

Primer koji prati strukturu iz zahteva: pokreæe testove sa 1, 2, 5, 10, 20 i 50 niti.

```bat
.\build\Release\test_threads.exe 5
```

Argument predstavlja **broj heap-ova** koje AHM koristi.

---

## Test server / client

Server prihvata više klijenata, èita poruku sa prefiksom dužine i vraæa odgovor nasumiène velièine. Klijent generiše poruke nasumiène velièine.

```bat
.\build\Release\test_server.exe --port 4000
.\build\Release\test_client.exe --host 127.0.0.1 --port 4000
```

Za poreðenje sa podrazumevanim alokatorom, dodati `--malloc` bilo kom izvršnom fajlu.

---

## Napomena

Na Windows-u je potrebno koristiti:

* `\` kao separator putanja
* prefiks `.` (npr. `.\build\Release\...`)
* ekstenziju `.exe`

U suprotnom, komandni prompt neæe prepoznati izvršni fajl.
