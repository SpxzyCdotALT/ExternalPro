#pragma once
#include <Windows.h>

namespace offsets
{

	const uintptr_t fake_datamodel = 0x8ee1728;
	const uintptr_t real_datamodel = 0x1f8;


	const uintptr_t primitive = 0x178;

	const uintptr_t position = 0xd4;
	const uintptr_t rotation = 0xb0;
	const uintptr_t size = 0x1bc;
	const uintptr_t velocity = 0xe0;

	const uintptr_t name = 0x8;
	const uintptr_t namecontainer = 0x70;
	const uintptr_t parent = 0x68;
	const uintptr_t children = 0x78;
	const uintptr_t children_end = 0x8;
	const uintptr_t class_descriptor = 0x18;
	const uintptr_t local_player = 0x120;
	const uintptr_t model_instance = 0x288;
	const uintptr_t display_name = 0xa8;


	const uintptr_t visualengine_pointer = 0x851bf08;
	const uintptr_t viewmatrix = 0x1b0;
	const uintptr_t visual_dimensions = 0xb10;


	const uintptr_t camera_subject = 0xb8;
	const uintptr_t current_camera = 0x4a8;
	const uintptr_t fov = 0x130;
	const uintptr_t camera_position = 0xec;
	const uintptr_t camera_rotation = 0xc8;
	const uintptr_t viewport = 0x2bc;


	const uintptr_t walk_speed = 0x1c0;
	const uintptr_t walk_speed_check = 0x39c;
	const uintptr_t jump_power = 0x194;
	const uintptr_t sitting = 0x0;
	const uintptr_t health = 0x180;
	const uintptr_t max_health = 0x198;
}
