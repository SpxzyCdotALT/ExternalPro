#include "menu.hpp"

#include <Windows.h>
#include <unordered_map>

#include "imgui/imgui.h"

#include "../globals.hpp"

namespace menu
{
    namespace
    {
        const char* listen_id = nullptr;

        const int listen_vks[] = {
            0x01, 0x02, 0x04, 0x05, 0x06, 0x08, 0x09, 0x0D, 0x10, 0x11, 0x12, 0x14, 0x1B,
            0x20, 0x21, 0x22, 0x23, 0x24, 0x2D, 0x2E,
            0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
            0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D,
            0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,
            0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
            0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B,
        };

        const char* key_name(int vk)
        {
            static char buf[16];
            switch (vk)
            {
            case 0: return "none";
            case 0x01: return "lmb";
            case 0x02: return "rmb";
            case 0x04: return "mmb";
            case 0x05: return "mb4";
            case 0x06: return "mb5";
            case 0x08: return "back";
            case 0x09: return "tab";
            case 0x0D: return "enter";
            case 0x10: return "shift";
            case 0x11: return "ctrl";
            case 0x12: return "alt";
            case 0x14: return "caps";
            case 0x1B: return "esc";
            case 0x20: return "space";
            case 0x2D: return "ins";
            case 0x2E: return "del";
            default: break;
            }
            if (vk >= 0x30 && vk <= 0x39) { buf[0] = (char)vk; buf[1] = 0; return buf; }
            if (vk >= 0x41 && vk <= 0x5A) { buf[0] = (char)(vk + 32); buf[1] = 0; return buf; }
            if (vk >= 0x60 && vk <= 0x69) { std::snprintf(buf, 16, "np%d", vk - 0x60); return buf; }
            if (vk >= 0x70 && vk <= 0x7B) { std::snprintf(buf, 16, "f%d", vk - 0x70 + 1); return buf; }
            std::snprintf(buf, 16, "0x%x", vk);
            return buf;
        }

