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
#include <ModeManager.h>
#include <KeyState.h>
#include "InputSimulator.h"
// ---------------------------------------------
// Configuration Loading (Optional)
struct Config {
    int click_count;
    int interval_ms;
};

bool loadConfig(const std::string& filename, Config& config) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening configuration file: " << filename << std::endl;
        return false;
    }
    try {
        nlohmann::json jsonConfig;
        file >> jsonConfig;
        config.click_count = jsonConfig.at("click_count").get<int>();
        config.interval_ms = jsonConfig.at("interval_ms").get<int>();
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing configuration: " << e.what() << std::endl;
        return false;
    }
    std::cout << "Configuration loaded: click_count = " << config.click_count
        << ", interval_ms = " << config.interval_ms << std::endl;
    return true;
}

void updateKeyState(int vkCode, bool isDown) {
    std::lock_guard<std::mutex> lock(keyStatesMutex);
    if (isDown) {
        if (keyStates.find(vkCode) == keyStates.end() || !keyStates[vkCode].held) {
            keyStates[vkCode].held = true;
            keyStates[vkCode].timePressed = GetTickCount();
            keyStates[vkCode].timeReleased = 0;
        }
    }
    else {
        if (keyStates.find(vkCode) != keyStates.end()) {
            keyStates[vkCode].held = false;
            keyStates[vkCode].timeReleased = GetTickCount();
        }
    }
}

bool isKeyAlreadyHeld(int vkCode) {
    std::lock_guard<std::mutex> lock(keyStatesMutex);
    return keyStates.count(vkCode) && keyStates[vkCode].held;
}

// ---------------------------------------------
// Constants and Globals for Movement with Acceleration
const int MODE_TIMEOUT = 300; // ms for space tap vs. mode
const double ACCELERATION = 2.0;
const double FRICTION = 0.85;
const double MAX_SPEED = 50.0;
double mouseVelX = 0.0, mouseVelY = 0.0;
const DWORD RAPID_THRESHOLD = 100; // ms

// Leap booleans for each direction.
bool leapLeft = false;
bool leapRight = false;
bool leapUp = false;
bool leapDown = false;

// -----------------

KeyState spaceMode = { VK_SPACE, "space_bar", 0, MODE_TIMEOUT, false };
bool spacePressed = false;  // Is SPACE held?
bool modeUsed = false;      // Were any movement/mouse keys used while SPACE held?

