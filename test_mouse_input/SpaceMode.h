#pragma once
#include "ModeManager.h"
#include "InputSimulator.h"
class SpaceMode :
    public Mode
{
    void handleKeyDownEvent(int vkCode) {


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
            if (vkCode == 'A' || vkCode == 'K') {//&& (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
                std::lock_guard<std::mutex> lock(keyStatesMutex);
                if (keyStates.count(vkCode) && !keyStates[vkCode].held &&
                    keyStates[vkCode].timeReleased != 0 &&
                    (now - keyStates[vkCode].timeReleased < RAPID_THRESHOLD))
                {
                    leapLeft = true;
                }
            }
            // Right keys: 'D' or VK_OEM_1
            if (vkCode == 'D' || vkCode == VK_OEM_1) {//&& (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
                std::lock_guard<std::mutex> lock(keyStatesMutex);
                if (keyStates.count(vkCode) && !keyStates[vkCode].held &&
                    keyStates[vkCode].timeReleased != 0 &&
                    (now - keyStates[vkCode].timeReleased < RAPID_THRESHOLD))
                {
                    leapRight = true;
                }
            }
            // Up keys: 'W' or 'K'
            if (vkCode == 'W' || vkCode == 'O'){ //&& (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
                std::lock_guard<std::mutex> lock(keyStatesMutex);
                if (keyStates.count(vkCode) && !keyStates[vkCode].held &&
                    keyStates[vkCode].timeReleased != 0 &&
                    (now - keyStates[vkCode].timeReleased < RAPID_THRESHOLD))
                {
                    leapUp = true;
                }
            }
            // Down keys: 'S' or 'L'
            if (vkCode == 'S' || vkCode == 'L'){// && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
                std::lock_guard<std::mutex> lock(keyStatesMutex);
                if (keyStates.count(vkCode) && !keyStates[vkCode].held &&
                    keyStates[vkCode].timeReleased != 0 &&
                    (now - keyStates[vkCode].timeReleased < RAPID_THRESHOLD))
                {
                    leapDown = true;
                }
            }
            // Block auto-repeat events.
            if (/*(wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && */
                isKeyAlreadyHeld(vkCode))
                return 1;
            updateKeyState(vkCode, true);
            //modeUsed = true;
            return 1;
        }

        if (isMouseButtonKey) {
            if (isKeyAlreadyHeld(vkCode))
                return 1;
            updateKeyState(vkCode,true);
            //modeUsed = true;
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

    bool isKeyAlreadyHeld(int vkCode) {
        std::lock_guard<std::mutex> lock(keyStatesMutex);
        return keyStates.count(vkCode) && keyStates[vkCode].held;
    }

   

};

