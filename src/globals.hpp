#pragma once

#include <Windows.h>
#include <cstdint>
#include <iostream>
#include <vector>

#include "structures/structures.hpp"

enum PlayerJoint : uint8_t
{
    J_HEAD = 0, J_ROOT, J_TORSO, J_TORSO_LOWER,
    J_UPPER_ARM_L, J_LOWER_ARM_L, J_HAND_L,
    J_UPPER_ARM_R, J_LOWER_ARM_R, J_HAND_R,
    J_UPPER_LEG_L, J_LOWER_LEG_L, J_FOOT_L,
    J_UPPER_LEG_R, J_LOWER_LEG_R, J_FOOT_R
};

struct player_info {
    std::string name;
    std::string display_name;
    Vector3 position;
    Vector3 velocity;
    Vector3 joints[16];
    Vector3 joint_tops[16];
    Vector3 joint_bottoms[16];
    Vector3 joint_sizes[16];
    Vector3 joint_axes[16][3];
    uint16_t joint_mask = 0;
    float health = 0.0f;
    float max_health = 100.0f;
};

namespace globals
{
	inline DWORD pid;
    inline HANDLE handle;
    inline HWND rbx;
    inline uintptr_t base;

    inline int menu_key = 0x2D;
    inline float menu_accent[4]{ 0.549f, 0.275f, 0.863f, 1.0f };

    inline bool fov_enabled = false;
    inline float fov = 90.0f;
    inline bool walk_speed_enabled = false;
    inline float walk_speed = 16.0f;
    inline float jump_power = 50.0f;
    inline bool jump_power_enabled = false;
    inline bool sitting = false;


    inline float debug_walk_speed = 0.0f;
    inline float debug_jump_power = 0.0f;

    inline bool aimbot_enabled = false;
    inline int aimbot_type = 0;
    inline bool draw_fov_circle = false;
    inline bool use_fov_circle= false;
    inline float fov_circle_size = 120.0f;
    inline bool smoothing_enableed = false;
    inline float smoothing_x = 5.0f;
    inline float smoothing_y = 5.0f;
    inline bool prediction_enableed = false;
    inline float prediction_x = 1.0f;
    inline float prediction_y = 1.0f;
    inline bool aim_tracer_enabled = true;
    inline bool aim_highlight = true;
    inline int aim_key = 0x02;
    inline int aim_key_mode = 0;
    inline int aim_part = 0;


    inline std::string aim_target_name;
    inline Vector2 aim_target_screen{};
    inline bool aim_target_valid = false;


    inline bool triggerbot_enabled = false;
    inline int trigger_key = 0x05;
    inline int trigger_key_mode = 0;
    inline float trigger_radius = 8.0f;
    inline float trigger_delay_ms = 80.0f;
    inline bool trigger_humanize = true;
    inline float trigger_hold_ms = 20.0f;
    inline float trigger_cooldown_ms = 150.0f;
    inline float trigger_max_distance = 500.0f;
    inline bool trigger_require_aim_lock = false;

    inline bool esp_enableed = false;
    inline bool include_client = false;
    inline bool box_esp_enabled = false;
    inline int box_type= 0;
    inline bool name_esp_enabled = false;
    inline int name_type = 0;
    inline bool distance_esp_enabled = false;
    inline int distance_type = 0;
    inline bool health_esp_enabled = false;
    inline bool skeleton_esp_enabled = false;
    inline float skeleton_thickness = 1.0f;
    inline bool skeleton_outline = true;



    inline bool chams_enabled = false;
    inline float chams_fill[4]{ 1.0f, 0.2f, 0.6f, 0.55f };
    inline float chams_outline_col[4]{ 1.0f, 0.2f, 0.6f, 1.0f };

    inline std::vector<player_info> player_list;
    inline std::string local_player_name;
    inline bool updated = false;
}
