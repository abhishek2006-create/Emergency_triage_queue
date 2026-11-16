#ifndef LOCATOR_TABLE_HPP
#define LOCATOR_TABLE_HPP
#include <cstdint>
#include <cstddef>
#include <vector>
class LocatorTable {
    enum class State { empty, occupied, deleted };
    struct Slot { std::uint64_t id; std::size_t index; State state; Slot():id(0),index(0),state(State::empty){} };
    std::vector<Slot> table_;
    std::size_t count_;
    std::size_t deleted_;
    std::size_t hash(std::uint64_t id) const;
    void rehash(std::size_t capacity);
public:
    LocatorTable();
    void insert(std::uint64_t id, std::size_t index);
    std::size_t find(std::uint64_t id) const;
    bool contains(std::uint64_t id) const;
    void erase(std::uint64_t id);
    std::size_t size() const;
    void clear();
};
#endif
