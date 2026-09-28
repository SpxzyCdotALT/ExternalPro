#include "player.hpp"

#include <chrono>
#include <cstring>
#include <thread>
#include <vector>
#include <iostream>

#include "../../instance/datamodel.hpp"
#include "../../globals.hpp"
#include "../../memory/memory.hpp"
#include "../../offsets.hpp"

namespace features::player
{
    namespace
    {
        struct toggle_state
        {
            bool applied = false;
            float original = 0.0f;
        };




        void apply_float_toggle(bool enabled, toggle_state& state, uintptr_t primary, float value, uintptr_t secondary = 0)
        {
            if (!primary)
            {
                state.applied = false;
                return;
            }

            if (enabled)
            {
                if (!state.applied)
                {
                    state.original = memory::read<float>(primary);
                    state.applied = true;
                }

                memory::write<float>(primary, value);
                if (secondary)
                    memory::write<float>(secondary, value);
            }
            else if (state.applied)
            {
                memory::write<float>(primary, state.original);
                if (secondary)
                    memory::write<float>(secondary, state.original);
                state.applied = false;
            }
        }

        inline toggle_state walk_speed_state{};
        inline toggle_state jump_power_state{};
        inline toggle_state fov_state{};


        float read_obfuscated_float(uintptr_t address)
        {
            if (!address)
                return 0.0f;

            const auto ptr = memory::read<uintptr_t>(address);
            if (!ptr)
                return 0.0f;

            const uint64_t xored = static_cast<uint64_t>(ptr) ^ memory::read<uint64_t>(ptr);

            float value = 0.0f;
            std::memcpy(&value, &xored, sizeof(value));
            return value;
        }

