
#ifndef LOCATOR_TABLE_HPP
#define LOCATOR_TABLE_HPP

#include <vector>
#include <cstdint>
#include <cstddef>

class LocatorTable {
private:
    enum class State {
        EMPTY,
        OCCUPIED,
        DELETED
    };

    struct Slot {
        std::uint64_t id = 0;
        std::size_t index = 0;
        State state = State::EMPTY;
    };

    std::vector<Slot> table;
    std::size_t count = 0;
    std::size_t deletedCount = 0;

    std::size_t hashFunction(std::uint64_t id) const;
    void rehash(std::size_t newCapacity);

public:
    LocatorTable();

    void insert(std::uint64_t id, std::size_t index);
    std::size_t find(std::uint64_t id) const;
    void erase(std::uint64_t id);
    bool contains(std::uint64_t id) const;

    std::size_t size() const;
    void clear();
};

#endif
