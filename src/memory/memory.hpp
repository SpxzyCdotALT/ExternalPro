#pragma once

#include <Windows.h>
#include <string>
#include "../globals.hpp"

namespace memory
{
    namespace detail
    {



        bool sys_read(uintptr_t address, void* buffer, size_t size);
        bool sys_write(uintptr_t address, const void* buffer, size_t size);
    }

    template<typename T>
    T read(uintptr_t address)
    {
        T value{};
        detail::sys_read(address, &value, sizeof(T));
        return value;
    }

    template<typename T>
    bool write(uintptr_t address, const T& buffer)
    {
        return detail::sys_write(address, &buffer, sizeof(T));
    }

    template<typename T>
    T read_module(uintptr_t address)
    {
        T value{};
        detail::sys_read(address + globals::base, &value, sizeof(T));
        return value;
    }


    bool read_raw(uintptr_t address, void* buffer, size_t size);
    bool write_raw(uintptr_t address, const void* buffer, size_t size);

    std::string read_string(uintptr_t address);


    std::string decode_string(uintptr_t string_address);
}

