#pragma once
#include <Windows.h>
#include <iostream>
#include <thread>
#include <dwmapi.h>
#include <d3d11.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#pragma comment(lib, "d3d11.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

struct screen_info
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool operator==(const screen_info& other) const
    {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }

    bool operator!=(const screen_info& other) const
    {
        return !(*this == other);
    }
};

class overlay_manager
{
public:
    ID3D11Device* d3d_device = nullptr;
    ID3D11DeviceContext* d3d_context = nullptr;
    IDXGISwapChain* swap_chain = nullptr;
    ID3D11RenderTargetView* render_target_view = nullptr;

    bool create_d3d_device(HWND window_handle);
    void cleanup_d3d_device();

    void create_render_target_view();
    void cleanup_render_target_view();

    bool initialize_overlay();
};

inline auto overlay_instance = std::make_unique<overlay_manager>();