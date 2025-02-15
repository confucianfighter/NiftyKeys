#pragma once
#include <windows.h>
#include <cctype>

inline int CharToVK(char key) {
    key = static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
    if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9'))
        return static_cast<int>(key);
    switch (key) {
    case ';': return VK_OEM_1;
    case ':': return VK_OEM_1;
    case '=': return VK_OEM_PLUS;
    case '+': return VK_OEM_PLUS;
    case ',': return VK_OEM_COMMA;
    case '<': return VK_OEM_COMMA;
    case '-': return VK_OEM_MINUS;
    case '_': return VK_OEM_MINUS;
    case '.': return VK_OEM_PERIOD;
    case '>': return VK_OEM_PERIOD;
    case '/': return VK_OEM_2;
    case '?': return VK_OEM_2;
    case '`': return VK_OEM_3;
    case '~': return VK_OEM_3;
    case '[': return VK_OEM_4;
    case '{': return VK_OEM_4;
    case '\\': return VK_OEM_5;
    case '|': return VK_OEM_5;
    case ']': return VK_OEM_6;
    case '}': return VK_OEM_6;
    case '\'': return VK_OEM_7;
    case '\"': return VK_OEM_7;
    default: return 0;
    }
}
