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
#include "sprites.hpp"
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
        const bool artProbe = argc > 1 && std::string(argv[1]) == "--art-probe";
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
            L"Moonfall DX12 Original Art Test | Tab Z/Nyra | E hero skill | WASD move | Space attack",
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
        moonfall::OriginalHeroSprites originalSprites;
        bool hasOriginalSprites=false;
        wchar_t modulePath[MAX_PATH]={};
        if(!GetModuleFileNameW(nullptr,modulePath,MAX_PATH))
            throw std::runtime_error("Could not locate the DX12 executable");
        std::wstring moduleDirectory(modulePath);
        const auto separator=moduleDirectory.find_last_of(L"\\/");
        if(separator==std::wstring::npos)throw std::runtime_error("Invalid DX12 executable path");
        moduleDirectory.resize(separator+1);
        const std::wstring zArt=moduleDirectory+L"hero-z.png";
        const std::wstring nyraArt=moduleDirectory+L"hero-nyra.png";
        if(GetFileAttributesW(zArt.c_str())!=INVALID_FILE_ATTRIBUTES &&
           GetFileAttributesW(nyraArt.c_str())!=INVALID_FILE_ATTRIBUTES){
            const HRESULT comResult=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
            if(FAILED(comResult))throw std::runtime_error("WIC COM initialization failed");
            Ensure(allocator->Reset(),"Reset art upload allocator");
            Ensure(commands->Reset(allocator.Get(),nullptr),"Begin hero sprite texture uploads");
            originalSprites.Initialize(device.Get(),commands.Get(),zArt.c_str(),nyraArt.c_str());
            Ensure(commands->Close(),"Complete hero sprite uploads");
            ID3D12CommandList* assetCommands[]={commands.Get()};
            queue->ExecuteCommandLists(1,assetCommands);
            Ensure(queue->Signal(fence.Get(),++fenceValue),"Signal image upload fence");
            if(fence->GetCompletedValue()<fenceValue){
                Ensure(fence->SetEventOnCompletion(fenceValue,fenceEvent),"Wait for hero uploads");
                WaitForSingleObject(fenceEvent,INFINITE);
            }
            hasOriginalSprites=true;
        }else{
            std::fprintf(stderr,"Original Z/Nyra image copies are missing; using fallback marker.\n");
        }
        if(artProbe){
            if(!hasOriginalSprites)throw std::runtime_error("Native art probe requires Z and Nyra PNG copies");
            std::puts("PASS: Z and Nyra original image copies decoded and uploaded to DX12 GPU resources");
            return 0;
        }
        moonfall::Arena arena;
        using GameClock = std::chrono::steady_clock;
        auto previousFrame = GameClock::now();
        bool wasRestartDown = false, wasSwitchDown=false, wasSpecialDown=false, wasWeaponDown=false;
        const float gridTint[4] = {0.11f, 0.10f, 0.21f, 1.f};
        const float boundaryTint[4] = {0.29f, 0.19f, 0.44f, 1.f};
        const float playerTint[4] = {0.35f, 0.88f, 0.98f, 1.f};
        const float enemyTint[4] = {0.91f, 0.24f, 0.38f, 1.f};
        const float enemyEliteTint[4] = {0.96f, 0.51f, 0.25f, 1.f};
        const float runnerTint[4] = {0.19f, 0.75f, 0.85f, 1.f};
        const float bruteTint[4] = {0.84f, 0.39f, 0.17f, 1.f};
        const float hydraTint[4] = {0.52f, 0.22f, 0.79f, 1.f};
        const float hydraEnrageTint[4] = {0.93f, 0.17f, 0.69f, 1.f};
        const float telegraphTint[4] = {0.91f, 0.13f, 0.33f, 1.f};
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
            const bool switchDown=held(VK_TAB);
            input.switchHero=switchDown&&!wasSwitchDown;
            wasSwitchDown=switchDown;
            const bool specialDown=held('E');
            input.special=specialDown&&!wasSpecialDown;
            wasSpecialDown=specialDown;
            const bool weaponDown=held('Q');
            input.switchWeapon=weaponDown&&!wasWeaponDown;
            wasWeaponDown=weaponDown;
            arena.update(dt,input);
            if ((frameNumber % 15) == 0) {
                std::wstring title=L"Moonfall DX12 ART TEST | "+std::wstring(arena.hero==moonfall::Hero::Z?L"Z":L"Nyra")+
                    L" | HP "+std::to_wstring(static_cast<int>(arena.health))+
                    L" | Defeated "+std::to_wstring(arena.kills)+L"/36 | Wave "+std::to_wstring(arena.wave);
                title+=L" | "+std::wstring(arena.weapon==moonfall::Weapon::Bolt?L"Bolt":L"Nova");
                if(const auto* hydra=arena.hydra()){
                    title+=L" | HYDRA "+std::to_wstring(static_cast<int>(std::max(0.f,hydra->hp)))+
                           L"/32"+(arena.hydraEnraged()?std::wstring(L" ENRAGED"):std::wstring());
                }
                if (arena.phase == moonfall::Phase::Won) title+=L" | HYDRA DEFEATED! Press R to restart";
                else if (arena.phase == moonfall::Phase::Lost) title+=L" | FALLEN! Press R to restart";
                else title+=L" | Q: weapon | Tab: hero | E: skill | WASD, Space, Shift, R";
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
            // DX12 scene: primitive enemies and bullets, with actual original hero art if present.
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
            // Enemies and weapons are simplified abstract DX12 prototypes.
            // The Hydra's large warning box indicates an upcoming area attack.
            const auto* hydra=arena.hydra();
            if(hydra&&arena.hydraTell>0){
                const float r=100.f;
                rectangle(hydra->x-r,hydra->y-r,r*2,3,telegraphTint);
                rectangle(hydra->x-r,hydra->y+r-3,r*2,3,telegraphTint);
                rectangle(hydra->x-r,hydra->y-r,3,r*2,telegraphTint);
                rectangle(hydra->x+r-3,hydra->y-r,3,r*2,telegraphTint);
            }
            for(const auto& enemy:arena.enemies)if(enemy.active){
                const float size=enemy.role==moonfall::EnemyRole::Hydra?76.f:
                                 enemy.role==moonfall::EnemyRole::Brute?36.f:
                                 enemy.role==moonfall::EnemyRole::Runner?18.f:24.f;
                const float* roleColor=enemy.role==moonfall::EnemyRole::Runner?runnerTint:
                                       enemy.role==moonfall::EnemyRole::Brute?bruteTint:
                                       enemy.role==moonfall::EnemyRole::Hydra?
                                         (arena.hydraEnraged()?hydraEnrageTint:hydraTint):enemyTint;
                rectangle(enemy.x-size*.5f,enemy.y-size*.5f,size,size,
                          enemy.flash>0?hurtTint:roleColor);
                const float healthRatio=std::max(0.f,std::min(1.f,
                                           enemy.hp/std::max(1.f,enemy.maxHp)));
                rectangle(enemy.x-size*.5f,enemy.y-size*.5f-8,size,4,boundaryTint);
                rectangle(enemy.x-size*.5f,enemy.y-size*.5f-8,size*healthRatio,4,hpTint);
            }
            for(const auto& shot:arena.shots)if(shot.active)
                rectangle(shot.x-4.f,shot.y-4.f,8,8,projectileTint);
            // Fixed-capacity short-lived impact effects (no art regeneration).
            for(const auto& fx:arena.impacts)if(fx.active){
                const float r=fx.radius;
                const float thickness=std::max(2.f,std::min(5.f,r*.07f));
                rectangle(fx.x-r,fx.y-r,r*2,thickness,projectileTint);
                rectangle(fx.x-r,fx.y+r-thickness,r*2,thickness,projectileTint);
                rectangle(fx.x-r,fx.y-r,thickness,r*2,projectileTint);
                rectangle(fx.x+r-thickness,fx.y-r,thickness,r*2,projectileTint);
            }
            if(arena.novaPulse>0){
                const float r=116.f;
                rectangle(arena.x-r,arena.y-r,r*2,4,enemyEliteTint);
                rectangle(arena.x-r,arena.y+r-4,r*2,4,enemyEliteTint);
                rectangle(arena.x-r,arena.y-r,4,r*2,enemyEliteTint);
                rectangle(arena.x+r-4,arena.y-r,4,r*2,enemyEliteTint);
            }
            if(arena.hero==moonfall::Hero::Nyra && arena.nyraShield>0){
                rectangle(arena.x-58.f,arena.y-58.f,116.f,116.f,boundaryTint);
                rectangle(arena.x-54.f,arena.y-54.f,108.f,108.f,clearColor);
            }
            if(arena.hero==moonfall::Hero::Z && arena.dragonPower>0){
                rectangle(arena.x-53.f,arena.y-53.f,106.f,106.f,enemyEliteTint);
            }
            if(hasOriginalSprites){
                originalSprites.Draw(commands.Get(),arena.hero==moonfall::Hero::Nyra?1:0,
                                     arena.x,arena.y,rtv);
            }else{
                rectangle(arena.x-13.f,arena.y-13.f,26,26,
                          arena.invulnerable>0 && frameNumber%4<2?hurtTint:playerTint);
                rectangle(arena.x-4.f,arena.y-4.f,8,8,clearColor);
            }
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
