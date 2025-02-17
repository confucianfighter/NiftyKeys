#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <mutex>
#include <cmath>
#include "ModeManager.h"
#include "KeyState.h"
#include "InputSimulator.h"
// ---------------------------------------------
// Configuration Loading (Optional)
struct Config
{
    int click_count;
    int interval_ms;
};

bool loadConfig(const std::string &filename, Config &config)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error opening configuration file: " << filename << std::endl;
        return false;
    }
    try
    {
        nlohmann::json jsonConfig;
        file >> jsonConfig;
        config.click_count = jsonConfig.at("click_count").get<int>();
        config.interval_ms = jsonConfig.at("interval_ms").get<int>();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error parsing configuration: " << e.what() << std::endl;
        return false;
    }
    std::cout << "Configuration loaded: click_count = " << config.click_count
              << ", interval_ms = " << config.interval_ms << std::endl;
    return true;
}

void updateKeyState(int vkCode, bool isDown)
{
    std::lock_guard<std::mutex> lock(keyStatesMutex);
    if (isDown)
    {
        if (keyStates.find(vkCode) == keyStates.end() || !keyStates[vkCode].held)
        {
            keyStates[vkCode].held = true;
            keyStates[vkCode].timePressed = GetTickCount64();
            keyStates[vkCode].timeReleased = 0;
        }
    }
    else
    {
        if (keyStates.find(vkCode) != keyStates.end())
        {
            keyStates[vkCode].held = false;
            keyStates[vkCode].timeReleased = GetTickCount64();
        }
    }
}

bool running = true;
void pollingThread()
{
    while (running)
    {
        if (Mode::currentMode != nullptr)
        {
            Mode::currentMode->Update();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ---------------------------------------------
// Low-level Keyboard Hook Procedure
//
// While SPACE is held (space mode), movement keys (WASD, JKL;) and mouse button keys
// (Q = Left, E = Right, H = Middle) are intercepted. For each movement key, if a rapid re-press
// is detected (by comparing the current press time to the last release), the corresponding leap boolean
// is set.
HHOOK hHook = NULL;
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        bool handled = false;
        KBDLLHOOKSTRUCT *pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        int vkCode = pKeyboard->vkCode;
        DWORD now = GetTickCount64();

        // In the low-level keyboard hook procedure, the return value determines whether the event is consumed:
        // Returning 1 indicates that the key event has been handled (for example, a mode action was taken)
        // and should not be propagated further to other hooks or the system.
        // Returning 0 (or calling CallNextHookEx) means the event was not handled, so it should be passed on
        // for normal processing.
        // Here, SPACE (or any activation key) is used to activate a mode:
        // If a mode is already active, only the release of that key is processed;
        // Otherwise, activation keys are checked to set the current mode.

           if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            handled = Mode::checkIfActivatesMode(vkCode);
            if (!handled && Mode::currentMode != nullptr)
            {
                handled = Mode::currentMode->handleKeyDownEvent(vkCode);
            }
            updateKeyState(vkCode, true);
        }
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
        {
            handled = Mode::checkActiveModeEnded(vkCode);
            if (!handled && Mode::currentMode != nullptr)
            {
         handled = Mode::currentMode->handleKeyUpEvent(vkCode);
            }
            updateKeyState(vkCode, false);
        }
        if (!handled)
        {
            return CallNextHookEx(hHook, nCode, wParam, lParam);
        }
        else
        {
            return 1;
        }
    }
    // the only case in which I haven't consumed the entire event.
    return 0;
}

// ---------------------------------------------
// Main Function
int main()
{
    Mode::loadModes("modes.json");
    Config config;
    const std::string configFile = "config.json";
    if (!loadConfig(configFile, config))
    {
        std::cerr << "Error loading configuration. Continuing without config parameters." << std::endl;
    }
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (hHook == NULL)
    {
        std::cerr << "Failed to install keyboard hook." << std::endl;
        return 1;
    }
    std::thread poller(pollingThread);
    std::cout << "MouseKeys app running in space mode:" << std::endl;
    std::cout << "Movement keys (while SPACE held):" << std::endl;
    std::cout << "  WASD and JKL; control movement with acceleration." << std::endl;
    std::cout << "    (Rapid re-press of a direction key causes a leap/jump half-way to that screen edge)" << std::endl;
    std::cout << "Mouse buttons (while SPACE held):" << std::endl;
    std::cout << "  Q = Left, E = Right, H = Middle (separate down/up events)" << std::endl;
    std::cout << "A quick tap of SPACE sends a normal SPACE." << std::endl;
    std::cout << "Press ESC to exit." << std::endl;
    MSG msg;
    // Passing &msg to GetMessage is correct because GetMessage expects a pointer to a MSG structure.
    // However, pay attention to the declaration of msg above:
    //     MSG msg;\
    // That trailing backslash causes a line continuation, which may accidentally merge your comment
    // or subsequent code with the msg declaration. Remove the backslash so that msg is properly declared.
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    running = false;
    // The red underline on "poller" in "poller.join()" is typically an IDE warning rather than a compilation error.
    // It often appears when the IDE’s static analyzer suspects that the std::thread object might not be joinable.
    // This can happen if:
    //   1. The thread has already been joined or detached, which would make another join() call invalid.
    //   2. The thread object is default constructed (i.e., not associated with a running thread).
    //   3. There’s an issue with project settings or missing includes, causing the IDE to misinterpret the thread’s status.
    // In our case, "poller" is correctly initialized with std::thread poller(pollingThread), and we set "running = false"
    // to ensure the pollingThread eventually exits, making the thread joinable when we call join().
    // Thus, the red underline is most likely a false positive from the IDE’s static analysis.
    poller.join();
    if (hHook)
    {
        UnhookWindowsHookEx(hHook);
        hHook = NULL;
    }
    return 0;
}
