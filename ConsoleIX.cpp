#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <stdio.h>
#include <Windows.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <map>
#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include <wininet.h>
#include <shellapi.h>
#include "proc.h"
#include "mem.h"
#include "Helpers.h"
#include "Signature.h"

#pragma comment(lib, "wininet.lib")


DWORD procId = 0;
HANDLE hProcess = nullptr;
uintptr_t moduleBase = 0;


struct Patch
{
    uintptr_t address = 0;
    std::vector<BYTE> onBytes;
    std::vector<BYTE> offBytes;
};

std::map<std::string, uintptr_t> BaseMap;
std::map<std::string, uintptr_t> AddressMap;
std::map<std::string, Patch> PatchMap;


bool bHealth = false;
bool bEnergy = false;
bool bInstant = false;
bool bJump = false;
bool bRainbow = false;
bool bTest2 = false;
bool bTest3 = false;
bool bScrap = false;
bool ItsElectric = false;


std::thread rainbowThread;
std::atomic<bool> stopThread(false);


int MWeaponId = 0;
int QWeaponId = 0;
int WWeaponId = 0;
int EWeaponId = 0;
int PhysicalDamageId = 0;
int AbilityDamageId = 0;
int SpeedId = 0;
int JumpHeightId = 0;
int SoulChipsId = 0;
int BoltsId = 0;
int MemoriaId = 0;
int IonCubeId = 0;
int BlueCubeId = 0;
int OMWeaponId = 0;

uintptr_t LocalPlayerOffset = 0;
uintptr_t IonCubeAddr = 0;
uintptr_t BlueCubeAddr = 0;
uintptr_t SalvageAddr = 0;
uintptr_t SalvageBaseAddr = 0;
uintptr_t CatAddr = 0;
uintptr_t healthAddr = 0;
uintptr_t BypassColorAddr = 0;
uintptr_t EnergyAddr = 0;
uintptr_t AugBaseAddr = 0;
uintptr_t Aug1Addr = 0;
uintptr_t JumpAddr = 0;
uintptr_t MemoriaAddr = 0;
uintptr_t TitanShards = 0;
uintptr_t SoulChipsAddr = 0;
uintptr_t ColorAddr = 0;
uintptr_t BoltsAddr = 0;
uintptr_t MWeaponAddr = 0;
uintptr_t QWeaponAddr = 0;
uintptr_t WWeaponAddr = 0;
uintptr_t EWeaponAddr = 0;
uintptr_t PhysicalDamagePtr = 0;
uintptr_t AbilityDamagePtr = 0;
uintptr_t SpeedPtr = 0;
uintptr_t JumpHeightPtr = 0;
uintptr_t xCoord = 0;
uintptr_t zCoord = 0;
uintptr_t WingsAddr = 0;
uintptr_t EntityListAddr = 0;
uintptr_t ScrapCmpAddr = 0;
uintptr_t ScrapAddr = 0;



std::string DownloadOffsets(const char* url)
{
    HINTERNET hInternet = InternetOpenA("IX-Offsets", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet)
        return "";

    HINTERNET hFile = InternetOpenUrlA(
        hInternet,
        url,
        NULL,
        0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
        0
    );

    if (!hFile)
    {
        InternetCloseHandle(hInternet);
        return "";
    }

    std::string result;
    char buffer[1024];
    DWORD bytesRead = 0;

    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead)
        result.append(buffer, bytesRead);

    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);
    return result;
}

std::vector<BYTE> HexToBytes(const std::string& hex)
{
    std::vector<BYTE> bytes;
    std::stringstream ss(hex);
    std::string byteStr;

    while (ss >> byteStr)
        bytes.push_back((BYTE)strtoul(byteStr.c_str(), nullptr, 16));

    return bytes;
}

