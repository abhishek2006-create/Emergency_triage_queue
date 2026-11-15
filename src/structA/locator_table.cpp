
#include "locator_table.hpp"
#include <stdexcept>
#include <utility>

LocatorTable::LocatorTable() : table(16) {}

std::size_t LocatorTable::hashFunction(std::uint64_t id) const {

    id ^= id >> 30;
    id *= 0xbf58476d1ce4e5b9ULL;
    id ^= id >> 27;
    id *= 0x94d049bb133111ebULL;
    id ^= id >> 31;

    return static_cast<std::size_t>(id % table.size());
}

bool LocatorTable::contains(std::uint64_t id) const {
    std::size_t pos = hashFunction(id);

    for (std::size_t i = 0; i < table.size(); i++) {
        const Slot& slot = table[pos];

        if (slot.state == State::EMPTY)
            return false;

        if (slot.state == State::OCCUPIED && slot.id == id)
            return true;

        pos = (pos + 1) % table.size();
    }

    return false;
}

std::size_t LocatorTable::find(std::uint64_t id) const {
    std::size_t pos = hashFunction(id);

    for (std::size_t i = 0; i < table.size(); i++) {
        const Slot& slot = table[pos];

        if (slot.state == State::EMPTY)
            break;

        if (slot.state == State::OCCUPIED && slot.id == id)
            return slot.index;

        pos = (pos + 1) % table.size();
    }

    throw std::out_of_range("Patient ID not found");
}


void LocatorTable::insert(std::uint64_t id, std::size_t index) {

    std::size_t pos = hashFunction(id);

    for (std::size_t i = 0; i < table.size(); i++) {
        if (table[pos].state == State::EMPTY)
            break;

        if (table[pos].state == State::OCCUPIED &&
            table[pos].id == id) {
            table[pos].index = index;
            return;
        }

        pos = (pos + 1) % table.size();
    }

    if ((count + deletedCount + 1) * 10 >= table.size() * 7) {
        if ((count + 1) * 10 >= table.size() * 7)
            rehash(table.size() * 2);
        else
            rehash(table.size());
    }

    pos = hashFunction(id);
    std::size_t firstDeleted = table.size();

    for (std::size_t i = 0; i < table.size(); i++) {
        if (table[pos].state == State::DELETED &&
            firstDeleted == table.size()) {
            firstDeleted = pos;
        }

        if (table[pos].state == State::EMPTY) {
            if (firstDeleted != table.size())
                pos = firstDeleted;

            if (table[pos].state == State::DELETED)
                deletedCount--;

            table[pos] = {id, index, State::OCCUPIED};
            count++;
            return;
        }

        pos = (pos + 1) % table.size();
    }

    if (firstDeleted != table.size()) {
        table[firstDeleted] = {id, index, State::OCCUPIED};
        deletedCount--;
        count++;
        return;
    }

    throw std::runtime_error("Locator table is full");
}

void LocatorTable::erase(std::uint64_t id) {
    std::size_t pos = hashFunction(id);

    for (std::size_t i = 0; i < table.size(); i++) {
        if (table[pos].state == State::EMPTY)
            break;

        if (table[pos].state == State::OCCUPIED &&
            table[pos].id == id) {
            table[pos].state = State::DELETED;
            count--;
            deletedCount++;
            return;
        }

        pos = (pos + 1) % table.size();
    }

    throw std::out_of_range("Patient ID not found");
}

void LocatorTable::rehash(std::size_t newCapacity) {
    std::vector<Slot> oldTable = std::move(table);

    table = std::vector<Slot>(newCapacity);
    count = 0;
    deletedCount = 0;

    for (const Slot& slot : oldTable) {
        if (slot.state == State::OCCUPIED) {
            std::size_t pos = hashFunction(slot.id);

            while (table[pos].state == State::OCCUPIED) {
                pos = (pos + 1) % table.size();
            }

            table[pos] = slot;
            count++;
        }
    }
}

std::size_t LocatorTable::size() const {
    return count;
}

void LocatorTable::clear() {
    table = std::vector<Slot>(16);
    count = 0;
    deletedCount = 0;
}
