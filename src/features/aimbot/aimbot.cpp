#include "aimbot.hpp"

#include <Windows.h>
#include <cfloat>
#include <cmath>

#include "imgui/imgui.h"

#include "../../instance/datamodel.hpp"
#include "../../globals.hpp"
#include "../../memory/memory.hpp"
#include "../../offsets.hpp"
#include "../../ui/menu.hpp"

namespace features::aimbot
{
    namespace
    {
        constexpr ImU32 white = IM_COL32(255, 255, 255, 255);
        constexpr ImU32 black = IM_COL32(0, 0, 0, 255);




        struct cam_rot3
        {
            float m[9]{};
        };

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


        ImVec2 anchor()
        {
            const ImVec2 size = ImGui::GetIO().DisplaySize;
            return ImVec2(size.x * 0.5f, size.y * 0.5f);
        }

        bool key_down()
        {
            return menu::key_active(globals::aim_key, globals::aim_key_mode);
        }

        bool foreground_is_game()
        {
            return GetForegroundWindow() == FindWindowA(nullptr, "Roblox");
        }



        void smooth_divisors(float& sx, float& sy)
        {
            if (!globals::smoothing_enableed)
            {
                sx = 5.0f;
                sy = 5.0f;
                return;
            }

            sx = globals::smoothing_x;
            sy = globals::smoothing_y;
            if (sx < 0.1f) sx = 0.1f;
            if (sy < 0.1f) sy = 0.1f;
        }



        inline float mouse_acc_x = 0.0f;
        inline float mouse_acc_y = 0.0f;

        void reset_mouse_accumulator()
        {
            mouse_acc_x = 0.0f;
            mouse_acc_y = 0.0f;
        }

        void clear_lock()
        {
            globals::aim_target_name.clear();
            globals::aim_target_valid = false;
            reset_mouse_accumulator();
        }




        Vector3 predict(const player_info& player, const Vector3& world, const Vector3& cam_pos)
        {
            if (!globals::prediction_enableed)
                return world;

            const float lead = world.distance(cam_pos) / 1200.0f;

            Vector3 out = world;
            out.x += player.velocity.x * lead * globals::prediction_x;
            out.z += player.velocity.z * lead * globals::prediction_x;
            out.y += player.velocity.y * lead * globals::prediction_y;
            return out;
        }

        struct lock
        {
            bool valid = false;
            std::string name;
            Vector3 world{};
            Vector2 screen{};
        };





        lock select_target(const view_data& view, const ImVec2& anchor_point)
        {
            lock best;

            const float radius = globals::use_fov_circle
                ? (globals::fov_circle_size < 1.0f ? 1.0f : globals::fov_circle_size)
                : FLT_MAX;

            float best_dist = FLT_MAX;

            for (const player_info& player : globals::player_list)
            {
                if (player.name.empty())
                    continue;
                if (player.name == globals::local_player_name)
                    continue;



                uint8_t slots[16]{};
                int slot_count = 0;

                if (globals::aim_part == 1)
                {
                    slots[slot_count++] = J_TORSO;
                }
                else if (globals::aim_part == 2)
                {
                    slots[slot_count++] = J_ROOT;
                }
                else if (globals::aim_part == 3)
                {
                    for (uint8_t i = 0; i < 16; ++i)
                        if (player.joint_mask & (1u << i))
                            slots[slot_count++] = i;
                }
                else
                {
                    slots[slot_count++] = J_HEAD;
                    slots[slot_count++] = J_TORSO;

                }




                bool picked = false;
                Vector3 picked_world{};
                Vector2 picked_screen{};
                float picked_dist = FLT_MAX;

                for (int i = 0; i < slot_count; ++i)
                {
                    if (!(player.joint_mask & (1u << slots[i])))
                        continue;

                    const Vector3 world = predict(player, player.joints[slots[i]], view.cam_pos);

                    Vector2 screen{};
                    if (!world_to_screen(world, view, screen))
                        continue;

                    const float dx = screen.x - anchor_point.x;
                    const float dy = screen.y - anchor_point.y;
                    const float dist = sqrtf(dx * dx + dy * dy);

                    if (globals::aim_part == 3)
                    {
                        if (dist <= radius && (!picked || dist < picked_dist))
                        {
                            picked = true;
                            picked_world = world;
                            picked_screen = screen;
                            picked_dist = dist;
                        }
                        continue;
                    }



                    if (dist <= radius)
                    {
                        picked = true;
                        picked_world = world;
                        picked_screen = screen;
                        picked_dist = dist;
                        break;
                    }
                }

                if (picked && picked_dist < best_dist)
                {
                    best_dist = picked_dist;
                    best.valid = true;
                    best.name = player.name;
                    best.world = picked_world;
                    best.screen = picked_screen;
                }
            }

            return best;
        }







