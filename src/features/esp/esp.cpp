#include "esp.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

#include "imgui/imgui.h"

#include "../../instance/datamodel.hpp"
#include "../../globals.hpp"
#include "../../memory/memory.hpp"
#include "../../offsets.hpp"
#include "../chams/chams.hpp"

namespace features::esp
{
    namespace
    {
        constexpr ImU32 white = IM_COL32(255, 255, 255, 255);
        constexpr ImU32 black = IM_COL32(0, 0, 0, 255);
        constexpr ImU32 dim = IM_COL32(0, 0, 0, 160);
        constexpr ImU32 red = IM_COL32(255, 45, 45, 255);

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

        void text_centered(ImDrawList* draw, const char* text, float center_x, float y, ImU32 color)
        {
            const ImVec2 size = ImGui::CalcTextSize(text);
            const ImVec2 pos(center_x - size.x * 0.5f, y);
            draw->AddText(ImVec2(pos.x + 1, pos.y + 1), black, text);
            draw->AddText(pos, color, text);
        }

        void line(ImDrawList* draw, const ImVec2& a, const ImVec2& b, ImU32 color)
        {
            draw->AddLine(a, b, black, 3.0f);
            draw->AddLine(a, b, color, 1.0f);
        }



        void skeleton_line(ImDrawList* draw, const view_data& v, const Vector3& a_world, const Vector3& b_world, ImU32 color)
        {
            Vector2 sa{}, sb{};
            if (!world_to_screen(a_world, v, sa) || !world_to_screen(b_world, v, sb))
                return;

            float thick = globals::skeleton_thickness;
            if (thick < 1.0f) thick = 1.0f;

            const ImVec2 a(sa.x, sa.y), b(sb.x, sb.y);
            if (globals::skeleton_outline)
                draw->AddLine(a, b, black, thick + 2.0f);
            draw->AddLine(a, b, color, thick);
        }

        Vector3 lerp3(const Vector3& a, const Vector3& b, float t)
        {
            return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
        }




        Vector3 point_at_height(const Vector3& top, const Vector3& bottom, const Vector3& ref)
        {
            const float dy = top.y - bottom.y;
            if (fabsf(dy) < 0.001f) return lerp3(top, bottom, 0.5f);
            float t = (top.y - ref.y) / dy;
            if (t < 0.0f) t = 0.0f;
            else if (t > 1.0f) t = 1.0f;
            return lerp3(top, bottom, t);
        }

        void draw_box(ImDrawList* draw, const ImVec2& top_left, const ImVec2& bottom_right, ImU32 color)
        {
            draw->AddRect(ImVec2(top_left.x + 1, top_left.y + 1), ImVec2(bottom_right.x + 1, bottom_right.y + 1), black, 0.0f, 0, 1.0f);
            draw->AddRect(ImVec2(top_left.x - 1, top_left.y - 1), ImVec2(bottom_right.x - 1, bottom_right.y - 1), black, 0.0f, 0, 1.0f);
            draw->AddRect(top_left, bottom_right, color, 0.0f, 0, 1.0f);
        }

        void draw_corners(ImDrawList* draw, const ImVec2& top_left, const ImVec2& bottom_right, ImU32 color)
        {
            const float arm = (bottom_right.x - top_left.x) * 0.3f;
            const float x1 = top_left.x, y1 = top_left.y;
            const float x2 = bottom_right.x, y2 = bottom_right.y;

            line(draw, ImVec2(x1, y1), ImVec2(x1 + arm, y1), color);
            line(draw, ImVec2(x1, y1), ImVec2(x1, y1 + arm), color);
            line(draw, ImVec2(x2, y1), ImVec2(x2 - arm, y1), color);
            line(draw, ImVec2(x2, y1), ImVec2(x2, y1 + arm), color);
            line(draw, ImVec2(x1, y2), ImVec2(x1 + arm, y2), color);
            line(draw, ImVec2(x1, y2), ImVec2(x1, y2 - arm), color);
            line(draw, ImVec2(x2, y2), ImVec2(x2 - arm, y2), color);
            line(draw, ImVec2(x2, y2), ImVec2(x2, y2 - arm), color);
        }
    }

