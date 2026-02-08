#pragma once

#include <cstddef>
#include <cstdint>

// Jednostavna hash mapa (bez STL map/list) za mapiranje adresa na vrednosti.
template <typename TValue>
class AllocationMap {
public:
    AllocationMap() : entries_(nullptr), capacity_(0), size_(0), tombstones_(0) {
        Rehash(32);
    }

    ~AllocationMap() {
        delete[] entries_;
    }

    AllocationMap(const AllocationMap&) = delete;
    AllocationMap& operator=(const AllocationMap&) = delete;

    void Insert(void* key, const TValue& value) {
        if ((size_ + tombstones_ + 1) * 10 >= capacity_ * 7) {
            Rehash(capacity_ * 2);
        }

        size_t index = Hash(key) % capacity_;
        size_t first_tombstone = capacity_;
        while (entries_[index].occupied) {
            if (entries_[index].key == key) {
                entries_[index].value = value;
                return;
            }
            if (entries_[index].tombstone && first_tombstone == capacity_) {
                first_tombstone = index;
            }
            index = (index + 1) % capacity_;
        }

        size_t target = (first_tombstone == capacity_) ? index : first_tombstone;
        entries_[target].key = key;
        entries_[target].value = value;
        entries_[target].occupied = true;
        if (entries_[target].tombstone) {
            entries_[target].tombstone = false;
            if (tombstones_ > 0) {
                --tombstones_;
            }
        }
        ++size_;
    }

    bool Find(void* key, TValue& value) const {
        if (capacity_ == 0) {
            return false;
        }
        size_t index = Hash(key) % capacity_;
        size_t start = index;
        while (entries_[index].occupied) {
            if (!entries_[index].tombstone && entries_[index].key == key) {
                value = entries_[index].value;
                return true;
            }
            index = (index + 1) % capacity_;
            if (index == start) {
                break;
            }
        }
        return false;
    }

    bool Erase(void* key) {
        if (capacity_ == 0) {
            return false;
        }
        size_t index = Hash(key) % capacity_;
        size_t start = index;
        while (entries_[index].occupied) {
            if (!entries_[index].tombstone && entries_[index].key == key) {
                entries_[index].tombstone = true;
                if (size_ > 0) {
                    --size_;
                }
                ++tombstones_;
                return true;
            }
            index = (index + 1) % capacity_;
            if (index == start) {
                break;
            }
        }
        return false;
    }

private:
    struct Entry {
        void* key = nullptr;
        TValue value{};
        bool occupied = false;
        bool tombstone = false;
    };

    void Rehash(size_t new_capacity) {
        Entry* old_entries = entries_;
        size_t old_capacity = capacity_;

        entries_ = new Entry[new_capacity];
        capacity_ = new_capacity;
        size_ = 0;
        tombstones_ = 0;

        if (old_entries) {
            for (size_t i = 0; i < old_capacity; ++i) {
                if (old_entries[i].occupied && !old_entries[i].tombstone) {
                    Insert(old_entries[i].key, old_entries[i].value);
                }
            }
            delete[] old_entries;
        }
    }

    size_t Hash(void* key) const {
        size_t value = static_cast<size_t>(reinterpret_cast<uintptr_t>(key));
        value ^= (value >> 33);
        value *= 0xff51afd7ed558ccdULL;
        value ^= (value >> 33);
        return value;
    }

    Entry* entries_;
    size_t capacity_;
    size_t size_;
    size_t tombstones_;
};