// ---------------------------------------------
// Polling Thread: continuously update mouse position.
// This thread now first checks if any leap boolean is set. If so, it calculates the target jump
// for that direction (halfway to the corresponding screen edge) and calls InputSimulator::moveMouse() with that difference.
// Then it resets the leap booleans and resumes normal acceleration movement.
bool running = true;
void pollingThread() {
    while (running) {
        if (spacePressed) {
            // First, check for leap jumps.
            POINT pos;
            if (GetCursorPos(&pos)) {
                int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                int screenHeight = GetSystemMetrics(SM_CYSCREEN);
               
                if (leapLeft) {
                    int targetX = pos.x / 2;
                    int diffX = targetX - pos.x;
                    InputSimulator::moveMouse(diffX/2, 0);
                }
                if (leapRight) {
                    int targetX = pos.x + (screenWidth - pos.x) / 2;
                    int diffX = targetX - pos.x;
                    InputSimulator::moveMouse(diffX/2, 0);
                }
                if (leapUp) {
                    int targetY = pos.y / 2;
                    int diffY = targetY - pos.y;
                    InputSimulator::moveMouse(0, diffY/2);
                }
                if (leapDown) {
                    int targetY = pos.y + (screenHeight - pos.y) / 2;
                    int diffY = targetY - pos.y;
                    InputSimulator::moveMouse(0, diffY/2);
                }
            }
            // Reset leap booleans after processing.
            leapLeft = leapRight = leapUp = leapDown = false;
            double accelX = 0.0, accelY = 0.0;
            {
                std::lock_guard<std::mutex> lock(keyStatesMutex);

                // Check leftward keys: 'A' and 'J'
                bool leftA = (keyStates.count('A') && keyStates['A'].held);
                bool leftJ = (keyStates.count('K') && keyStates['K'].held);
                if (leftA || leftJ) {
                    double leftAccel = 0.0;
                    if (leftA)
                        leftAccel -= ACCELERATION;
                    if (leftJ)
                        leftAccel -= ACCELERATION;
                    if (leftA && leftJ)
                        leftAccel *= 3;
                    accelX += leftAccel;
                }

                // Check rightward keys: 'D' and VK_OEM_1 (semicolon)
                bool rightD = (keyStates.count('D') && keyStates['D'].held);
                bool rightSemicolon = (keyStates.count(VK_OEM_1) && keyStates[VK_OEM_1].held);
                if (rightD || rightSemicolon) {
                    double rightAccel = 0.0;
                    if (rightD)
                        rightAccel += ACCELERATION;
                    if (rightSemicolon)
                        rightAccel += ACCELERATION;
                    if (rightD && rightSemicolon)
                        rightAccel *= 3;
                    accelX += rightAccel;
                }

                // Check upward keys: 'W' and 'K'
                bool upW = (keyStates.count('W') && keyStates['W'].held);
                bool upK = (keyStates.count('O') && keyStates['O'].held);
                if (upW || upK) {
                    double upAccel = 0.0;
                    if (upW)
                        upAccel -= ACCELERATION;
                    if (upK)
                        upAccel -= ACCELERATION;
                    if (upW && upK)
                        upAccel *= 3;
                    accelY += upAccel;
                }

                // Check downward keys: 'S' and 'L'
                bool downS = (keyStates.count('S') && keyStates['S'].held);
                bool downL = (keyStates.count('L') && keyStates['L'].held);
                if (downS || downL) {
                    double downAccel = 0.0;
                    if (downS)
                        downAccel += ACCELERATION;
                    if (downL)
                        downAccel += ACCELERATION;
                    if (downS && downL)
                        downAccel *= 3;
                    accelY += downAccel;
                }
            }
           

            mouseVelX += accelX;
            mouseVelY += accelY;
            if (accelX == 0.0)
                mouseVelX *= FRICTION;
            if (accelY == 0.0)
                mouseVelY *= FRICTION;
            if (std::abs(mouseVelX) > MAX_SPEED)
                mouseVelX = (mouseVelX > 0 ? MAX_SPEED : -MAX_SPEED);
            if (std::abs(mouseVelY) > MAX_SPEED)
                mouseVelY = (mouseVelY > 0 ? MAX_SPEED : -MAX_SPEED);
            int moveX = static_cast<int>(std::round(mouseVelX));
            int moveY = static_cast<int>(std::round(mouseVelY));
            if (moveX != 0 || moveY != 0) {
                InputSimulator::moveMouse(moveX, moveY);
            }
        }
        else {
            mouseVelX = 0.0;
            mouseVelY = 0.0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
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
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        int vkCode = pKeyboard->vkCode;
        DWORD now = GetTickCount();

        // Process SPACE as the mode key.
        if (vkCode == spaceMode.vkCode) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                if (!spacePressed) {
                    spacePressed = true;
                    modeUsed = false;
                    spaceMode.timePressed = now;
                }
                updateKeyState(vkCode, true);
                return 1;
            }
            else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                spacePressed = false;
                DWORD heldTime = now - spaceMode.timePressed;
                if (!modeUsed && heldTime < static_cast<DWORD>(spaceMode.timeout)) {
                    InputSimulator::simulateKeyTap(VK_SPACE);
                }
                updateKeyState(vkCode, false);
                return 1;
            }
        }
        Mode* mode = Mode::checkActiveMode(keyStates);
        if (mode != nullptr) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                mode->handleKeyDownEvent(nCode);
            }
            else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                mode->handleKeyUpEvent(nCode);
            }
        }
        // If not in space mode, let events pass.
        if (!spacePressed && !mode)
            return CallNextHookEx(hHook, nCode, wParam, lParam);
        
        // Determine if the key is a movement key.
        bool isMovementKey = (vkCode == 'W' || vkCode == 'A' ||
            vkCode == 'S' || vkCode == 'D' ||
            vkCode == 'K' ||
            vkCode == 'L' || vkCode == 'O' || vkCode == VK_OEM_1);
        // Mouse button keys.
        bool isMouseButtonKey = (vkCode == 'Q' || vkCode == 'E' || vkCode == 'H');

        if (isMovementKey) {
            // For each directional group, check if the key is rapidly re-pressed.
            // Left keys: 'A' or 'J'
            if ((vkCode == 'A' || vkCode == 'K') && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
                std::lock_guard<std::mutex> lock(keyStatesMutex);
                if (keyStates.count(vkCode) && !keyStates[vkCode].held &&
                    keyStates[vkCode].timeReleased != 0 &&
                    (now - keyStates[vkCode].timeReleased < RAPID_THRESHOLD))
                {
                    leapLeft = true;
                }
            }
            // Right keys: 'D' or VK_OEM_1
            if ((vkCode == 'D' || vkCode == VK_OEM_1) && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
                std::lock_guard<std::mutex> lock(keyStatesMutex);
                if (keyStates.count(vkCode) && !keyStates[vkCode].held &&
                    keyStates[vkCode].timeReleased != 0 &&
                    (now - keyStates[vkCode].timeReleased < RAPID_THRESHOLD))
                {
                    leapRight = true;
                }
            }
            // Up keys: 'W' or 'K'
            if ((vkCode == 'W' || vkCode == 'O') && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
                std::lock_guard<std::mutex> lock(keyStatesMutex);
                if (keyStates.count(vkCode) && !keyStates[vkCode].held &&
                    keyStates[vkCode].timeReleased != 0 &&
                    (now - keyStates[vkCode].timeReleased < RAPID_THRESHOLD))
                {
                    leapUp = true;
                }
            }
            // Down keys: 'S' or 'L'
            if ((vkCode == 'S' || vkCode == 'L') && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
                std::lock_guard<std::mutex> lock(keyStatesMutex);
                if (keyStates.count(vkCode) && !keyStates[vkCode].held &&
                    keyStates[vkCode].timeReleased != 0 &&
                    (now - keyStates[vkCode].timeReleased < RAPID_THRESHOLD))
                {
                    leapDown = true;
                }
            }
            // Block auto-repeat events.
            if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && isKeyAlreadyHeld(vkCode))
                return 1;
            updateKeyState(vkCode, (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN));
            modeUsed = true;
            return 1;
        }

        if (isMouseButtonKey) {
            if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && isKeyAlreadyHeld(vkCode))
                return 1;
            updateKeyState(vkCode, (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN));
            modeUsed = true;
            switch (vkCode) {
            case 'Q': { // Left mouse button.
                static bool leftDown = false;
                if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && !leftDown) {
                    InputSimulator::simulateLeftDown();
                    leftDown = true;
                    return 1;
                }
                else if ((wParam == WM_KEYUP || wParam == WM_SYSKEYUP) && leftDown) {
                    InputSimulator::simulateLeftUp();
                    leftDown = false;
                    return 1;
                }
                break;
            }
            case 'E': { // Right mouse button.
                static bool rightDown = false;
                if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && !rightDown) {
                    InputSimulator::simulateRightDown();
                    rightDown = true;
                    return 1;
                }
                else if ((wParam == WM_KEYUP || wParam == WM_SYSKEYUP) && rightDown) {
                    InputSimulator::simulateRightUp();
                    rightDown = false;
                    return 1;
                }
                break;
            }
            case 'H': { // Middle mouse button.
                static bool middleDown = false;
                if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && !middleDown) {
                    InputSimulator::simulateMiddleDown();
                    middleDown = true;
                    return 1;
                }
                else if ((wParam == WM_KEYUP || wParam == WM_SYSKEYUP) && middleDown) {
                    InputSimulator::simulateMiddleUp();
                    middleDown = false;
                    return 1;
                }
                break;
            }
            default:
                break;
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// ---------------------------------------------
// Main Function
int main() {
    Mode::loadModes("modes.json");
    Config config;
    const std::string configFile = "config.json";
    if (!loadConfig(configFile, config)) {
        std::cerr << "Error loading configuration. Continuing without config parameters." << std::endl;
    }
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (hHook == NULL) {
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
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    running = false;
    poller.join();
    if (hHook) {
        UnhookWindowsHookEx(hHook);
        hHook = NULL;
    }
    return 0;
}
