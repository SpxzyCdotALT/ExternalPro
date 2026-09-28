#include <Windows.h>
#include <chrono>
#include <iostream>
#include <thread>
#include "src/instance/datamodel.hpp"
#include "src/features/player/player.hpp"
#include "src/overlay/overlay.hpp"

int main()
{
    // wait for roblox instead of dying when it isn't open yet
    while (!datamodel::initialize())
        std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "datamodel   " << datamodel::handle << std::endl;
    std::cout << "players     " << datamodel::players << std::endl;
    std::cout << "workspace   " << datamodel::workspace << std::endl;
    std::cout << "camera      " << datamodel::camera << std::endl;

    const instance local = datamodel::local_player();
    std::cout << "localplayer " << local << " (" << local.name() << ")" << std::endl;

    std::thread(&overlay_manager::initialize_overlay, overlay_instance.get()).detach();
    std::thread(features::player::initialize).detach();

    // features + overlay own their threads, main just idles
    for (;;)
        std::this_thread::sleep_for(std::chrono::seconds(10));
}
