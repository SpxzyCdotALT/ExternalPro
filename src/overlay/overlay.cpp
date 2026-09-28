#include "overlay.hpp"
#include "string"
#include "../instance/instance.hpp"
#include "../features/esp/esp.hpp"
#include "../features/aimbot/aimbot.hpp"
#include "../features/triggerbot/triggerbot.hpp"
#include "../ui/menu.hpp"

bool overlay_manager::create_d3d_device(HWND window_handle)
{
    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.BufferDesc.Width = 0;
    swap_chain_desc.BufferDesc.Height = 0;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.OutputWindow = window_handle;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.Windowed = TRUE;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const UINT creation_flags = 0;
    D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL selected_feature_level;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creation_flags,
        feature_levels,
        _countof(feature_levels),
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &swap_chain,
        &d3d_device,
        &selected_feature_level,
        &d3d_context);

    if (hr == DXGI_ERROR_UNSUPPORTED)
    {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            creation_flags,
            feature_levels,
            _countof(feature_levels),
            D3D11_SDK_VERSION,
            &swap_chain_desc,
            &swap_chain,
            &d3d_device,
            &selected_feature_level,
            &d3d_context);
    }

    if (hr != S_OK)
        return false;

    create_render_target_view();
    return true;
}

void overlay_manager::cleanup_d3d_device()
{
    cleanup_render_target_view();

    if (swap_chain)
    {
        swap_chain->Release();
        swap_chain = nullptr;
    }
    if (d3d_context)
    {
        d3d_context->Release();
        d3d_context = nullptr;
    }
    if (d3d_device)
    {
        d3d_device->Release();
        d3d_device = nullptr;
    }
}

void overlay_manager::create_render_target_view()
{
    ID3D11Texture2D* back_buffer = nullptr;
    if (swap_chain && swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)) == S_OK)
    {
        d3d_device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
        back_buffer->Release();
    }
}

void overlay_manager::cleanup_render_target_view()
{
    if (render_target_view)
    {
        render_target_view->Release();
        render_target_view = nullptr;
    }
}

LRESULT CALLBACK overlay_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (overlay_instance->d3d_device && wparam != SIZE_MINIMIZED)
        {
            overlay_instance->cleanup_render_target_view();
            overlay_instance->swap_chain->ResizeBuffers(2, LOWORD(lparam), HIWORD(lparam), DXGI_FORMAT_UNKNOWN, 0);
            overlay_instance->create_render_target_view();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wparam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

bool overlay_manager::initialize_overlay()
{
    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = overlay_wndproc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(CreateSolidBrush(RGB(0, 0, 0)));
    wc.lpszClassName = "skibidi ohio";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    RegisterClassExA(&wc);

    HWND overlay_window = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        wc.lpszClassName,
        "rizzler",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr);

    SetLayeredWindowAttributes(overlay_window, 0, 255, LWA_ALPHA);

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(overlay_window, &margins);

    if (!create_d3d_device(overlay_window))
    {
        cleanup_d3d_device();
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return false;
    }

    ShowWindow(overlay_window, SW_SHOW);
    UpdateWindow(overlay_window);

    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    menu::style();

    ImGui_ImplWin32_Init(overlay_window);
    ImGui_ImplDX11_Init(d3d_device, d3d_context);

    ImVec4 clear_color = ImVec4(0, 0, 0, 0);

    bool done = false;
    bool draw_overlay = true;

    while (!done)
    {
        MSG msg{};
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }

        if (done)
            break;

        HWND target = FindWindowA(nullptr, "Roblox");
        HWND foreground = GetForegroundWindow();

        if (target != foreground && overlay_window != foreground)
        {
            MoveWindow(overlay_window, 0, 0, 0, 0, true);
        }
        else
        {
            RECT target_rect{};
            GetWindowRect(target, &target_rect);

            int width = target_rect.right - target_rect.left;
            int height = target_rect.bottom - target_rect.top;

            bool fullscreen = false;
            MONITORINFO monitor_info{ sizeof(MONITORINFO) };
            if (GetMonitorInfoA(MonitorFromWindow(target, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
            {
                RECT window_rect{};
                if (GetWindowRect(target, &window_rect))
                {
                    fullscreen = window_rect.left == monitor_info.rcMonitor.left &&
                        window_rect.right == monitor_info.rcMonitor.right &&
                        window_rect.top == monitor_info.rcMonitor.top &&
                        window_rect.bottom == monitor_info.rcMonitor.bottom;
                }
            }

            if (fullscreen)
            {
                width += 16;
                height -= 24;
            }
            else
            {
                height -= 63;
                target_rect.left += 8;
                target_rect.top += 31;
            }

            MoveWindow(overlay_window, target_rect.left, target_rect.top, width, height, TRUE);
        }

        if (!menu::listening() && (GetAsyncKeyState(globals::menu_key) & 1))
            draw_overlay = !draw_overlay;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (draw_overlay)
            menu::render(&draw_overlay);

        if (draw_overlay)
            SetWindowLongA(overlay_window, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
        else
            SetWindowLongA(overlay_window, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW);

        ImGui::EndFrame();
        ImGui::Render();

        float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        d3d_context->OMSetRenderTargets(1, &render_target_view, nullptr);
        d3d_context->ClearRenderTargetView(render_target_view, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        swap_chain->Present(1, 0);

        static DWORD last_time = timeGetTime();
        DWORD current_time = timeGetTime();
        float delta = (current_time - last_time) / 1000.0f;
        const float target_frame_time = 1.0f / 1000.0f;

        if (delta < target_frame_time)
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((target_frame_time - delta) * 1000)));

        last_time = current_time;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    cleanup_d3d_device();
    DestroyWindow(overlay_window);
    UnregisterClassA(wc.lpszClassName, wc.hInstance);

    return true;
}