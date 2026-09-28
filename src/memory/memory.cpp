#include "memory.hpp"
#include "../globals.hpp"
#include <cstdint>
#include <cstring>
#include <vector>

namespace memory
{
    namespace detail
    {
        using nt_read_t = NTSTATUS(WINAPI*)(HANDLE, PVOID, PVOID, ULONG, PULONG);
        using nt_write_t = NTSTATUS(WINAPI*)(HANDLE, PVOID, PVOID, ULONG, PULONG);





        uint32_t resolve_ssn(const char* name)
        {
            const HMODULE ntdll = GetModuleHandleA("ntdll.dll");
            if (!ntdll)
                return 0;

            const auto proc = reinterpret_cast<const uint8_t*>(GetProcAddress(ntdll, name));
            if (!proc)
                return 0;

            if (proc[0] != 0x4C || proc[1] != 0x8B || proc[2] != 0xD1 || proc[3] != 0xB8)
                return 0;

            uint32_t ssn = 0;
            std::memcpy(&ssn, proc + 4, sizeof(ssn));
            return ssn;
        }



        void* make_stub(uint32_t ssn)
        {
            uint8_t code[] = {
                0x4C, 0x8B, 0xD1,
                0xB8, 0x00, 0x00, 0x00, 0x00,
                0x0F, 0x05,
                0xC3
            };
            std::memcpy(code + 4, &ssn, sizeof(ssn));

            void* exec = VirtualAlloc(nullptr, sizeof(code), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (!exec)
                return nullptr;

            std::memcpy(exec, code, sizeof(code));
            return exec;
        }

        struct syscalls
        {
            nt_read_t read = nullptr;
            nt_write_t write = nullptr;
            bool ready = false;

            syscalls()
            {
                const uint32_t r = resolve_ssn("NtReadVirtualMemory");
                const uint32_t w = resolve_ssn("NtWriteVirtualMemory");
                if (!r || !w)
                    return;

                read = reinterpret_cast<nt_read_t>(make_stub(r));
                write = reinterpret_cast<nt_write_t>(make_stub(w));
                ready = read && write;
            }
        };

        bool sys_read(uintptr_t address, void* buffer, size_t size)
        {
            if (!address || !buffer || !size || size > 0xFFFFFFFFu)
                return false;

            static syscalls sc;

            if (sc.ready)
            {
                ULONG done = 0;
                const NTSTATUS status = sc.read(globals::handle,
                    reinterpret_cast<PVOID>(address), buffer,
                    static_cast<ULONG>(size), &done);
                return status >= 0 && done == size;
            }

            SIZE_T done = 0;
            return ReadProcessMemory(globals::handle, reinterpret_cast<LPCVOID>(address),
                buffer, static_cast<SIZE_T>(size), &done) && done == size;
        }

        bool sys_write(uintptr_t address, const void* buffer, size_t size)
        {
            if (!address || !buffer || !size || size > 0xFFFFFFFFu)
                return false;

            static syscalls sc;

            if (sc.ready)
            {
                ULONG done = 0;
                const NTSTATUS status = sc.write(globals::handle,
                    reinterpret_cast<PVOID>(address), const_cast<PVOID>(buffer),
                    static_cast<ULONG>(size), &done);
                return status >= 0 && done == size;
            }

            SIZE_T done = 0;
            return WriteProcessMemory(globals::handle, reinterpret_cast<LPVOID>(address),
                buffer, static_cast<SIZE_T>(size), &done) && done == size;
        }
    }

    bool read_raw(uintptr_t address, void* buffer, size_t size)
    {
        return detail::sys_read(address, buffer, size);
    }

    bool write_raw(uintptr_t address, const void* buffer, size_t size)
    {
        return detail::sys_write(address, buffer, size);
    }
}




std::string memory::read_string(uintptr_t address)
{
    if (!address)
        return {};

    const int32_t length = read<int32_t>(address + 0x10);
    if (length <= 0 || length > 255)
        return {};

    const uintptr_t string_address = (length >= 16) ? read<uintptr_t>(address) : address;
    if (!string_address)
        return {};

    std::vector<char> buffer(static_cast<size_t>(length) + 1, 0);
    if (!read_raw(string_address, buffer.data(), static_cast<size_t>(length)))
        return {};

    return std::string(buffer.data(), static_cast<size_t>(length));
}

std::string memory::decode_string(uintptr_t string_address)
{
    return read_string(string_address);
}