        void apply_mouse(const Vector2& target_screen, const ImVec2& anchor_point)
        {
            const float dx = target_screen.x - anchor_point.x;
            const float dy = target_screen.y - anchor_point.y;
            const float dist = sqrtf(dx * dx + dy * dy);

            if (dist <= 0.5f)
            {
                reset_mouse_accumulator();
                return;
            }

            float sx = 0.0f, sy = 0.0f;
            smooth_divisors(sx, sy);

            LONG mx = 0, my = 0;

            if (dist <= 2.0f)
            {
                mx = static_cast<LONG>(std::lround(dx));
                my = static_cast<LONG>(std::lround(dy));
                reset_mouse_accumulator();
            }
            else
            {
                mouse_acc_x += dx / sx;
                mouse_acc_y += dy / sy;

                float step_x = truncf(mouse_acc_x);
                float step_y = truncf(mouse_acc_y);



                if (step_x > 64.0f) step_x = 64.0f;
                if (step_x < -64.0f) step_x = -64.0f;
                if (step_y > 64.0f) step_y = 64.0f;
                if (step_y < -64.0f) step_y = -64.0f;

                mouse_acc_x -= step_x;
                mouse_acc_y -= step_y;

                mx = static_cast<LONG>(step_x);
                my = static_cast<LONG>(step_y);
            }

            if (mx == 0 && my == 0)
                return;

            INPUT in{};
            in.type = INPUT_MOUSE;
            in.mi.dx = mx;
            in.mi.dy = my;
            in.mi.dwFlags = MOUSEEVENTF_MOVE;
            SendInput(1, &in, sizeof(in));
        }




        void apply_camera(const Vector3& target_world, const Vector3& cam_pos)
        {
            if (!datamodel::camera.address)
                return;

            Vector3 want = target_world - cam_pos;
            if (want.squared() < 1e-6f)
                return;
            want = want.normalize();

            const cam_rot3 cur = memory::read<cam_rot3>(
                datamodel::camera.address + offsets::camera_rotation);
            Vector3 cur_look(-cur.m[2], -cur.m[5], -cur.m[8]);
            if (cur_look.squared() < 1e-6f)
                cur_look = want;
            else
                cur_look = cur_look.normalize();

            float sx = 0.0f, sy = 0.0f;
            smooth_divisors(sx, sy);

            const float tx = 1.0f / sx;
            const float ty = 1.0f / sy;
            const float tz = (tx + ty) * 0.5f;

            Vector3 look(
                cur_look.x + (want.x - cur_look.x) * tx,
                cur_look.y + (want.y - cur_look.y) * ty,
                cur_look.z + (want.z - cur_look.z) * tz);
            if (look.squared() < 1e-6f)
                return;
            look = look.normalize();

            const Vector3 world_up(0.0f, 1.0f, 0.0f);
            Vector3 right = look.cross(world_up);
            if (right.squared() < 1e-6f)
                right = Vector3(1.0f, 0.0f, 0.0f);
            right = right.normalize();
            const Vector3 up = right.cross(look);
            const Vector3 back = look * -1.0f;

            const cam_rot3 rot{
                right.x, up.x, back.x,
                right.y, up.y, back.y,
                right.z, up.z, back.z,
            };
            memory::write<cam_rot3>(
                datamodel::camera.address + offsets::camera_rotation, rot);
        }
    }

    void tick()
    {
        if (!globals::aimbot_enabled)
        {
            clear_lock();
            return;
        }

        if (!key_down() || !foreground_is_game())
        {
            clear_lock();
            return;
        }

        const view_data view = fetch_view();
        if (!view.valid)
        {
            clear_lock();
            return;
        }

        const ImVec2 anchor_point = anchor();
        const lock target = select_target(view, anchor_point);

        if (!target.valid)
        {
            clear_lock();
            return;
        }

        globals::aim_target_name = target.name;
        globals::aim_target_screen = target.screen;
        globals::aim_target_valid = true;

        if (globals::aimbot_type == 0)
        {
            apply_mouse(target.screen, anchor_point);
        }
        else if (globals::aimbot_type == 1)
        {
            apply_camera(target.world, view.cam_pos);
        }


    }

    void render()
    {
        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        if (!draw)
            return;

        const ImVec2 anchor_point = anchor();

        if (globals::draw_fov_circle)
        {
            float radius = globals::fov_circle_size;
            if (radius < 1.0f) radius = 1.0f;
            draw->AddCircle(anchor_point, radius + 1.0f, black, 64, 1.0f);
            draw->AddCircle(anchor_point, radius, white, 64, 1.0f);
        }


        if (globals::aim_tracer_enabled && globals::aim_target_valid)
        {
            const ImVec2 to(globals::aim_target_screen.x, globals::aim_target_screen.y);
            draw->AddLine(anchor_point, to, black, 3.0f);
            draw->AddLine(anchor_point, to, white, 1.0f);
        }
    }
}

