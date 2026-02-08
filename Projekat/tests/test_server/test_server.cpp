#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "../../ahm/ahm.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <utility>

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
    uint16_t port = 4000;
    size_t max_message = 64 * 1024;
    bool use_ahm = true;
};

// Jednostavna dinamicka lista niti bez STL kontejnera.
class ThreadList {
public:
    ThreadList() : data_(nullptr), size_(0), capacity_(0) {}
    ~ThreadList() { delete[] data_; }

    ThreadList(const ThreadList&) = delete;
    ThreadList& operator=(const ThreadList&) = delete;

    void Add(std::thread&& thread) {
        if (size_ == capacity_) {
            Resize(capacity_ == 0 ? 4 : capacity_ * 2);
        }
        data_[size_++] = std::move(thread);
    }

    void JoinAll() {
        for (size_t i = 0; i < size_; ++i) {
            if (data_[i].joinable()) {
                data_[i].join();
            }
        }
    }

private:
    void Resize(size_t new_capacity) {
        std::thread* new_data = new std::thread[new_capacity];
        for (size_t i = 0; i < size_; ++i) {
            new_data[i] = std::move(data_[i]);
        }
        delete[] data_;
        data_ = new_data;
        capacity_ = new_capacity;
    }

    std::thread* data_;
    size_t size_;
    size_t capacity_;
};

Options ParseArgs(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            options.port = static_cast<uint16_t>(std::stoi(argv[++i]));
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
    config.heap_count = 8;
    AdvancedHeapManager ahm(config);
    std::atomic<size_t> total_messages{ 0 };
    std::atomic<size_t> total_bytes{ 0 };

#ifndef _WIN32
    std::cerr << "Ovaj test server zahteva Windows Winsock.\n";
    return 1;
#else
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "WSAStartup neuspesan.\n";
        return 1;
    }

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        std::cerr << "socket neuspesan.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = htonl(INADDR_ANY);
    service.sin_port = htons(options.port);

    if (bind(listen_socket, reinterpret_cast<sockaddr*>(&service), sizeof(service)) == SOCKET_ERROR) {
        std::cerr << "bind neuspesan.\n";
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen neuspesan.\n";
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server pokrenut, port " << options.port << "\n";
    std::atomic<bool> running{true};

    ThreadList workers;
    while (running.load()) {
        SOCKET client_socket = accept(listen_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            break;
        }

        workers.Add(std::thread([&, client_socket]() {
            // Za svakog klijenta generisemo nasumicnu duzinu odgovora.
            std::mt19937 rng(static_cast<unsigned int>(GetTickCount()));
            std::uniform_int_distribution<size_t> dist(1, options.max_message);
            size_t client_messages = 0;
            size_t client_bytes = 0;
            auto client_start = std::chrono::steady_clock::now();

            while (true) {
                uint32_t length = 0;
                if (!RecvAll(client_socket, &length, sizeof(length))) {
                    break;
                }
                length = ntohl(length);

                // Primi poruku.
                void* recv_buffer = options.use_ahm ? ahm.Malloc(length) : std::malloc(length);
                if (!recv_buffer) {
                    break;
                }
                if (!RecvAll(client_socket, recv_buffer, length)) {
                    if (options.use_ahm) {
                        ahm.Free(recv_buffer);
                    } else {
                        std::free(recv_buffer);
                    }
                    break;
                }
                ++client_messages;
                client_bytes += length;

                // Pripremi odgovor nasumicne duzine.
                size_t response_size = dist(rng);
                void* send_buffer = options.use_ahm ? ahm.Malloc(response_size) : std::malloc(response_size);
                if (!send_buffer) {
                    if (options.use_ahm) {
                        ahm.Free(recv_buffer);
                    } else {
                        std::free(recv_buffer);
                    }
                    break;
                }
                std::memset(send_buffer, 0xA5, response_size);

                uint32_t response_length = htonl(static_cast<uint32_t>(response_size));
                if (!SendAll(client_socket, &response_length, sizeof(response_length)) ||
                    !SendAll(client_socket, send_buffer, response_size)) {
                    if (options.use_ahm) {
                        ahm.Free(send_buffer);
                        ahm.Free(recv_buffer);
                    } else {
                        std::free(send_buffer);
                        std::free(recv_buffer);
                    }
                    break;
                }
                ++client_messages;
                client_bytes += response_size;

                if (options.use_ahm) {
                    ahm.Free(send_buffer);
                    ahm.Free(recv_buffer);
                } else {
                    std::free(send_buffer);
                    std::free(recv_buffer);
                }
            }
            auto client_end = std::chrono::steady_clock::now();
            total_messages.fetch_add(client_messages, std::memory_order_relaxed);
            total_bytes.fetch_add(client_bytes, std::memory_order_relaxed);
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(client_end - client_start);
            std::cout << "Klijent zavrsio: poruke=" << client_messages
                << " bajtova=" << client_bytes
                << " vreme(ms)=" << elapsed_ms.count() << "\n";

            closesocket(client_socket);
        }));
    }

    workers.JoinAll();
    std::cout << "Server statistika: poruke=" << total_messages.load() << " bajtova=" << total_bytes.load() << "\n";

    closesocket(listen_socket);
    WSACleanup();
    return 0;
#endif
}
