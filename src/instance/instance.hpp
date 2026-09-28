#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "../memory/memory.hpp"
#include "../offsets.hpp"
#include "../structures/structures.hpp"

class instance {
public:
    uintptr_t address;

    explicit instance(uintptr_t address);

    std::string name() const;
    std::string class_name() const;
    std::string display_name() const;

    instance parent() const;

    std::vector<instance> children() const;

    instance find_first_child(const std::string& childname) const;
    instance find_first_child_of_class(const std::string& class_name) const;
    instance find_first_descendant(const std::string& target_name) const;

    uintptr_t primitive() const;
    Vector3 position() const;
    Vector3 size() const;
    bool rotation(float out[9]) const;
    Vector3 up() const;
    Vector3 velocity() const;
};

inline std::ostream& operator<<(std::ostream& os, const instance& inst) {
    os << "0x" << std::hex << inst.address << std::dec;
    return os;
}