void ParseOffsets(const std::string& data)
{
    std::istringstream stream(data);
    std::string line;
    std::string section;

    while (std::getline(stream, line))
    {
        if (line.empty())
            continue;

        if (line[0] == '[')
        {
            section = line;
            continue;
        }

        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        if (section == "[BASE]")
        {
            uintptr_t offset = strtoull(val.c_str(), nullptr, 16);
            BaseMap[key] = moduleBase + offset;
        }
        else if (section == "[STATIC]")
        {
            uintptr_t offset = strtoull(val.c_str(), nullptr, 16);
            AddressMap[key] = moduleBase + offset;
        }
        else if (section == "[POINTER]")
        {
            size_t colon = val.find(':');
            if (colon == std::string::npos)
                continue;

            std::string baseName = val.substr(0, colon);
            std::string offsetsStr = val.substr(colon + 1);

            std::vector<unsigned int> offsets;
            std::stringstream ss(offsetsStr);
            std::string item;

            while (std::getline(ss, item, ','))
                offsets.push_back(strtoul(item.c_str(), nullptr, 16));

            if (BaseMap.find(baseName) != BaseMap.end())
                AddressMap[key] = FindDMAAddy(hProcess, BaseMap[baseName], offsets);
        }
        else if (section == "[PATCH]")
        {
            std::stringstream ss(val);
            std::string addrStr, onStr, offStr;

            std::getline(ss, addrStr, '|');
            std::getline(ss, onStr, '|');
            std::getline(ss, offStr, '|');

            Patch p;
            p.address = moduleBase + strtoull(addrStr.c_str(), nullptr, 16);
            p.onBytes = HexToBytes(onStr);
            p.offBytes = HexToBytes(offStr);

            PatchMap[key] = p;
        }
    }
}

bool ApplyPatchByName(const std::string& name, bool enabled)
{
    auto it = PatchMap.find(name);
    if (it == PatchMap.end())
        return false;

    Patch& p = it->second;
    if (!p.address)
        return false;

    const std::vector<BYTE>& bytes = enabled ? p.onBytes : p.offBytes;
    if (bytes.empty())
        return false;

    mem::PatchEx((BYTE*)p.address, (BYTE*)bytes.data(), (unsigned int)bytes.size(), hProcess);
    return true;
}

ImVec4 RainbowColor(float frequency, float phase, float amplitude)
{
    float time = ImGui::GetTime();
    float r = sin(frequency * time + 0 + phase) * amplitude + 0.5f;
    float g = sin(frequency * time + 2 + phase) * amplitude + 0.5f;
    float b = sin(frequency * time + 4 + phase) * amplitude + 0.5f;
    return ImVec4(r, g, b, 1.0f);
}


