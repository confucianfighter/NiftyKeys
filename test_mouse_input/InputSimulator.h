#pragma once
#include <iostream>
#include <windows.h>

class InputSimulator
{
public:
    // ---------------------------------------------
    // Static Functions to simulate mouse actions

    static void moveMouse(int dx, int dy) {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dx = dx;
        input.mi.dy = dy;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        SendInput(1, &input, sizeof(INPUT));
    }

    static void simulateLeftDown() {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));
    }

    static void simulateLeftUp() {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    static void simulateRightDown() {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        SendInput(1, &input, sizeof(INPUT));
    }

    static void simulateRightUp() {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    static void simulateMiddleDown() {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        SendInput(1, &input, sizeof(INPUT));
    }

    static void simulateMiddleUp() {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    // Simulate a key tap by sending a key down followed by a key up for the given VK code.
    static void simulateKeyTap(int vk_code) {
        INPUT inputs[2] = {};
        // Key down event
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = vk_code;
        // Key up event
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = vk_code;
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, inputs, sizeof(INPUT));
    }
};
