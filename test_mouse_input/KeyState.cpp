#include "KeyState.h"

std::unordered_map<int, KeyState> keyStates;
std::mutex keyStatesMutex;