        void update_player_list()
        {
            globals::player_list.clear();

            if (!datamodel::players.address)
                return;

            globals::local_player_name = datamodel::local_player().name();



            for (const instance& player : datamodel::players.children())
            {
                if (!player.address)
                    continue;

                const auto character_addr = memory::read<uintptr_t>(player.address + offsets::model_instance);
                if (!character_addr)
                    continue;
                const instance character(character_addr);




                instance hrp(0), humanoid(0);
                Vector3 joints[16]{};
                Vector3 joint_tops[16]{};
                Vector3 joint_bottoms[16]{};
                Vector3 joint_sizes[16]{};
                Vector3 joint_axes[16][3]{};
                uint16_t joint_mask = 0;

                auto set_joint = [&](PlayerJoint slot, const instance& part) {
                    const Vector3 center = part.position();
                    const Vector3 part_size = part.size();
                    const Vector3 up = part.up();
                    float hy = part_size.y * 0.5f;
                    if (!(hy > 0.0f))
                        hy = 0.5f;
                    const Vector3 offset = up * hy;
                    joints[slot] = center;
                    joint_tops[slot] = center + offset;
                    joint_bottoms[slot] = center - offset;
                    joint_sizes[slot] = part_size;
                    float rot[9]{};
                    if (part.rotation(rot))
                    {

                        joint_axes[slot][0] = Vector3(rot[0], rot[3], rot[6]);
                        joint_axes[slot][1] = Vector3(rot[1], rot[4], rot[7]);
                        joint_axes[slot][2] = Vector3(rot[2], rot[5], rot[8]);
                    }
                    else
                    {
                        joint_axes[slot][0] = Vector3(1.0f, 0.0f, 0.0f);
                        joint_axes[slot][1] = Vector3(0.0f, 1.0f, 0.0f);
                        joint_axes[slot][2] = Vector3(0.0f, 0.0f, 1.0f);
                    }
                    joint_mask |= static_cast<uint16_t>(1u << slot);
                };

                for (const instance& part : character.children())
                {
                    const std::string part_name = part.name();
                    if (part_name.empty())
                        continue;

                    if (part_name == "HumanoidRootPart") { hrp = part; set_joint(J_ROOT, part); }
                    else if (part_name == "Humanoid") humanoid = part;
                    else if (part_name == "Head") set_joint(J_HEAD, part);
                    else if (part_name == "Torso" || part_name == "UpperTorso") set_joint(J_TORSO, part);
                    else if (part_name == "LowerTorso") set_joint(J_TORSO_LOWER, part);
                    else if (part_name == "LeftUpperArm") set_joint(J_UPPER_ARM_L, part);
                    else if (part_name == "LeftLowerArm") set_joint(J_LOWER_ARM_L, part);
                    else if (part_name == "LeftHand" || part_name == "Left Arm") set_joint(J_HAND_L, part);
                    else if (part_name == "RightUpperArm") set_joint(J_UPPER_ARM_R, part);
                    else if (part_name == "RightLowerArm") set_joint(J_LOWER_ARM_R, part);
                    else if (part_name == "RightHand" || part_name == "Right Arm") set_joint(J_HAND_R, part);
                    else if (part_name == "LeftUpperLeg") set_joint(J_UPPER_LEG_L, part);
                    else if (part_name == "LeftLowerLeg") set_joint(J_LOWER_LEG_L, part);
                    else if (part_name == "LeftFoot" || part_name == "Left Leg") set_joint(J_FOOT_L, part);
                    else if (part_name == "RightUpperLeg") set_joint(J_UPPER_LEG_R, part);
                    else if (part_name == "RightLowerLeg") set_joint(J_LOWER_LEG_R, part);
                    else if (part_name == "RightFoot" || part_name == "Right Leg") set_joint(J_FOOT_R, part);
                }

                if (!hrp.address || !humanoid.address)
                    continue;

                const std::string player_name = player.name();
                const std::string player_display = player.display_name();

                std::string display_name;
                if (!player_display.empty() && player_display != player_name)
                    display_name = player_display;

                player_info info;
                info.name = character.name();
                info.display_name = display_name;
                info.position = hrp.position();
                info.velocity = hrp.velocity();
                info.joint_mask = joint_mask;
                std::memcpy(info.joints, joints, sizeof(joints));
                std::memcpy(info.joint_tops, joint_tops, sizeof(joint_tops));
                std::memcpy(info.joint_bottoms, joint_bottoms, sizeof(joint_bottoms));
                std::memcpy(info.joint_sizes, joint_sizes, sizeof(joint_sizes));
                std::memcpy(info.joint_axes, joint_axes, sizeof(joint_axes));
                info.health = read_obfuscated_float(humanoid.address + offsets::health);
                info.max_health = read_obfuscated_float(humanoid.address + offsets::max_health);
                if (info.max_health <= 0.0f)
                    info.max_health = 100.0f;

                globals::player_list.push_back(info);
            }
        }

        void update_local_attributes()
        {
            const instance humanoid = datamodel::local_humanoid();

            apply_float_toggle(globals::walk_speed_enabled, walk_speed_state,
                humanoid.address ? humanoid.address + offsets::walk_speed : 0,
                globals::walk_speed,
                humanoid.address ? humanoid.address + offsets::walk_speed_check : 0);

            apply_float_toggle(globals::jump_power_enabled, jump_power_state,
                humanoid.address ? humanoid.address + offsets::jump_power : 0,
                globals::jump_power);

            apply_float_toggle(globals::fov_enabled, fov_state,
                datamodel::camera.address ? datamodel::camera.address + offsets::fov : 0,
                globals::fov);

            if (offsets::sitting != 0 && humanoid.address)
                memory::write<int>(humanoid.address + offsets::sitting, globals::sitting ? 0 : 257);



            if (humanoid.address)
            {
                globals::debug_walk_speed = memory::read<float>(humanoid.address + offsets::walk_speed);
                globals::debug_jump_power = memory::read<float>(humanoid.address + offsets::jump_power);
            }
            else
            {
                globals::debug_walk_speed = 0.0f;
                globals::debug_jump_power = 0.0f;
            }
        }
    }

    void initialize()
    {
        for (;;)
        {
            datamodel::ensure();

            update_player_list();
            update_local_attributes();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

