#include "includes.h"
#include <map>
#include <string>
#include <vector>
#include <mutex>
#include "include/xorstr/xorstr.hpp"
#include "cfx/resource.h"
#include "cfx/resource_manager.h"
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#include "TextEditor.h"

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

std::vector<std::string> m_savedScripts;

// Declare showMenu before WndProc so it can be used
static bool showMenu = false;

void InitImGui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (showMenu && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

static SHORT lastInsertState = 0;
bool init = false;
static float menuAlpha = 0.0f;

void SetupCustomImGuiStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();

    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 6);

    style.ScrollbarSize = 12;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;

    style.WindowRounding = 14.0f;
    style.FrameRounding = 10.0f;
    style.GrabRounding = 8.0f;
    style.ScrollbarRounding = 10.0f;
    style.TabRounding = 10.0f;

    ImVec4* colors = style.Colors;

    // TEXT
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.50f, 0.50f, 1.00f);

    // WINDOWS
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.05f, 0.05f, 0.95f); // Dark red
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.06f, 0.06f, 0.90f);

    // POPUPS
    colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.07f, 0.07f, 0.95f);

    // BORDERS
    colors[ImGuiCol_Border] = ImVec4(0.70f, 0.25f, 0.15f, 0.50f); // red/orange

    // FRAMES (inputs, dropdown, etc.)
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.85f, 0.35f, 0.25f, 1.00f); // bright Xmas orange/red
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.95f, 0.40f, 0.30f, 1.00f);

    // TITLE BAR
    colors[ImGuiCol_TitleBg] = ImVec4(0.18f, 0.07f, 0.07f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.90f, 0.25f, 0.20f, 1.00f); // bright red
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.05f, 0.05f, 0.80f);

    // MENU BAR
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.07f, 0.07f, 1.00f);

    // SCROLLBAR
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.07f, 0.07f, 0.70f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.90f, 0.40f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.00f, 0.50f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 0.60f, 0.40f, 1.00f);

    // CHECKMARK (green Christmas)
    colors[ImGuiCol_CheckMark] = ImVec4(0.10f, 0.80f, 0.25f, 1.00f); // pine green

    // SLIDERS
    colors[ImGuiCol_SliderGrab] = ImVec4(0.90f, 0.40f, 0.25f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.55f, 0.30f, 1.00f);

    // BUTTONS (Red → Orange → Gold)
    colors[ImGuiCol_Button] = ImVec4(0.85f, 0.20f, 0.15f, 0.90f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.30f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.45f, 0.25f, 1.00f);

    // HEADERS (tabs, trees)
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.10f, 0.10f, 0.85f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.40f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 0.50f, 0.30f, 1.00f);

    // SEPARATORS
    colors[ImGuiCol_Separator] = ImVec4(0.60f, 0.20f, 0.10f, 0.70f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(1.00f, 0.45f, 0.25f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(1.00f, 0.55f, 0.30f, 1.00f);

    // GRIPS
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.90f, 0.35f, 0.20f, 0.85f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 0.50f, 0.30f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.60f, 0.40f, 1.00f);

    // TABS
    colors[ImGuiCol_Tab] = ImVec4(0.30f, 0.08f, 0.08f, 0.90f);
    colors[ImGuiCol_TabHovered] = ImVec4(1.00f, 0.40f, 0.25f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(1.00f, 0.45f, 0.25f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.05f, 0.05f, 0.70f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.30f, 0.10f, 0.10f, 1.00f);

    // PLOTS
    colors[ImGuiCol_PlotLines] = ImVec4(0.90f, 0.50f, 0.20f, 1.00f); // gold/orange
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);

    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.40f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.55f, 0.25f, 1.00f);

    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.90f, 0.35f, 0.20f, 0.70f); // Xmas highlight
}


