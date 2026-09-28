#include "instance.hpp"

instance::instance(uintptr_t address) : address(address) {}


std::string instance::name() const {
    if (!address)
        return "";

    const auto name_container = memory::read<uintptr_t>(address + offsets::namecontainer);
    if (!name_container)
        return "";

    return memory::read_string(name_container + offsets::name);
}


std::string instance::class_name() const {
    if (!address)
        return "";

    const auto descriptor = memory::read<uintptr_t>(address + offsets::class_descriptor);
    if (!descriptor)
        return "";

    const auto string_ptr = memory::read<uintptr_t>(descriptor + 0x8);
    if (!string_ptr)
        return "";

    return memory::read_string(string_ptr);
}


std::string instance::display_name() const {
    if (!address)
        return "";

    return memory::read_string(address + offsets::display_name);
}

instance instance::parent() const {
    if (!address)
        return instance(0);
    return instance(memory::read<uintptr_t>(address + offsets::parent));
}

std::vector<instance> instance::children() const {
    std::vector<instance> container;
    if (!address)
        return container;

    const auto start = memory::read<uintptr_t>(address + offsets::children);
    if (!start)
        return container;

    const auto end = memory::read<uintptr_t>(start + offsets::children_end);

    for (auto curr = memory::read<uintptr_t>(start); curr != end; curr += 0x10)
        container.emplace_back(memory::read<uintptr_t>(curr));

    return container;
}

instance instance::find_first_child(const std::string& childname) const {
    if (!address || childname.empty())
        return instance(0);

    for (const instance& child : children()) {
        if (!child.address)
            continue;
        if (child.name() == childname)
            return child;
    }
    return instance(0);
}

instance instance::find_first_child_of_class(const std::string& wanted_class) const {
    if (!address || wanted_class.empty())
        return instance(0);

    for (const instance& child : children()) {
        if (!child.address)
            continue;
        if (child.class_name() == wanted_class)
            return child;
    }
    return instance(0);
}

instance instance::find_first_descendant(const std::string& target_name) const {
    if (!address || target_name.empty())
        return instance(0);

    for (const auto& child : children()) {
        if (!child.address)
            continue;
        if (child.name() == target_name)
            return child;

        const instance nested = child.find_first_descendant(target_name);
        if (nested.address)
            return nested;
    }
    return instance(0);
}

uintptr_t instance::primitive() const {
    if (!address)
        return 0;
    return memory::read<uintptr_t>(address + offsets::primitive);
}

Vector3 instance::position() const {
    const auto prim = primitive();
    if (!prim)
        return Vector3();
    return memory::read<Vector3>(prim + offsets::position);
}

Vector3 instance::size() const {
    const auto prim = primitive();
    if (!prim)
        return Vector3();
    return memory::read<Vector3>(prim + offsets::size);
}


bool instance::rotation(float out[9]) const {
    const auto prim = primitive();
    if (!prim || !out)
        return false;
    return memory::read_raw(prim + offsets::rotation, out, sizeof(float) * 9);
}



Vector3 instance::up() const {
    float rot[9]{};
    if (!rotation(rot))
        return Vector3(0.0f, 1.0f, 0.0f);

    Vector3 up(rot[1], rot[4], rot[7]);
    const float mag = up.magnitude();
    if (mag < 1e-6f)
        return Vector3(0.0f, 1.0f, 0.0f);
    return up * (1.0f / mag);
}

Vector3 instance::velocity() const {
    const auto prim = primitive();
    if (!prim)
        return Vector3();
    return memory::read<Vector3>(prim + offsets::velocity);
}

