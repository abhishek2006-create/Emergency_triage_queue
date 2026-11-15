#ifndef LOCATOR_TABLE_HPP
#define LOCATOR_TABLE_HPP

class LocatorTable {
    public:
        void insert(int id, int index);
        int find(int id);
        void erase(int id);
};
#endif