        void keybind(const char* id, int* key, int* mode)
        {
            ImGui::PushID(id);
            const char* txt = (listen_id == id) ? "-" : key_name(*key);
            if (ImGui::Button(txt, ImVec2(70, 0)) && listen_id != id) listen_id = id;
            if (listen_id == id)
            {
                if (GetAsyncKeyState(VK_ESCAPE) & 1) listen_id = nullptr;
                else
                {
                    for (int i = 0; i < (int)(sizeof(listen_vks) / sizeof(listen_vks[0])); ++i)
                    {
                        if (GetAsyncKeyState(listen_vks[i]) & 1)
                        {
                            *key = listen_vks[i];
                            listen_id = nullptr;
                            break;
                        }
                    }
                }
            }
            if (mode && ImGui::BeginPopupContextItem("##mode"))
            {
                const char* modes[] = { "Hold", "Toggle", "Always" };
                for (int m = 0; m < 3; ++m)
                    if (ImGui::Selectable(modes[m], *mode == m)) *mode = m;
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }

        void page_combat()
        {
            if (ImGui::CollapsingHeader("Aimbot", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enabled", &globals::aimbot_enabled);
                ImGui::SameLine();
                keybind("aimbot", &globals::aim_key, &globals::aim_key_mode);
                const char* types[] = { "mouse", "camera" };
                ImGui::Combo("Type", &globals::aimbot_type, types, 2);
                const char* parts[] = { "head", "torso", "hrp", "closest" };
                ImGui::Combo("HitPart", &globals::aim_part, parts, 4);
                ImGui::Checkbox("Smoothing", &globals::smoothing_enableed);
                ImGui::SliderFloat("Smooth X", &globals::smoothing_x, 1.0f, 30.0f, "%.1f");
                ImGui::SliderFloat("Smooth Y", &globals::smoothing_y, 1.0f, 30.0f, "%.1f");
                ImGui::Checkbox("Prediction", &globals::prediction_enableed);
                ImGui::SliderFloat("Prediction X", &globals::prediction_x, 0.0f, 5.0f, "%.1f");
                ImGui::SliderFloat("Prediction Y", &globals::prediction_y, 0.0f, 5.0f, "%.1f");
                ImGui::Checkbox("Use FOV Circle", &globals::use_fov_circle);
                ImGui::Checkbox("Draw FOV Circle", &globals::draw_fov_circle);
                ImGui::SliderFloat("FOV Size", &globals::fov_circle_size, 10.0f, 800.0f, "%.0f");
                ImGui::Checkbox("Aim Tracer", &globals::aim_tracer_enabled);
                ImGui::Checkbox("Aim Highlight", &globals::aim_highlight);
            }
            if (ImGui::CollapsingHeader("Triggerbot", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enabled", &globals::triggerbot_enabled);
                ImGui::SameLine();
                keybind("triggerbot", &globals::trigger_key, &globals::trigger_key_mode);
                ImGui::SliderFloat("Hover Radius", &globals::trigger_radius, 1.0f, 40.0f, "%.0f");
                ImGui::SliderFloat("Reaction Delay", &globals::trigger_delay_ms, 0.0f, 500.0f, "%.0f");
                ImGui::Checkbox("Humanize Delay", &globals::trigger_humanize);
                ImGui::SliderFloat("Hold Time", &globals::trigger_hold_ms, 0.0f, 200.0f, "%.0f");
                ImGui::SliderFloat("Cooldown", &globals::trigger_cooldown_ms, 0.0f, 1000.0f, "%.0f");
                ImGui::SliderFloat("Max Distance", &globals::trigger_max_distance, 50.0f, 2000.0f, "%.0f");
                ImGui::Checkbox("Require Aim Lock", &globals::trigger_require_aim_lock);
            }
        }

        void page_visuals()
        {
            if (ImGui::CollapsingHeader("ESP", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enable ESP", &globals::esp_enableed);
                ImGui::Checkbox("Include Self", &globals::include_client);
                ImGui::Checkbox("Bounding Box", &globals::box_esp_enabled);
                ImGui::Combo("Box Type", &globals::box_type, "full\0corners\0");
                ImGui::Checkbox("Player Name", &globals::name_esp_enabled);
                ImGui::Combo("Name Type", &globals::name_type, "username\0display name\0");
                ImGui::Checkbox("Distance", &globals::distance_esp_enabled);
                ImGui::Combo("Distance Type", &globals::distance_type, "meters\0studs\0");
                ImGui::Checkbox("Health Bar", &globals::health_esp_enabled);
                ImGui::Checkbox("Skeleton ESP", &globals::skeleton_esp_enabled);
                ImGui::SliderFloat("Skeleton Thickness", &globals::skeleton_thickness, 1.0f, 5.0f, "%.1f");
                ImGui::Checkbox("Skeleton Outline", &globals::skeleton_outline);
            }
            if (ImGui::CollapsingHeader("Chams", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enable Chams", &globals::chams_enabled);
                ImGui::ColorEdit4("Fill Color", globals::chams_fill);
                ImGui::ColorEdit4("Outline Color", globals::chams_outline_col);
            }
        }

        void page_player()
        {
            if (ImGui::CollapsingHeader("Local Player", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enable WalkSpeed", &globals::walk_speed_enabled);
                ImGui::SliderFloat("WalkSpeed", &globals::walk_speed, 50.0f, 320.0f, "%.0f");
                ImGui::Text("Current: %.1f", globals::debug_walk_speed);
                ImGui::Checkbox("Enable JumpPower", &globals::jump_power_enabled);
                ImGui::SliderFloat("JumpPower", &globals::jump_power, 16.0f, 1000.0f, "%.0f");
                ImGui::Text("Current: %.1f", globals::debug_jump_power);
                ImGui::Checkbox("Sitting", &globals::sitting);
                ImGui::Checkbox("Enable FOV", &globals::fov_enabled);
                ImGui::SliderFloat("FOV", &globals::fov, 30.0f, 120.0f, "%.0f");
            }
            if (ImGui::CollapsingHeader("Player List", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (size_t i = 0; i < globals::player_list.size(); ++i)
                {
                    const auto& p = globals::player_list[i];
                    ImGui::Text("%s", p.name.c_str());
                    if (!p.display_name.empty())
                        ImGui::Text("%s", p.display_name.c_str());
                    ImGui::Text("%.0f, %.0f, %.0f", p.position.x, p.position.y, p.position.z);
                    ImGui::Separator();
                }
            }
        }

        void page_settings()
        {
            if (ImGui::CollapsingHeader("Menu", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Menu Key");
                ImGui::SameLine();
                keybind("menukey", &globals::menu_key, nullptr);
            }
            if (ImGui::CollapsingHeader("Info", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Players: %d", (int)globals::player_list.size());
                ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
                if (globals::aim_target_valid)
                    ImGui::Text("Locked: %s", globals::aim_target_name.c_str());
            }
        }
    }

    void style()
    {
    }

    bool key_active(int vk, int mode)
    {
        struct state { bool tog = false; };
        static std::unordered_map<int, state> states;
        if (vk == 0 || mode == 2) return true;
        bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
        if (mode == 1)
        {
            if (GetAsyncKeyState(vk) & 1) states[vk].tog = !states[vk].tog;
            return states[vk].tog;
        }
        return down;
    }

    void render(bool* open)
    {
        if (!*open) return;
        ImGui::SetNextWindowSize(ImVec2(620, 480), ImGuiCond_FirstUseEver);
        ImGui::Begin("externalpro", nullptr, ImGuiWindowFlags_None);
        const char* tabs[] = { "Combat", "Visuals", "Player", "Settings" };
        if (ImGui::BeginTabBar("##tabs"))
        {
            if (ImGui::BeginTabItem("Combat"))
            {
                page_combat();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Visuals"))
            {
                page_visuals();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Player"))
            {
                page_player();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Settings"))
            {
                page_settings();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    bool listening()
    {
        return listen_id != nullptr;
    }
}
