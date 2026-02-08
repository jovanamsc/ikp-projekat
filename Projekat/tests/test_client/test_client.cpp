#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "../../ahm/ahm.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma warning(disable : 4996)
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
#endif

namespace {
struct Options {
    std::string host = "127.0.0.1";
    uint16_t port = 4000;
    size_t messages = 1000;
    size_t max_message = 64 * 1024;
    bool use_ahm = true;
};

Options ParseArgs(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            options.host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            options.port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--messages" && i + 1 < argc) {
            options.messages = static_cast<size_t>(std::stoull(argv[++i]));
        } else if (arg == "--max-message" && i + 1 < argc) {
            options.max_message = static_cast<size_t>(std::stoull(argv[++i]));
        } else if (arg == "--malloc") {
            options.use_ahm = false;
        }
    }
    return options;
}

#ifdef _WIN32
bool RecvAll(SOCKET socket, void* buffer, size_t size) {
    // Pomocna funkcija: primi tacno zadat broj bajtova.
    char* data = static_cast<char*>(buffer);
    size_t received = 0;
    while (received < size) {
        int result = recv(socket, data + received, static_cast<int>(size - received), 0);
        if (result <= 0) {
            return false;
        }
        received += static_cast<size_t>(result);
    }
    return true;
}

bool SendAll(SOCKET socket, const void* buffer, size_t size) {
    // Pomocna funkcija: posalji tacno zadat broj bajtova.
    const char* data = static_cast<const char*>(buffer);
    size_t sent = 0;
    while (sent < size) {
        int result = send(socket, data + sent, static_cast<int>(size - sent), 0);
        if (result <= 0) {
            return false;
        }
        sent += static_cast<size_t>(result);
    }
    return true;
}
#endif
}

int main(int argc, char** argv) {
    Options options = ParseArgs(argc, argv);
    AdvancedHeapManager::Config config;
    config.heap_count = 4;
    AdvancedHeapManager ahm(config);

#ifndef _WIN32
    std::cerr << "Ovaj test klijent zahteva Windows Winsock.\n";
    return 1;
#else
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "WSAStartup neuspesan.\n";
        return 1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "socket neuspesan.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(options.port);
    inet_pton(AF_INET, options.host.c_str(), &server.sin_addr);

    if (connect(client_socket, reinterpret_cast<sockaddr*>(&server), sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "connect neuspesan.\n";
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Klijent povezan na " << options.host << ":" << options.port
        << " (poruke=" << options.messages
        << ", max_poruka=" << options.max_message
        << ", allocator=" << (options.use_ahm ? "AHM" : "malloc") << ")\n";


    std::mt19937 rng(static_cast<unsigned int>(GetTickCount()));
    std::uniform_int_distribution<size_t> dist(1, options.max_message);

    size_t total_sent = 0;
    size_t total_received = 0;
    auto start_time = std::chrono::steady_clock::now();

    for (size_t i = 0; i < options.messages; ++i) {
        size_t size = dist(rng);
        // Alociraj poruku (AHM ili malloc).
        void* send_buffer = options.use_ahm ? ahm.Malloc(size) : std::malloc(size);
        if (!send_buffer) {
            break;
        }
        std::memset(send_buffer, 0x5A, size);

        uint32_t length = htonl(static_cast<uint32_t>(size));
        if (!SendAll(client_socket, &length, sizeof(length)) ||
            !SendAll(client_socket, send_buffer, size)) {
            if (options.use_ahm) {
                ahm.Free(send_buffer);
            } else {
                std::free(send_buffer);
            }
            break;
        }
        total_sent += size;

        uint32_t response_length = 0;
        if (!RecvAll(client_socket, &response_length, sizeof(response_length))) {
            if (options.use_ahm) {
                ahm.Free(send_buffer);
            } else {
                std::free(send_buffer);
            }
            break;
        }
        response_length = ntohl(response_length);

        // Primi odgovor.
        void* recv_buffer = options.use_ahm ? ahm.Malloc(response_length) : std::malloc(response_length);
        if (!recv_buffer) {
            if (options.use_ahm) {
                ahm.Free(send_buffer);
            } else {
                std::free(send_buffer);
            }
            break;
        }

        if (!RecvAll(client_socket, recv_buffer, response_length)) {
            if (options.use_ahm) {
                ahm.Free(send_buffer);
                ahm.Free(recv_buffer);
            } else {
                std::free(send_buffer);
                std::free(recv_buffer);
            }
            break;
        }
        total_received += response_length;

        if (options.use_ahm) {
            ahm.Free(send_buffer);
            ahm.Free(recv_buffer);
        } else {
            std::free(send_buffer);
            std::free(recv_buffer);
        }

        if ((i + 1) % 100 == 0 || i + 1 == options.messages) {
            std::cout << "Napredak: " << (i + 1) << "/" << options.messages
                << " poslato_bajtova=" << total_sent
                << " primljeno_bajtova=" << total_received << "\n";
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Klijent zavrsio: poslato_bajtova=" << total_sent << " primljeno_bajtova=" << total_received  << " vreme(ms)=" << elapsed_ms.count() << "\n";

    closesocket(client_socket);
    WSACleanup();
    return 0;
#endif
}
