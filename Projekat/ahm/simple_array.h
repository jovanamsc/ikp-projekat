#pragma once

#include <cstddef>

// Jednostavan dinamicki niz (bez STL kontejnera).
template <typename T>
class SimpleArray {
public:
    SimpleArray() : data_(nullptr), size_(0) {}
    ~SimpleArray() { delete[] data_; }
    SimpleArray(const SimpleArray&) = delete;
    SimpleArray& operator=(const SimpleArray&) = delete;

    void Reset(size_t size) {
        delete[] data_;
        size_ = size;
        data_ = (size_ > 0) ? new T[size_] : nullptr;
    }

    size_t Size() const { return size_; }
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }

private:
    T* data_;
    size_t size_;
};