TextEditor luaEditor;
bool editorInitialized = false;

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!init)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
        {
            pDevice->GetImmediateContext(&pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
            InitImGui();
            SetupCustomImGuiStyle();
            init = true;
        }
        else
            return oPresent(pSwapChain, SyncInterval, Flags);
    }

    SHORT currentInsertState = GetAsyncKeyState(VK_INSERT);
    if ((currentInsertState & 0x8000) && !(lastInsertState & 0x8000))
    {
        showMenu = !showMenu;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Only draw cursor when menu is actually visible
    io.MouseDrawCursor = showMenu && (menuAlpha > 0.0f);
    lastInsertState = currentInsertState;

    if (showMenu)
    {
        menuAlpha = (menuAlpha + 0.05f > 1.0f) ? 1.0f : menuAlpha + 0.05f;
    }
    else
    {
        menuAlpha = (menuAlpha - 0.05f < 0.0f) ? 0.0f : menuAlpha - 0.05f;
    }

    if (menuAlpha > 0.0f)
    {
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);
        ImGui::Begin(xorstr_("JKLeaks FiveM  EXECUTOR  Free Version"), nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);

        if (ImGui::BeginTabBar(xorstr_("WindowTabBar")))
        {
            if (ImGui::BeginTabItem(xorstr_("Executor")))
            {
                auto resources = fx::ResourceManager::getCurrentIl()->getAllResources();
                static int selectedResourceIndex = 0;
                static std::string scriptInput = xorstr_("print('JKLeaks On Top Of The World')");

                if (!editorInitialized)
                {
                    TextEditor::Palette customPalette = TextEditor::GetDarkPalette();

                    customPalette[(int)TextEditor::PaletteIndex::Default] = 0xFFECECEC;
                    customPalette[(int)TextEditor::PaletteIndex::Background] = 0xFF1A0A0A;  // deep red
                    customPalette[(int)TextEditor::PaletteIndex::Cursor] = 0xFFFFD27F;  // gold
                    customPalette[(int)TextEditor::PaletteIndex::Selection] = 0xFF802020;  // dark cherry red

                    // Christmas Highlight Colors
                    customPalette[(int)TextEditor::PaletteIndex::Keyword] = 0xFFFF5533;  // bright red
                    customPalette[(int)TextEditor::PaletteIndex::Number] = 0xFFFFB066;  // gold/orange
                    customPalette[(int)TextEditor::PaletteIndex::String] = 0xFF55CC55;  // pine green
                    customPalette[(int)TextEditor::PaletteIndex::CharLiteral] = 0xFF66DD66;

                    customPalette[(int)TextEditor::PaletteIndex::Identifier] = 0xFFECECEC;
                    customPalette[(int)TextEditor::PaletteIndex::Comment] = 0xFFAA5030;  // warm cinnamon
                    customPalette[(int)TextEditor::PaletteIndex::MultiLineComment] = 0xFFAA5030;

                    customPalette[(int)TextEditor::PaletteIndex::Preprocessor] = 0xFFFF3300;  // bright Xmas orange/red
                    customPalette[(int)TextEditor::PaletteIndex::ErrorMarker] = 0xFFFF0000;

                    customPalette[(int)TextEditor::PaletteIndex::Breakpoint] = 0xFFAA0000;
                    customPalette[(int)TextEditor::PaletteIndex::LineNumber] = 0xFF663333;

                    customPalette[(int)TextEditor::PaletteIndex::CurrentLineFill] = 0x22AA3A3A;
                    customPalette[(int)TextEditor::PaletteIndex::CurrentLineFillInactive] = 0x11000000;

                    luaEditor.SetPalette(customPalette);

                }

                luaEditor.Render("LuaEditor", ImVec2(-1, 325));

                if (ImGui::Button(xorstr_("Execute")))
                {
                    if (!resources.empty())
                    {
                        auto r = resources[selectedResourceIndex];
                        static size_t connHandle = 0;
                        static std::map<std::string, int> loadCounts;

                        scriptInput = luaEditor.GetText();

                        connHandle = r->Runtime.Connect(
                            [r](std::vector<char>* info)
                            {
                                int& count = loadCounts[r->get_impl()->GetName()];
                                int resolved = count - 4;
                                std::string buffer = scriptInput + ";";
                                if (resolved == 0)
                                {
                                    info->insert(info->begin(), buffer.begin(), buffer.end());
                                }
                                count++;
                            }
                        );

                        r->Stop();
                        r->Start();
                        r->Runtime.Disconnect(connHandle);
                        loadCounts.clear();
                    }
                    else
                    {
                        MessageBoxA(nullptr, xorstr_("Error finding resource"), xorstr_("Error"), 0);
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Clear"))
                {
                    luaEditor.SetText("");
                }

                ImGui::SameLine();

                std::string curNameStr = "Select a resource to execute";
                if (!resources.empty())
                {
                    curNameStr = resources[selectedResourceIndex]->get_impl()->GetName();
                }

                if (ImGui::BeginCombo(xorstr_("##resourceCombo"), curNameStr.c_str()))
                {
                    for (int i = 0; i < (int)resources.size(); i++)
                    {
                        std::string nameStr = resources[i]->get_impl()->GetName();
                        bool isSelected = (selectedResourceIndex == i);
                        if (ImGui::Selectable(nameStr.c_str(), isSelected))
                        {
                            selectedResourceIndex = i;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Resources"))
            {
                auto resources = fx::ResourceManager::getCurrentIl()->getAllResources();
                int Count = (int)resources.size();
                static int CurrentId = -1;

                if (CurrentId >= Count)
                    CurrentId = -1;

                static std::vector<bool> started_flags;
                if ((int)started_flags.size() != Count)
                {
                    started_flags.assign(Count, true);
                }

                ImGui::BeginGroup();

                static ImGuiTextFilter filter;
                filter.Draw("##Filter", ImGui::GetContentRegionAvailWidth());

                if (ImGui::BeginChild("##ResourceList", ImVec2{ 0, -ImGui::GetFrameHeightWithSpacing() }, true))
                {
                    for (int i = 0; i < Count; i++)
                    {
                        auto name = resources[i]->GetName();
                        bool selected = (CurrentId == i);

                        if (filter.PassFilter(name.c_str()))
                        {
                            std::string displayName = name + (started_flags[i] ? " - Started" : " - Stopped");
                            if (ImGui::Selectable(displayName.c_str(), selected))
                            {
                                CurrentId = i;
                            }
                        }
                        else if (selected)
                        {
                            CurrentId = -1;
                        }
                    }
                    ImGui::EndChild();
                }

                if (CurrentId != -1)
                {
                    if (ImGui::Button("Start"))
                    {
                        resources[CurrentId]->get_impl()->Start();
                        started_flags[CurrentId] = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Stop"))
                    {
                        resources[CurrentId]->get_impl()->Stop();
                        started_flags[CurrentId] = false;
                    }
                }
                else
                {
                    // Use PushStyleVar to make buttons look disabled
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    ImGui::Button("Start");
                    ImGui::SameLine();
                    ImGui::Button("Stop");
                    ImGui::PopStyleVar();
                }

                ImGui::EndGroup();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    ImGui::Render();
    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI MainThread(LPVOID)
{
    if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
    {
        kiero::bind(8, (void**)&oPresent, hkPresent);
        Beep(1000, 200);
    }
    else
    {
        MessageBoxA(nullptr, xorstr_("Error inicializando hook"), xorstr_("K9"), MB_OK | MB_ICONERROR);
    }

    return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hMod);
        CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        kiero::shutdown();
        break;
    }
    return TRUE;
}