    void render()
    {
        if (!globals::esp_enableed || globals::player_list.empty())
            return;

        const view_data view = fetch_view();
        if (!view.valid)
            return;

        ImDrawList* draw = ImGui::GetBackgroundDrawList();

        for (const player_info& player : globals::player_list)
        {
            if (player.name.empty())
                continue;
            if (!globals::include_client && player.name == globals::local_player_name)
                continue;


            ImVec2 joint_2d[16]{};
            uint32_t projected_mask = 0;

            ImVec2 top_left{}, bottom_right{};
            bool has_points = false;

            for (int i = 0; i < 16; ++i)
            {
                if (!(player.joint_mask & (1u << i)))
                    continue;

                Vector2 projected{};
                if (!world_to_screen(player.joints[i], view, projected))
                    continue;

                joint_2d[i] = ImVec2(projected.x, projected.y);
                projected_mask |= (1u << i);

                if (!has_points)
                {
                    top_left = bottom_right = joint_2d[i];
                    has_points = true;
                }
                else
                {
                    if (projected.x < top_left.x) top_left.x = projected.x;
                    if (projected.y < top_left.y) top_left.y = projected.y;
                    if (projected.x > bottom_right.x) bottom_right.x = projected.x;
                    if (projected.y > bottom_right.y) bottom_right.y = projected.y;
                }
            }

            if (!has_points)
                continue;

            const bool r15 = (player.joint_mask & (1u << J_TORSO_LOWER)) != 0;




            float px_per_stud = 0.0f;
            const uint32_t head_torso_bits = (1u << J_HEAD) | (1u << J_TORSO);
            if ((projected_mask & head_torso_bits) == head_torso_bits)
            {
                const float dx = joint_2d[J_HEAD].x - joint_2d[J_TORSO].x;
                const float dy = joint_2d[J_HEAD].y - joint_2d[J_TORSO].y;
                px_per_stud = sqrtf(dx * dx + dy * dy) / 1.5f;
            }

            if (!r15 && px_per_stud > 0.0f)
            {
                const uint8_t feet[] = { J_FOOT_L, J_FOOT_R };
                for (uint8_t slot : feet)
                {
                    if (!(projected_mask & (1u << slot)))
                        continue;
                    const float toe = joint_2d[slot].y + px_per_stud;
                    if (toe > bottom_right.y)
                        bottom_right.y = toe;
                }
            }



            float pad_top = (bottom_right.y - top_left.y) * 0.08f;
            float pad_bottom = (bottom_right.y - top_left.y) * 0.04f;
            float pad_side = (bottom_right.x - top_left.x) * 0.06f;
            if (pad_top < 1.0f) pad_top = 1.0f;
            if (pad_bottom < 1.0f) pad_bottom = 1.0f;
            if (pad_side < 1.0f) pad_side = 1.0f;

            top_left.x -= pad_side;
            top_left.y -= pad_top;
            bottom_right.x += pad_side;
            bottom_right.y += pad_bottom;

            const float height = bottom_right.y - top_left.y;
            if (height <= 0.0f)
                continue;

            const float center_x = (top_left.x + bottom_right.x) * 0.5f;



            const bool targeted = globals::aim_highlight && globals::aim_target_valid &&
                player.name == globals::aim_target_name;
            const ImU32 accent = targeted ? red : white;

            auto skel = [&](const Vector3& a, const Vector3& b) {
                skeleton_line(draw, view, a, b, accent);
            };



            if (globals::chams_enabled)
            {
                chams::pieces hulls;
                hulls.reserve(16);

                for (uint8_t slot = 0; slot < 16; ++slot)
                {
                    if (!(player.joint_mask & (1u << slot)))
                        continue;
                    if (slot == J_ROOT)
                        continue;

                    const Vector3 half = player.joint_sizes[slot] * 0.5f;
                    if (!(half.x > 0.0f) || !(half.y > 0.0f) || !(half.z > 0.0f))
                        continue;

                    const Vector3& center = player.joints[slot];
                    const Vector3& ax = player.joint_axes[slot][0];
                    const Vector3& ay = player.joint_axes[slot][1];
                    const Vector3& az = player.joint_axes[slot][2];

                    ImVec2 corners[8]{};
                    bool full = true;
                    int n = 0;
                    for (int ix = -1; ix <= 1 && full; ix += 2)
                        for (int iy = -1; iy <= 1 && full; iy += 2)
                            for (int iz = -1; iz <= 1 && full; iz += 2)
                            {
                                const Vector3 world = {
                                    center.x + ax.x * half.x * ix + ay.x * half.y * iy + az.x * half.z * iz,
                                    center.y + ax.y * half.x * ix + ay.y * half.y * iy + az.y * half.z * iz,
                                    center.z + ax.z * half.x * ix + ay.z * half.y * iy + az.z * half.z * iz,
                                };
                                Vector2 screen{};
                                if (!world_to_screen(world, view, screen))
                                {
                                    full = false;
                                    break;
                                }
                                corners[n++] = ImVec2(screen.x, screen.y);
                            }

                    if (!full)
                        continue;

                    auto hull = chams::convex_hull(std::vector<ImVec2>(corners, corners + 8));
                    if (hull.size() >= 3)
                        hulls.push_back(std::move(hull));
                }



                chams::pieces clipped;
                clipped.reserve(hulls.size() * 2);
                for (size_t i = 0; i < hulls.size(); ++i)
                {
                    if (hulls[i].size() < 3)
                        continue;
                    chams::pieces pieces{ hulls[i] };
                    for (size_t j = 0; j < i && !pieces.empty(); ++j)
                    {
                        if (hulls[j].size() < 3)
                            continue;
                        chams::pieces next;
                        for (auto& piece : pieces)
                            chams::subtract_poly(std::move(piece), hulls[j], next);
                        pieces = std::move(next);
                    }
                    for (auto& piece : pieces)
                        if (piece.size() >= 3)
                            clipped.push_back(std::move(piece));
                }

                if (!clipped.empty())
                {
                    const float* fc = globals::chams_fill;
                    const float* oc = globals::chams_outline_col;
                    ImU32 fill = IM_COL32(
                        (int)(fc[0] * 255.0f), (int)(fc[1] * 255.0f),
                        (int)(fc[2] * 255.0f), (int)(fc[3] * 255.0f));
                    ImU32 outline = IM_COL32(
                        (int)(oc[0] * 255.0f), (int)(oc[1] * 255.0f),
                        (int)(oc[2] * 255.0f), (int)(oc[3] * 255.0f));
                    if (targeted)
                    {
                        fill = red;
                        outline = red;
                    }
                    chams::draw_flat(draw, clipped, hulls, fill, outline);
                }
            }

            if (globals::skeleton_esp_enabled)
            {



                const auto has = [&](uint8_t slot) {
                    return (player.joint_mask & (1u << slot)) != 0;
                };

                auto bone = [&](uint8_t pa, uint8_t pb) {
                    if (!has(pa) || !has(pb))
                        return;
                    skel( player.joints[pa], player.joints[pb]);
                };

                if (!r15)
                {



                    if (has(J_TORSO))
                    {
                        const Vector3 shoulder_c = lerp3(
                            player.joint_tops[J_TORSO], player.joint_bottoms[J_TORSO], 0.18f);
                        skel( shoulder_c, player.joint_bottoms[J_TORSO]);



                        if (has(J_HEAD))
                            skel( player.joints[J_HEAD], shoulder_c);

                        const uint8_t arms[] = { J_HAND_L, J_HAND_R };
                        for (uint8_t arm : arms)
                        {
                            if (!has(arm))
                                continue;
                            const Vector3 joint = point_at_height(
                                player.joint_tops[arm], player.joint_bottoms[arm], shoulder_c);
                            skel( shoulder_c, joint);
                            skel( joint, player.joint_bottoms[arm]);
                        }

                        const uint8_t legs[] = { J_FOOT_L, J_FOOT_R };
                        for (uint8_t leg : legs)
                        {
                            if (!has(leg))
                                continue;
                            skel(
                                player.joint_bottoms[J_TORSO], player.joint_tops[leg]);
                            skel(
                                player.joint_tops[leg], player.joint_bottoms[leg]);
                        }
                    }
                }
                else
                {



                    auto shoulder_bone = [&](uint8_t arm) {
                        if (!has(J_TORSO) || !has(arm))
                        {
                            bone(J_TORSO, arm);
                            return;
                        }
                        const Vector3 shoulder_c = lerp3(
                            player.joint_tops[J_TORSO], player.joint_bottoms[J_TORSO], 0.15f);
                        const Vector3 joint = point_at_height(
                            player.joint_tops[arm], player.joint_bottoms[arm], shoulder_c);
                        skel( shoulder_c, joint);
                        skel( joint, player.joints[arm]);
                    };

                    bone(J_HEAD, J_TORSO);
                    bone(J_TORSO, J_TORSO_LOWER);

                    shoulder_bone(J_UPPER_ARM_L);
                    bone(J_UPPER_ARM_L, J_LOWER_ARM_L);
                    bone(J_LOWER_ARM_L, J_HAND_L);

                    shoulder_bone(J_UPPER_ARM_R);
                    bone(J_UPPER_ARM_R, J_LOWER_ARM_R);
                    bone(J_LOWER_ARM_R, J_HAND_R);

                    bone(J_TORSO_LOWER, J_UPPER_LEG_L);
                    bone(J_UPPER_LEG_L, J_LOWER_LEG_L);
                    bone(J_LOWER_LEG_L, J_FOOT_L);

                    bone(J_TORSO_LOWER, J_UPPER_LEG_R);
                    bone(J_UPPER_LEG_R, J_LOWER_LEG_R);
                    bone(J_LOWER_LEG_R, J_FOOT_R);
                }
            }

            if (globals::box_esp_enabled)
            {
                if (globals::box_type == 1)
                    draw_corners(draw, top_left, bottom_right, accent);
                else
                    draw_box(draw, top_left, bottom_right, accent);
            }

            if (globals::name_esp_enabled)
            {
                const std::string& label = (globals::name_type != 0 && !player.display_name.empty())
                    ? player.display_name
                    : player.name;
                text_centered(draw, label.c_str(), center_x, top_left.y - ImGui::GetFontSize() - 2.0f, accent);
            }

            if (globals::distance_esp_enabled)
            {
                const float studs = player.position.distance(view.cam_pos);

                char buffer[32];
                if (globals::distance_type == 0)
                    std::snprintf(buffer, sizeof(buffer), "%.0fm", studs * 0.28f);
                else
                    std::snprintf(buffer, sizeof(buffer), "%.0f studs", studs);

                text_centered(draw, buffer, center_x, bottom_right.y + 2.0f, accent);
            }

            if (globals::health_esp_enabled)
            {
                float fraction = player.max_health > 0.0f ? player.health / player.max_health : 0.0f;
                fraction = fraction < 0.0f ? 0.0f : (fraction > 1.0f ? 1.0f : fraction);

                const ImVec2 bar_min(top_left.x - 5.0f, top_left.y);
                const ImVec2 bar_max(top_left.x - 2.0f, bottom_right.y);

                draw->AddRectFilled(bar_min, bar_max, dim);
                draw->AddRectFilled(ImVec2(bar_min.x, bar_max.y - height * fraction), bar_max, white);
            }
        }
    }
}