void ResetAndReinit()
{
    std::cout << "[~] Resetting...\n";

    
    if (bRainbow)
    {
        stopThread = true;
        if (rainbowThread.joinable())
            rainbowThread.join();
        bRainbow = false;
        stopThread = false;
    }

   
    if (hProcess)
    {
        CloseHandle(hProcess);
        hProcess = nullptr;
    }


    procId = 0;
    moduleBase = 0;

    BaseMap.clear();
    AddressMap.clear();
    PatchMap.clear();

    LocalPlayerOffset = 0;
    IonCubeAddr = 0;
    BlueCubeAddr = 0;
    SalvageAddr = 0;
    SalvageBaseAddr = 0;
    CatAddr = 0;
    healthAddr = 0;
    BypassColorAddr = 0;
    EnergyAddr = 0;
    AugBaseAddr = 0;
    Aug1Addr = 0;
    JumpAddr = 0;
    MemoriaAddr = 0;
    TitanShards = 0;
    SoulChipsAddr = 0;
    ColorAddr = 0;
    BoltsAddr = 0;
    MWeaponAddr = 0;
    QWeaponAddr = 0;
    WWeaponAddr = 0;
    EWeaponAddr = 0;
    PhysicalDamagePtr = 0;
    AbilityDamagePtr = 0;
    SpeedPtr = 0;
    JumpHeightPtr = 0;
    xCoord = 0;
    zCoord = 0;
    WingsAddr = 0;
    EntityListAddr = 0;
    ScrapCmpAddr = 0;
    ScrapAddr = 0;
    OMWeaponId = 0;

 
    bHealth = false;
    bEnergy = false;
    bInstant = false;
    bJump = false;
    bTest2 = false;
    bTest3 = false;
    bScrap = false;
    ItsElectric = false;

    
    procId = GetProcId(L"30XX.exe");
    if (procId != 0)
    {
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
        if (hProcess)
        {
            moduleBase = GetModuleBaseAddress(procId, L"30XX.exe");

            std::string data = DownloadOffsets(
                "https://raw.githubusercontent.com/Jay-Lexington404/UwU-30XX/main/Main.txt"
            );

            if (!data.empty())
            {
                ParseOffsets(data);

                LocalPlayerOffset = BaseMap["LocalPlayer"];

                healthAddr = AddressMap["Health"];
                EnergyAddr = AddressMap["Energy"];
                MWeaponAddr = AddressMap["MWeapon"];
                QWeaponAddr = AddressMap["QWeapon"];
                WWeaponAddr = AddressMap["WWeapon"];
                EWeaponAddr = AddressMap["EWeapon"];
                PhysicalDamagePtr = AddressMap["PhysicalDamage"];
                AbilityDamagePtr = AddressMap["AbilityDamage"];
                SpeedPtr = AddressMap["Speed"];
                JumpHeightPtr = AddressMap["JumpHeight"];
                ColorAddr = AddressMap["Color"];
                IonCubeAddr = AddressMap["IonCube"];
                BlueCubeAddr = AddressMap["BlueCube"];
                BoltsAddr = AddressMap["Bolts"];
                MemoriaAddr = AddressMap["Memoria"];
                SoulChipsAddr = AddressMap["SoulChips"];

                if (PatchMap.find("ScrapCmp") != PatchMap.end())
                    ScrapCmpAddr = PatchMap["ScrapCmp"].address;

                if (PatchMap.find("Scrap") != PatchMap.end())
                    ScrapAddr = PatchMap["Scrap"].address;

                ReadProcessMemory(hProcess, (LPCVOID)MWeaponAddr, &OMWeaponId, sizeof(OMWeaponId), nullptr);

                std::cout << "[+] Reset successful! Process reattached.\n";
            }
            else
            {
                std::cout << "[-] Reset: Failed to load offsets\n";
            }
        }
        else
        {
            std::cout << "[-] Reset: Failed to open process\n";
        }
    }
    else
    {
        std::cout << "[-] Reset: Process not found (is 30XX.exe running?)\n";
    }
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void RainbowCycleLoop()
{
    while (!stopThread)
    {
        for (int value = 0; value <= 13; ++value)
        {
            if (stopThread)
                break;

            if (ColorAddr && hProcess)
            {
                mem::PatchEx(
                    (BYTE*)ColorAddr,
                    (BYTE*)&value,
                    sizeof(value),
                    hProcess
                );
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void DrawDebugWatermark(HANDLE hProc, uintptr_t weaponAddr)
{
    if (!bTest2)
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (!drawList)
        return;

    int weaponID = 0;
    if (weaponAddr && hProc)
        ReadProcessMemory(hProc, (LPCVOID)weaponAddr, &weaponID, sizeof(weaponID), nullptr);

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "W-ID = %d", weaponID);

    ImVec2 textSize = ImGui::CalcTextSize(buffer);
    ImVec2 pos(
        (io.DisplaySize.x - textSize.x) * 0.5f,
        (io.DisplaySize.y - textSize.y) * 0.5f
    );

    float time = static_cast<float>(ImGui::GetTime());
    float r = sinf(time * 1.5f + 0.0f) * 0.5f + 0.5f;
    float g = sinf(time * 1.5f + 2.0f) * 0.5f + 0.5f;
    float b = sinf(time * 1.5f + 4.0f) * 0.5f + 0.5f;

    ImU32 rainbowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1.0f));
    ImU32 shadowColor = IM_COL32(0, 0, 0, 150);

    drawList->AddText(ImVec2(pos.x + 1, pos.y + 1), shadowColor, buffer);
    drawList->AddText(pos, rainbowColor, buffer);
}

void DrawTextWatermark(const char* watermarkText)
{
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    ImVec2 padding(10.0f, 10.0f);
    ImVec2 textSize = ImGui::CalcTextSize(watermarkText);

    ImVec2 pos(
        io.DisplaySize.x - textSize.x - padding.x,
        io.DisplaySize.y - textSize.y - padding.y
    );

    float time = static_cast<float>(ImGui::GetTime());
    float r = sinf(time * 1.5f + 0.0f) * 0.5f + 0.5f;
    float g = sinf(time * 1.5f + 2.0f) * 0.5f + 0.5f;
    float b = sinf(time * 1.5f + 4.0f) * 0.5f + 0.5f;

    ImU32 rainbowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1.0f));
    ImU32 shadowColor = IM_COL32(0, 0, 0, 150);

    drawList->AddText(ImVec2(pos.x + 1, pos.y + 1), shadowColor, watermarkText);
    drawList->AddText(pos, rainbowColor, watermarkText);
}

