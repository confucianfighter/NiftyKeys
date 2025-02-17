#include "ModeManager.h"
#include <fstream>
#include <iostream>
#include "KeyState.h"
#include "CharToVK.h"
#include "InputSimulator.h"
#include "test_mouse_input.cpp"
// Define the static activationMap using VK codes as keys.
std::vector<Mode *> Mode::modes;
int timeout = 200;
Mode *Mode::currentMode = nullptr;
Mode::Mode(
    const std::string &name,
    const std::unordered_map<int, int> &keyMapping,
    const std::vector<int> &activationKeys)
    : name(name), keyMapping(keyMapping), activationKeys(activationKeys)
{
    // Register each activation key in the static activationMap,
    // converting each key (if needed) to its corresponding VK code.
    // Since activationKeys are already ints (VK codes), this step might be redundant,
    // but if CharToVK needs to be applied, you can do so.
    for (int key : activationKeys)
    {
        // For demonstration, assume key is already a VK code.
        modes[key] = this;
    }
}

bool Mode::handleKeyUpEvent(int keycode)
{
    bool handled = false;
    // check if the keycode is in the keyMapping.
    auto it = keyMapping.find(keycode);

    if (it != keyMapping.end())
    {
        std::lock_guard<std::mutex> lock(keyStatesMutex);
        auto it = keyStates.find(keycode);
        KeyState state = it->second;
        state.timeReleased = GetTickCount64();
        state.held = false;
        std::cout << "Key down: " << keycode << " remapped to " << it->first << std::endl;
        handled = true;
    }
    // Additional logic for key up events can go here.
    return handled;
}

bool Mode::handleKeyDownEvent(int keycode)
{
    bool handled = false;
    // check if this is the activation key.
    if (keycode == keyCodeActivatedBy)
    {
        return;
    }
    auto key = keyMapping.find(keycode);
    if (key != keyMapping.end())
    {
        std::lock_guard<std::mutex> lock(keyStatesMutex);
        auto it = keyStates.find(keycode);
        KeyState state = it->second;
        state.timePressed = GetTickCount64();
        state.held = true;
        InputSimulator::simulateKeyTap(key->second);
        std::cout << "Key down: " << keycode << " remapped to " << it->first << std::endl;
        handled = true;
    }
    // Additional logic for key down events can go here.
    return handled;
}

const std::string &Mode::getName() const
{
    return name;
}

const std::unordered_map<int, int> &Mode::getKeyMapping() const
{
    return keyMapping;
}

const std::vector<int> &Mode::getActivationKeys() const
{
    return activationKeys;
}

std::vector<Mode *> Mode::loadModes(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error opening modes JSON file: " << filename << std::endl;
        return modes;
    }

    try
    {
        nlohmann::json jsonData;
        file >> jsonData;
        if (!jsonData.contains("modes") || !jsonData["modes"].is_array())
        {
            std::cerr << "Invalid JSON format: 'modes' array missing." << std::endl;
            return modes;
        }

        for (const auto &modeEntry : jsonData["modes"])
        {
            std::string modeName = modeEntry.value("name", "UnnamedMode");
            std::cout << "Loading mode " << modeName << std::endl;

            // Read and convert activation keys to VK codes.
            std::vector<int> activationKeys;
            if (modeEntry.contains("activation_keys") && modeEntry["activation_keys"].is_array())
            {
                for (const auto &keyVal : modeEntry["activation_keys"])
                {
                    std::string keyStr = keyVal.get<std::string>();
                    if (!keyStr.empty())
                    {
                        int vk = CharToVK(keyStr[0]);
                        activationKeys.push_back(vk);
                    }
                }
            }

            // Read key mapping and convert both keys and mapped values.
            std::unordered_map<int, int> keyMapping;
            if (modeEntry.contains("key_mapping") && modeEntry["key_mapping"].is_object())
            {
                for (auto it = modeEntry["key_mapping"].begin(); it != modeEntry["key_mapping"].end(); ++it)
                {
                    std::string src = it.key();
                    std::string dest = it.value().get<std::string>();
                    if (!src.empty() && !dest.empty())
                    {
                        int srcVK = CharToVK(src[0]);
                        int destVK = CharToVK(dest[0]);
                        keyMapping[srcVK] = destVK;
                    }
                }
            }
            // Create a new Mode object and add it to our vector.
            Mode *mode = new Mode(modeName, keyMapping, activationKeys);
            modes.push_back(mode);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error parsing modes JSON: " << e.what() << std::endl;
    }
    return modes;
}

// return this as a pointer to the current mode.

bool Mode::checkActiveModeEnded(int vkCode)
{
    bool handled = false;
    Mode *currentMode = nullptr;
    if (Mode::currentMode != nullptr)
    {
        if (vkCode == Mode::currentMode->keyCodeActivatedBy)
        {
            // check for timeout
            DWORD now = GetTickCount64();
            DWORD heldTime = now - keyStates[vkCode].timePressed;
            if (heldTime < timeout)
            {
                InputSimulator::simulateKeyTap(vkCode);
            }
            Mode::currentMode = nullptr;
            handled = true;
        }
    }
    return handled;
}
bool Mode::checkIfActivatesMode(int vkCode)
{
    bool handled = false;
    for (Mode *mode : Mode::modes)
    {
        auto it = std::find(mode->activationKeys.begin(), mode->activationKeys.end(), vkCode);
        if (it != mode->activationKeys.end())
        {
            Mode::currentMode = mode;
            Mode::currentMode->keyCodeActivatedBy = vkCode;
            handled = true;
        }
    }

    return handled;
}
