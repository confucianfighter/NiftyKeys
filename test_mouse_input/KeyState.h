#pragma once
#include <unordered_map>
#include <mutex>
#include <windows.h>

struct KeyState {
    int vkCode = 0;
    std::string name = "no name";
    DWORD timePressed = 0;
    int timeout = 200;
    bool held = false;
        // Timestamp when the key was first pressed.
    DWORD timeReleased = 0;
    
    
    
    
};

// Declare the global keyStates dictionary and its mutex as extern
extern std::unordered_map<int, KeyState> keyStates;
extern std::mutex keyStatesMutex;
