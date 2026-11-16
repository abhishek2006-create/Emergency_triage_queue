#include "locator_table.hpp"
#include <stdexcept>
#include <utility>
LocatorTable::LocatorTable():table_(16),count_(0),deleted_(0){}
std::size_t LocatorTable::hash(std::uint64_t id) const {
    id ^= id >> 30;
    id *= 0xbf58476d1ce4e5b9ULL;
    id ^= id >> 27;
    id *= 0x94d049bb133111ebULL;
    id ^= id >> 31;
    return static_cast<std::size_t>(id % table_.size());
}
void LocatorTable::rehash(std::size_t capacity) {
    std::vector<Slot> old;
    old.swap(table_);
    table_.resize(capacity);
    count_ = 0;
    deleted_ = 0;
    for (std::size_t i = 0; i < old.size(); ++i) {
        if (old[i].state == State::occupied) {
            std::size_t p = hash(old[i].id);
            while (table_[p].state == State::occupied) p = (p + 1) % table_.size();
            table_[p] = old[i];
            ++count_;
        }
    }
}
void LocatorTable::insert(std::uint64_t id, std::size_t index) {
    std::size_t p = hash(id);
    for (std::size_t i = 0; i < table_.size(); ++i) {
        if (table_[p].state == State::empty) break;
        if (table_[p].state == State::occupied && table_[p].id == id) {
            table_[p].index = index;
            return;
        }
        p = (p + 1) % table_.size();
    }
    if ((count_ + deleted_ + 1) * 10 >= table_.size() * 7) {
        if ((count_ + 1) * 10 >= table_.size() * 7) rehash(table_.size() * 2);
        else rehash(table_.size());
    }
    p = hash(id);
    std::size_t first_deleted = table_.size();
    for (std::size_t i = 0; i < table_.size(); ++i) {
        if (table_[p].state == State::deleted && first_deleted == table_.size()) first_deleted = p;
        if (table_[p].state == State::empty) {
            if (first_deleted != table_.size()) p = first_deleted;
            if (table_[p].state == State::deleted) --deleted_;
            table_[p].id = id;
            table_[p].index = index;
            table_[p].state = State::occupied;
            ++count_;
            return;
        }
        p = (p + 1) % table_.size();
    }
    if (first_deleted != table_.size()) {
        Slot& slot = table_[first_deleted];
        slot.id = id;
        slot.index = index;
        slot.state = State::occupied;
        --deleted_;
        ++count_;
        return;
    }
    throw std::runtime_error("Locator full");
}
bool LocatorTable::contains(std::uint64_t id) const {
    std::size_t p = hash(id);
    for (std::size_t i = 0; i < table_.size(); ++i) {
        if (table_[p].state == State::empty) return false;
        if (table_[p].state == State::occupied && table_[p].id == id) return true;
        p = (p + 1) % table_.size();
    }
    return false;
}
std::size_t LocatorTable::find(std::uint64_t id) const {
    std::size_t p = hash(id);
    for (std::size_t i = 0; i < table_.size(); ++i) {
        if (table_[p].state == State::empty) break;
        if (table_[p].state == State::occupied && table_[p].id == id) return table_[p].index;
        p = (p + 1) % table_.size();
    }
    throw std::out_of_range("Missing patient ID");
}
void LocatorTable::erase(std::uint64_t id) {
    std::size_t p = hash(id);
    for (std::size_t i = 0; i < table_.size(); ++i) {
        if (table_[p].state == State::empty) break;
        if (table_[p].state == State::occupied && table_[p].id == id) {
            table_[p].state = State::deleted;
            --count_;
            ++deleted_;
            return;
        }
        p = (p + 1) % table_.size();
    }
    throw std::out_of_range("Missing patient ID");
}
std::size_t LocatorTable::size() const { return count_; }
void LocatorTable::clear() { table_.clear(); table_.resize(16); count_ = 0; deleted_ = 0; }
