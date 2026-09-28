#pragma once

#include <Windows.h>
#include <Psapi.h>

#include "instance.hpp"
#include "../memory/memory.hpp"
#include "../offsets.hpp"



namespace datamodel
{
    inline instance handle{ 0 };
    inline instance players{ 0 };
    inline instance workspace{ 0 };
    inline instance camera{ 0 };


    inline bool attach()
    {
        globals::rbx = FindWindowA(nullptr, "Roblox");
        if (!globals::rbx)
            return false;

        GetWindowThreadProcessId(globals::rbx, &globals::pid);
        globals::handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, globals::pid);
        if (!globals::handle)
            return false;

        HMODULE modules[1024];
        DWORD needed = 0;

        if (EnumProcessModules(globals::handle, modules, sizeof(modules), &needed))
            globals::base = reinterpret_cast<uintptr_t>(modules[0]);

        return globals::base != 0;
    }


    inline bool refresh()
    {
        handle = instance(memory::read<uintptr_t>(memory::read_module<uintptr_t>(offsets::fake_datamodel) + offsets::real_datamodel));
        if (!handle.address)
            return false;


        players = handle.find_first_child_of_class("Players");
        if (!players.address)
            players = handle.find_first_child("Players");

        workspace = handle.find_first_child_of_class("Workspace");
        if (!workspace.address)
            workspace = handle.find_first_child("Workspace");
        if (!workspace.address)
            return false;



        camera = instance(memory::read<uintptr_t>(workspace.address + offsets::current_camera));
        if (!camera.address)
        {
            camera = workspace.find_first_child_of_class("Camera");
            if (!camera.address)
                camera = workspace.find_first_child("Camera");
        }

        return players.address != 0;
    }

    inline bool initialize()
    {
        return attach() && refresh();
    }


    inline void ensure()
    {
        if (!players.address || !workspace.address)
            refresh();
    }

    inline instance local_player()
    {
        if (!players.address)
            return instance(0);
        return instance(memory::read<uintptr_t>(players.address + offsets::local_player));
    }

    inline instance local_character()
    {
        const instance self = local_player();
        if (!self.address)
            return instance(0);


        const auto character_addr = memory::read<uintptr_t>(self.address + offsets::model_instance);
        if (character_addr)
            return instance(character_addr);

        if (!workspace.address)
            return instance(0);
        return workspace.find_first_child(self.name());
    }

    inline instance local_humanoid()
    {
        const instance character = local_character();
        if (!character.address)
            return instance(0);
        return character.find_first_child("Humanoid");
    }
}