int main(int, char**)
{
    system("color 4");

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor)
        return 0;

    int width = glfwGetVideoMode(monitor)->width;
    int height = glfwGetVideoMode(monitor)->height;

    glfwWindowHint(GLFW_FLOATING, true);
    glfwWindowHint(GLFW_RESIZABLE, false);
    glfwWindowHint(GLFW_MAXIMIZED, true);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, true);

    GLFWwindow* window = glfwCreateWindow(width, height, "IX Menu", nullptr, nullptr);
    if (window == nullptr)
        return 1;

    glfwSetWindowAttrib(window, GLFW_DECORATED, false);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    bool bMenuVisible = true;
    static auto lastCheckTime = std::chrono::steady_clock::now();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        DrawTextWatermark("X v0.1.0");
        DrawDebugWatermark(hProcess, MWeaponAddr);

        if (GetAsyncKeyState(VK_INSERT) & 1)
        {
            bMenuVisible = !bMenuVisible;
            if (bMenuVisible)
                ShowMenu(window);
            else
                HideMenu(window);
        }

        // ── Auto-attach check (every 2s) ──────────────────────────────
        {
            auto now = std::chrono::steady_clock::now();
            bool shouldCheck = std::chrono::duration_cast<std::chrono::seconds>(
                now - lastCheckTime).count() >= 2;

            if (shouldCheck)
            {
                lastCheckTime = now;
                bool processLost = (hProcess && WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0);

                if (processLost)
                {
                    std::cout << "[!] Process lost, reattaching...\n";
                    ResetAndReinit();
                }
                else if (procId == 0)
                {
                    procId = GetProcId(L"30XX.exe");
                    if (procId != 0)
                    {
                        hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
                        if (hProcess)
                        {
                            moduleBase = GetModuleBaseAddress(procId, L"30XX.exe");
                            std::string data = DownloadOffsets(
                                "https://raw.githubusercontent.com/Jay-Lexington404/UwU-30XX/main/Main.txt"
                            );
                            if (!data.empty())
                            {
                                ParseOffsets(data);
                                LocalPlayerOffset = BaseMap["LocalPlayer"];
                                healthAddr = AddressMap["Health"];
                                EnergyAddr = AddressMap["Energy"];
                                MWeaponAddr = AddressMap["MWeapon"];
                                QWeaponAddr = AddressMap["QWeapon"];
                                WWeaponAddr = AddressMap["WWeapon"];
                                EWeaponAddr = AddressMap["EWeapon"];
                                PhysicalDamagePtr = AddressMap["PhysicalDamage"];
                                AbilityDamagePtr = AddressMap["AbilityDamage"];
                                SpeedPtr = AddressMap["Speed"];
                                JumpHeightPtr = AddressMap["JumpHeight"];
                                ColorAddr = AddressMap["Color"];
                                IonCubeAddr = AddressMap["IonCube"];
                                BlueCubeAddr = AddressMap["BlueCube"];
                                BoltsAddr = AddressMap["Bolts"];
                                MemoriaAddr = AddressMap["Memoria"];
                                SoulChipsAddr = AddressMap["SoulChips"];
                                if (PatchMap.find("ScrapCmp") != PatchMap.end())
                                    ScrapCmpAddr = PatchMap["ScrapCmp"].address;
                                if (PatchMap.find("Scrap") != PatchMap.end())
                                    ScrapAddr = PatchMap["Scrap"].address;
                                ReadProcessMemory(hProcess, (LPCVOID)MWeaponAddr,
                                    &OMWeaponId, sizeof(OMWeaponId), nullptr);
                                std::cout << "IX v0.0.2 Loaded! UwU\n";
                                std::cout << "LocalPlayer: 0x" << std::hex << LocalPlayerOffset << "\n";
                                std::cout << "Main Weapon: 0x" << std::hex << MWeaponAddr << "\n";
                            }
                            else
                            {
                                std::cout << "[-] Failed to load offsets\n";
                            }
                        }
                    }
                    else
                    {
                        std::cout << "[~] Waiting for 30XX.exe...\n";
                    }
                }
            }
        }
        // ── End auto-attach ───────────────────────────────────────────

        if (bMenuVisible)
        {
            ImGui::Begin("IX Menu SynX Edition V0.0.1", nullptr, ImGuiWindowFlags_MenuBar);

            if (ImGui::BeginMenuBar())
            {
                float totalWidth = 200.0f;
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() - totalWidth);

                if (ImGui::SmallButton("Reset / Reattach"))
                    ResetAndReinit();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Re-finds 30XX.exe and reloads offsets.");

                ImGui::SameLine();

                if (procId != 0 && hProcess)
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "● Online");
                else
                    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "● Offline");

                ImGui::EndMenuBar();
            }

            ImGui::Checkbox("Health", &bHealth);
            ImGui::Checkbox("Unlimited Power", &bEnergy);
            ImGui::Checkbox("Instant-Kill", &bInstant);
            ImGui::Checkbox("Fly-Hack", &bJump);
            ImGui::Checkbox("Unlimited Scrapbits", &bScrap);
            ImGui::Checkbox("Debug Mode", &bTest2);

            if (ImGui::Button("Toggle Rainbow"))
            {
                bRainbow = !bRainbow;
                if (bRainbow)
                {
                    if (!rainbowThread.joinable())
                    {
                        stopThread = false;
                        rainbowThread = std::thread(RainbowCycleLoop);
                    }
                }
                else
                {
                    stopThread = true;
                    if (rainbowThread.joinable())
                        rainbowThread.join();
                    int staticColorValue = 0;
                    if (ColorAddr && hProcess)
                        mem::PatchEx((BYTE*)ColorAddr, (BYTE*)&staticColorValue, sizeof(staticColorValue), hProcess);
                }
            }

            if (ImGui::Button("Toggle DLC Unlock"))
            {
                ShellExecuteW(nullptr, L"open", L"https://www.youtube.com/watch?v=dQw4w9WgXcQ",
                    nullptr, nullptr, SW_SHOWNORMAL);
            }

            ImGui::InputInt("Main-Weapon", &MWeaponId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused())
                mem::PatchEx((BYTE*)MWeaponAddr, (BYTE*)&MWeaponId, sizeof(MWeaponId), hProcess);

            ImGui::InputInt("Q-Weapon", &QWeaponId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused())
                mem::PatchEx((BYTE*)QWeaponAddr, (BYTE*)&QWeaponId, sizeof(QWeaponId), hProcess);

            ImGui::InputInt("W-Weapon", &WWeaponId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused())
                mem::PatchEx((BYTE*)WWeaponAddr, (BYTE*)&WWeaponId, sizeof(WWeaponId), hProcess);

            ImGui::InputInt("E-Weapon", &EWeaponId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused())
                mem::PatchEx((BYTE*)EWeaponAddr, (BYTE*)&EWeaponId, sizeof(EWeaponId), hProcess);

            ImGui::InputInt("Damage", &PhysicalDamageId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused())
                mem::PatchEx((BYTE*)PhysicalDamagePtr, (BYTE*)&PhysicalDamageId, sizeof(PhysicalDamageId), hProcess);

            ImGui::InputInt("Ability-Damage", &AbilityDamageId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused())
                mem::PatchEx((BYTE*)AbilityDamagePtr, (BYTE*)&AbilityDamageId, sizeof(AbilityDamageId), hProcess);

            ImGui::InputInt("Speed", &SpeedId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused())
                mem::PatchEx((BYTE*)SpeedPtr, (BYTE*)&SpeedId, sizeof(SpeedId), hProcess);

            ImGui::InputInt("Jump-Height", &JumpHeightId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused())
                mem::PatchEx((BYTE*)JumpHeightPtr, (BYTE*)&JumpHeightId, sizeof(JumpHeightId), hProcess);

            ImGui::InputInt("Memoria", &MemoriaId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused())
                mem::PatchEx((BYTE*)MemoriaAddr, (BYTE*)&MemoriaId, sizeof(MemoriaId), hProcess);

            ImGui::InputInt("Bolts", &BoltsId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused())
                mem::PatchEx((BYTE*)BoltsAddr, (BYTE*)&BoltsId, sizeof(BoltsId), hProcess);

            ImGui::InputInt("Re-Rolls", &IonCubeId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused())
                mem::PatchEx((BYTE*)IonCubeAddr, (BYTE*)&IonCubeId, sizeof(IonCubeId), hProcess);

            ImGui::InputInt("Entropy", &BlueCubeId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused())
                mem::PatchEx((BYTE*)BlueCubeAddr, (BYTE*)&BlueCubeId, sizeof(BlueCubeId), hProcess);

            if (ImGui::Button("Exit"))
                glfwSetWindowShouldClose(window, GLFW_TRUE);

            ImVec4 rainbowTextColor = RainbowColor(1.0f, 0.3f, 1.0f);
            ImGui::TextColored(rainbowTextColor, "Cheat the game because, believe me, it doesn't mind cheating you. ~ CTG");
            ImGui::TextColored(rainbowTextColor, R"(
                   __              __
                   \ `-._......_.-` /
                    `.  '.    .'  .'
                     //  _`\/`_  \\
                    ||  /\O||O/\  ||
                    |\  \_/||\_/  /|
                    \ '.   \/   .' /
                    / ^ `'~  ~'`   \
                   /  _-^_~ -^_ ~-  |
                   | / ^_ -^_- ~_^\ |
                   | |~_ ^- _-^_ -| |
                   | \  ^-~_ ~-_^ / |
                   \_/;-.,____,.-;\_/
            =jgs======(_(_(==)_)_)=========

           ==================================

)");

            ImGui::End();
        }

        if (bHealth && healthAddr != 0)
        {
            const int god = 13337;
            Sleep(5);
            mem::PatchEx((BYTE*)healthAddr, (BYTE*)&god, sizeof(god), hProcess);
        }

        if (bEnergy && EnergyAddr != 0)
        {
            const int uEnergy = 13337;
            Sleep(5);
            mem::PatchEx((BYTE*)EnergyAddr, (BYTE*)&uEnergy, sizeof(uEnergy), hProcess);
        }

        ApplyPatchByName("JumpPatch", bJump);
        ApplyPatchByName("InstantKill", bInstant);
        ApplyPatchByName("RainbowPatch", bRainbow);
        ApplyPatchByName("ScrapCmp", bScrap);
        ApplyPatchByName("Scrap", bScrap);

        if (bTest2)
        {
            static SHORT prevF3State = 0;
            static SHORT prevF4State = 0;

            int n1 = 0;
            ReadProcessMemory(hProcess, (LPCVOID)MWeaponAddr, &n1, sizeof(n1), nullptr);

            SHORT currentF3State = GetAsyncKeyState(VK_F3);
            if ((currentF3State & 0x8000) && !(prevF3State & 0x8000))
            {
                n1--;
                mem::PatchEx((BYTE*)MWeaponAddr, (BYTE*)&n1, sizeof(n1), hProcess);
            }
            prevF3State = currentF3State;

            SHORT currentF4State = GetAsyncKeyState(VK_F4);
            if ((currentF4State & 0x8000) && !(prevF4State & 0x8000))
            {
                n1++;
                mem::PatchEx((BYTE*)MWeaponAddr, (BYTE*)&n1, sizeof(n1), hProcess);
            }
            prevF4State = currentF4State;
        }
        else
        {
            mem::PatchEx((BYTE*)MWeaponAddr, (BYTE*)&OMWeaponId, sizeof(OMWeaponId), hProcess);
        }

        if (bTest3)
        {
            int n1 = 0;
            ReadProcessMemory(hProcess, (LPCVOID)MWeaponAddr, &n1, sizeof(n1), nullptr);
        }

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    stopThread = true;
    if (rainbowThread.joinable())
        rainbowThread.join();

    return 0;
}