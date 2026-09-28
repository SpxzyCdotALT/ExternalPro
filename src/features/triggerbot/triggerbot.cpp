#include "triggerbot.hpp"

#include <Windows.h>
#include <cstdlib>

#include "imgui/imgui.h"

#include "../../instance/datamodel.hpp"
#include "../../globals.hpp"
#include "../../memory/memory.hpp"
#include "../../offsets.hpp"
#include "../../ui/menu.hpp"

namespace features::triggerbot
{
    namespace
    {
        struct view_data
        {
            Matrix4 matrix{};
            Vector2 dims{};
            Vector3 cam_pos{};
            bool valid = false;
        };

        view_data fetch_view()
        {
            view_data v;

            const uintptr_t visual = memory::read<uintptr_t>(globals::base + offsets::visualengine_pointer);
            if (!visual)
                return v;

            v.matrix = memory::read<Matrix4>(visual + offsets::viewmatrix);
            v.dims = memory::read<Vector2>(visual + offsets::visual_dimensions);
            if (v.dims.x <= 0.0f || v.dims.y <= 0.0f)
                return v;

            if (datamodel::camera.address)
                v.cam_pos = memory::read<Vector3>(datamodel::camera.address + offsets::camera_position);

            v.valid = true;
            return v;
        }

        bool world_to_screen(const Vector3& world, const view_data& v, Vector2& out)
        {
            const float* m = v.matrix.data;

            const float w = world.x * m[12] + world.y * m[13] + world.z * m[14] + m[15];
            if (w < 0.1f)
                return false;

            const float inv = 1.0f / w;
            const float nx = (world.x * m[0] + world.y * m[1] + world.z * m[2] + m[3]) * inv;
            const float ny = (world.x * m[4] + world.y * m[5] + world.z * m[6] + m[7]) * inv;

            out.x = (v.dims.x * 0.5f * nx) + (v.dims.x * 0.5f);
            out.y = -(v.dims.y * 0.5f * ny) + (v.dims.y * 0.5f);

            return out.x >= 0.0f && out.x <= v.dims.x && out.y >= 0.0f && out.y <= v.dims.y;
        }

        void mouse_down()
        {
            INPUT in{};
            in.type = INPUT_MOUSE;
            in.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            SendInput(1, &in, sizeof(in));
        }

        void mouse_up()
        {
            INPUT in{};
            in.type = INPUT_MOUSE;
            in.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &in, sizeof(in));
        }


        inline bool held = false;
        inline unsigned long long hover_start = 0;
        inline float pending_delay = 0.0f;
        inline unsigned long long shot_time = 0;
        inline unsigned long long last_shot = 0;

        void release()
        {
            if (held)
            {
                mouse_up();
                held = false;
                last_shot = GetTickCount64();
            }
            hover_start = 0;
        }

        bool key_down()
        {
            return menu::key_active(globals::trigger_key, globals::trigger_key_mode);
        }






        bool crosshair_on_target(const view_data& view, const ImVec2& center)
        {
            if (globals::trigger_require_aim_lock && !globals::aim_target_valid)
                return false;

            float radius = globals::trigger_radius;
            if (radius < 1.0f) radius = 1.0f;

            const float max_dist = globals::trigger_max_distance;
            const float max_sq = max_dist * max_dist;

            const uint8_t slots[] = { J_HEAD, J_TORSO, J_ROOT };

            for (const player_info& player : globals::player_list)
            {
                if (player.name.empty())
                    continue;
                if (player.name == globals::local_player_name)
                    continue;


                const Vector3 to_player = player.position - view.cam_pos;
                if (to_player.squared() > max_sq)
                    continue;


                for (uint8_t slot : slots)
                {
                    if (!(player.joint_mask & (1u << slot)))
                        continue;

                    Vector2 screen{};
                    if (!world_to_screen(player.joints[slot], view, screen))
                        continue;

                    const float dx = screen.x - center.x;
                    const float dy = screen.y - center.y;
                    if (dx * dx + dy * dy <= radius * radius)
                        return true;
                }
            }

            return false;
        }
    }

    void tick()
    {
        if (!globals::triggerbot_enabled || !key_down() ||
            GetForegroundWindow() != FindWindowA(nullptr, "Roblox"))
        {
            release();
            return;
        }

        const view_data view = fetch_view();
        if (!view.valid)
        {
            release();
            return;
        }

        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const ImVec2 center(display.x * 0.5f, display.y * 0.5f);

        const unsigned long long now = GetTickCount64();

        if (!crosshair_on_target(view, center))
        {

            release();
            return;
        }

        if (hover_start == 0)
        {
            hover_start = now;



            float delay = globals::trigger_delay_ms;
            if (delay < 0.0f) delay = 0.0f;
            if (globals::trigger_humanize && delay > 0.0f)
            {
                const float jitter = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 0.5f;
                delay *= (1.0f + jitter);
            }
            pending_delay = delay;
        }

        if (held)
        {

            if (now - shot_time >= static_cast<unsigned long long>(globals::trigger_hold_ms))
            {
                mouse_up();
                held = false;
                last_shot = now;
            }
            return;
        }

        if (now - hover_start < static_cast<unsigned long long>(pending_delay))
            return;

        if (now - last_shot < static_cast<unsigned long long>(globals::trigger_cooldown_ms))
            return;

        mouse_down();
        held = true;
        shot_time = now;
    }
}

