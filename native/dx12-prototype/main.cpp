// Standalone, experimental Direct3D 12 proof of concept for Moonfall.
// This does not replace the Electron game, integrate game art, or enable DLSS.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>
#include <algorithm>
#include <chrono>
#include "arena.hpp"
#include <cmath>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>

using Microsoft::WRL::ComPtr;
static constexpr UINT kFrames = 2;

static void Ensure(HRESULT result, const char* label) {
    if (FAILED(result)) {
        char message[256];
        std::snprintf(message, sizeof(message), "%s failed: HRESULT 0x%08lX", label,
                      static_cast<unsigned long>(result));
        throw std::runtime_error(message);
    }
}
static LRESULT CALLBACK WindowProc(HWND window, UINT event, WPARAM w, LPARAM l) {
    if (event == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(window, event, w, l);
}
static ComPtr<ID3D12Device> MakeDevice(IDXGIFactory4* factory, bool& software) {
    ComPtr<ID3D12Device> device;
    for (UINT index = 0;; ++index) {
        ComPtr<IDXGIAdapter1> adapter;
        if (factory->EnumAdapters1(index, &adapter) == DXGI_ERROR_NOT_FOUND) break;
        DXGI_ADAPTER_DESC1 details = {};
        Ensure(adapter->GetDesc1(&details), "Get adapter details");
        // Microsoft Basic Render Driver (vendor 0x1414) is not evidence of a discrete GPU.
        if ((details.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) || details.VendorId == 0x1414u) continue;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                         IID_PPV_ARGS(&device)))) {
            software = false;
            std::wprintf(L"Direct3D 12 adapter: %ls\n", details.Description);
            return device;
        }
    }
    ComPtr<IDXGIAdapter> warp;
    Ensure(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)), "Find WARP adapter");
    Ensure(D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_11_0,
                              IID_PPV_ARGS(&device)), "Create WARP D3D12 device");
    software = true;
    std::puts("Direct3D 12 adapter: WARP software fallback (NOT a gaming benchmark).");
    return device;
}
int main(int argc, char** argv) {
    try {
        const bool probe = argc > 1 && std::string(argv[1]) == "--probe";
        ComPtr<IDXGIFactory4> factory;
        Ensure(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "Create DXGI factory");
        bool software = false;
        ComPtr<ID3D12Device> device = MakeDevice(factory.Get(), software);
        if (probe) {
            std::printf("Direct3D 12 device initialized. Renderer: %s\n",
                        software ? "WARP" : "hardware");
            return 0;
        }

        HINSTANCE instance = GetModuleHandleW(nullptr);
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = instance;
        wc.lpszClassName = L"MoonfallDX12PrototypeWindow";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        if (!RegisterClassW(&wc)) throw std::runtime_error("RegisterClassW failed");
        HWND window = CreateWindowExW(0, wc.lpszClassName,
            L"Moonfall DX12 Combat Test | WASD move | SPACE shoot | SHIFT dash | R restart",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, 1040, 616,
            nullptr, nullptr, instance, nullptr);
        if (!window) throw std::runtime_error("CreateWindowExW failed");

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ComPtr<ID3D12CommandQueue> queue;
        Ensure(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)),
               "Create direct queue");

        DXGI_SWAP_CHAIN_DESC1 swapDesc = {};
        swapDesc.Width = 1024;
        swapDesc.Height = 576;
        swapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapDesc.BufferCount = kFrames;
        swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapDesc.SampleDesc.Count = 1;
        ComPtr<IDXGISwapChain1> swap1;
        Ensure(factory->CreateSwapChainForHwnd(queue.Get(), window, &swapDesc,
                                                nullptr, nullptr, &swap1),
               "Create DX12 swap chain");
        ComPtr<IDXGISwapChain3> swap;
        Ensure(swap1.As(&swap), "Get swap chain 3");
        factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.NumDescriptors = kFrames;
        ComPtr<ID3D12DescriptorHeap> heap;
        Ensure(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap)),
               "Create render target heap");
        const UINT stride = device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        std::array<ComPtr<ID3D12Resource>, kFrames> targets;
        auto handle = heap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < kFrames; ++i) {
            Ensure(swap->GetBuffer(i, IID_PPV_ARGS(&targets[i])), "Get back buffer");
            device->CreateRenderTargetView(targets[i].Get(), nullptr, handle);
            handle.ptr += stride;
        }

        ComPtr<ID3D12CommandAllocator> allocator;
        Ensure(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                 IID_PPV_ARGS(&allocator)), "Create allocator");
        ComPtr<ID3D12GraphicsCommandList> commands;
        Ensure(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                 allocator.Get(), nullptr, IID_PPV_ARGS(&commands)), "Create list");
        Ensure(commands->Close(), "Close initial list");
        ComPtr<ID3D12Fence> fence;
        Ensure(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
               "Create fence");
        HANDLE fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent) throw std::runtime_error("CreateEventW failed");
        UINT64 fenceValue = 0, frameNumber = 0;
        moonfall::Arena arena;
        using GameClock = std::chrono::steady_clock;
        auto previousFrame = GameClock::now();
        bool wasRestartDown = false;
        const float gridTint[4] = {0.11f, 0.10f, 0.21f, 1.f};
        const float boundaryTint[4] = {0.29f, 0.19f, 0.44f, 1.f};
        const float playerTint[4] = {0.35f, 0.88f, 0.98f, 1.f};
        const float enemyTint[4] = {0.91f, 0.24f, 0.38f, 1.f};
        const float enemyEliteTint[4] = {0.96f, 0.51f, 0.25f, 1.f};
        const float projectileTint[4] = {0.99f, 0.87f, 0.39f, 1.f};
        const float hpTint[4] = {0.31f, 0.94f, 0.62f, 1.f};
        const float hurtTint[4] = {0.96f, 0.96f, 0.99f, 1.f};
        const float successTint[4] = {0.16f, 0.66f, 0.43f, 1.f};
        const float defeatTint[4] = {0.48f, 0.11f, 0.23f, 1.f};


        ShowWindow(window, SW_SHOW);
        bool running = true;
        while (running) {
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) { running = false; break; }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            if (!running) break;
            const auto now = GameClock::now();
            const float dt = std::chrono::duration<float>(now-previousFrame).count();
            previousFrame = now;
            const bool focused = GetForegroundWindow() == window;
            const auto held = [focused](int key) {
                return focused && (GetAsyncKeyState(key) & 0x8000) != 0;
            };
            moonfall::Input input{};
            input.up=held('W') || held(VK_UP);
            input.down=held('S') || held(VK_DOWN);
            input.left=held('A') || held(VK_LEFT);
            input.right=held('D') || held(VK_RIGHT);
            input.attack=held(VK_SPACE) || held(VK_LBUTTON);
            input.dash=held(VK_SHIFT);
            const bool restartDown=held('R');
            input.restart=restartDown&&!wasRestartDown;
            wasRestartDown=restartDown;
            arena.update(dt,input);
            if ((frameNumber % 15) == 0) {
                std::wstring title=L"Moonfall DX12 ARENA TEST | HP "+std::to_wstring(static_cast<int>(arena.health))+
                    L" | Defeated "+std::to_wstring(arena.kills)+L"/36 | Wave "+std::to_wstring(arena.wave);
                if (arena.phase == moonfall::Phase::Won) title+=L" | VICTORY! Press R to restart";
                else if (arena.phase == moonfall::Phase::Lost) title+=L" | FALLEN! Press R to restart";
                else title+=L" | WASD/Arrows, Space/Click shoot, Shift dash, R restart";
                SetWindowTextW(window,title.c_str());
            }
            const UINT current = swap->GetCurrentBackBufferIndex();
            Ensure(allocator->Reset(), "Reset allocator");
            Ensure(commands->Reset(allocator.Get(), nullptr), "Reset commands");

            D3D12_RESOURCE_BARRIER transition = {};
            transition.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            transition.Transition.pResource = targets[current].Get();
            transition.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            transition.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            transition.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            commands->ResourceBarrier(1, &transition);
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = heap->GetCPUDescriptorHandleForHeapStart();
            rtv.ptr += current * stride;
            const float phase = 0.5f + 0.5f * std::sin(static_cast<float>(frameNumber) * 0.015f);
            const float clearColor[4] = {0.04f + 0.12f * phase, 0.02f, 0.14f + 0.22f * phase, 1.0f};
            commands->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
            // Renderer test uses DX12 RTV rectangle clears, no new shaders or art.
            // Logic coordinates stay independent of future sprite/asset migration.
            const auto rectangle = [&](float x, float y, float w, float h, const float* color){
                D3D12_RECT rect{};
                rect.left=std::max<LONG>(0,static_cast<LONG>(std::floor(x)));
                rect.top=std::max<LONG>(0,static_cast<LONG>(std::floor(y)));
                rect.right=std::min<LONG>(1024,static_cast<LONG>(std::ceil(x+w)));
                rect.bottom=std::min<LONG>(576,static_cast<LONG>(std::ceil(y+h)));
                if(rect.right>rect.left&&rect.bottom>rect.top)
                    commands->ClearRenderTargetView(rtv,color,1,&rect);
            };
            for(int xx=64;xx<1024;xx+=64)rectangle(float(xx),0,1,576,gridTint);
            for(int yy=64;yy<576;yy+=64)rectangle(0,float(yy),1024,1,gridTint);
            rectangle(12,12,1000,2,boundaryTint);
            rectangle(12,563,1000,2,boundaryTint);
            rectangle(12,12,2,553,boundaryTint);
            rectangle(1010,12,2,553,boundaryTint);
            rectangle(18,18,220,16,boundaryTint);
            rectangle(20,20,216.f*arena.health/100.f,12,hpTint);
            // Abstract geometric markers deliberately do not replace Z/Nyra art.
            for(const auto& enemy:arena.enemies)if(enemy.active) {
                rectangle(enemy.x-12.f,enemy.y-12.f,24,24,
                          arena.wave==3?enemyEliteTint:enemyTint);
                rectangle(enemy.x-7.f,enemy.y-17.f,14.f*enemy.hp/4.f,3,hpTint);
            }
            for(const auto& shot:arena.shots)if(shot.active)
                rectangle(shot.x-4.f,shot.y-4.f,8,8,projectileTint);
            rectangle(arena.x-13.f,arena.y-13.f,26,26,
                      arena.invulnerable>0 && frameNumber%4<2?hurtTint:playerTint);
            rectangle(arena.x-4.f,arena.y-4.f,8,8,clearColor);
            if(arena.phase==moonfall::Phase::Won){
                rectangle(190,250,644,74,successTint);
                rectangle(208,266,608,42,gridTint);
            }else if(arena.phase==moonfall::Phase::Lost){
                rectangle(190,250,644,74,defeatTint);
                rectangle(208,266,608,42,gridTint);
            }
            transition.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            transition.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            commands->ResourceBarrier(1, &transition);
            Ensure(commands->Close(), "Close command list");
            ID3D12CommandList* lists[] = {commands.Get()};
            queue->ExecuteCommandLists(1, lists);
            Ensure(swap->Present(1, 0), "Present");
            Ensure(queue->Signal(fence.Get(), ++fenceValue), "Signal fence");
            if (fence->GetCompletedValue() < fenceValue) {
                Ensure(fence->SetEventOnCompletion(fenceValue, fenceEvent),
                       "Wait for GPU");
                WaitForSingleObject(fenceEvent, INFINITE);
            }
            ++frameNumber;
        }
        CloseHandle(fenceEvent);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "DX12 prototype failed: %s\n", error.what());
        return 1;
    }
}
