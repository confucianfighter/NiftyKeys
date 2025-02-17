#pragma once
#include "ModeManager.h"
#include "InputSimulator.h"
class SpaceMode : public Mode
{
    // ms for space tap vs. mode
    const double ACCELERATION = 2.0;
    const double FRICTION = 0.85;
    const double MAX_SPEED = 50.0;
    double mouseVelX = 0.0, mouseVelY = 0.0;
    const DWORD RAPID_THRESHOLD = 100; // ms
    // constructor
public:
    SpaceMode(const std::string &name, const std::unordered_map<int, int> &keymapping, const std::vector<int> &keyCodes)
        : Mode(name, keymapping, keyCodes)
    {
    }

    bool handleKeyDownEvent(int vkCode)
    {
        bool handled = false;
        DWORD now = GetTickCount64();
        // if already held, return true.
        if (isKeyAlreadyHeld(vkCode))
        {
            handled = true;
        }
        else
        {
            switch (vkCode)
            {
            case 'Q':
            { // Left mouse button.
                InputSimulator::simulateLeftDown();
                handled = true;
                break;
            }
            case 'E':
            { // Right mouse button.k
                InputSimulator::simulateRightDown();
                handled = true;
                break;
            }   
            case 'H':
            { // Middle mouse button.
                InputSimulator::simulateMiddleDown();
                handled = true;
            }
            // mark it also as handled if awsd k, l, ; were pressed.
            case 'A':
            case 'W':
            case 'S':
            case 'D':
            case 'K':
            case 'L':
            case 'O':
            case VK_OEM_1:
                handled = true;
                break;
            default:

                break;
            }
        }
        return handled;
    }

    bool handleKeyUpEvent(int vkCode)
    {
        bool handled = false;

        switch (vkCode)
        {
        case 'Q':
            InputSimulator::simulateLeftUp();
            handled = true;

            break;

        case 'E':
            // Right mouse button.
            InputSimulator::simulateRightUp();
            handled = true;
            break;

        case 'H':
            InputSimulator::simulateMiddleUp();
            handled = true;
            break;
        case 'A':
        case 'W':
        case 'S':
        case 'D':
        case 'K':
        case 'L':
        case 'O':
        case VK_OEM_1:
            handled = true;
            break;
        default:

            break;
        }
        return handled;
    }
    bool isKeyAlreadyHeld(int vkCode)
    {
        std::lock_guard<std::mutex> lock(keyStatesMutex);
        return keyStates.count(vkCode) && keyStates[vkCode].held;
    }
    void Update() override
    {

        // First, check for leap jumps.
        POINT pos;
        if (GetCursorPos(&pos))
        {
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            double accelX = 0.0, accelY = 0.0;
            {
                std::lock_guard<std::mutex> lock(keyStatesMutex);

                // Check leftward keys: 'A' and 'J'
                bool leftA = (keyStates.count('A') && keyStates['A'].held);
                bool leftJ = (keyStates.count('K') && keyStates['K'].held);
                if (leftA || leftJ)
                {
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
                if (rightD || rightSemicolon)
                {
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
                if (upW || upK)
                {
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
                if (downS || downL)
                {
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
            if (moveX != 0 || moveY != 0)
            {
                InputSimulator::moveMouse(moveX, moveY);
            }
        }
        else
        {
            mouseVelX = 0.0;
            mouseVelY = 0.0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
};
