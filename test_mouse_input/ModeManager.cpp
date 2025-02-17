#include "ModeManager.h"
#include <fstream>
#include <iostream>
#include "KeyState.h"
#include "CharToVK.h"
#include "InputSimulator.h"
#include "SpaceMode.h"
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
}
bool Mode::handleKeyDownEvent(int keycode)
{
    bool handled = false;
    // check if this is the activation key.
    if (keycode == keyCodeActivatedBy)
    {
        handled = true;
    }
    else
    {
        // Use the unordered_map's find() method.
        auto mappingIt = keyMapping.find(keycode);
        bool doSimulateKey = false;
        if (mappingIt != keyMapping.end())
        {
            std::lock_guard<std::mutex> lock(keyStatesMutex);
            // Ensure the key exists in keyStates; if not, add a default KeyState.
            if (keyStates.find(keycode) == keyStates.end())
            {
                keyStates[keycode] = KeyState();
            }
            if (keyStates[keycode].held != true)
            {
                doSimulateKey = true;
                keyStates[keycode].timePressed = GetTickCount64();
                keyStates[keycode].held = true;
                // mappingIt->second is the remapped key (an int)
                // unlock the mutex
                std::cout << "Key down: " << keycode << " remapped to " << mappingIt->second << std::endl;
            }
            handled = true;
        }
        if (doSimulateKey) {
            InputSimulator::simulateKeyTap(mappingIt->second);
        }
    }
    return handled;
}

bool Mode::handleKeyUpEvent(int keycode)
{
    bool handled = false;
    // Use the unordered_map's find() method to check for a key mapping.
    auto mappingIt = keyMapping.find(keycode);
    if (mappingIt != keyMapping.end())
    {
        std::lock_guard<std::mutex> lock(keyStatesMutex);
        // Make sure the key exists in keyStates before accessing.a
        if (keyStates.find(keycode) == keyStates.end())
        {
            keyStates[keycode] = KeyState();
        }
        keyStates[keycode].timeReleased = GetTickCount64();
        keyStates[keycode].held = false;
        std::cout << "Key up: " << keycode << " remapped to " << mappingIt->second << std::endl;
        handled = true;
    }
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
                        std::cout << "found activation key: " << keyStr << " for " << modeName << std::endl;
                        int vk = CharToVK(keyStr[0]);
                        activationKeys.push_back(vk);
                    }
                }
            }

            // Read key mapping and convert both keys and mapped values.
            std::unordered_map<int, int> keyMapping;
            if (modeEntry.contains("key_mapping"))
                std::cout << "key_mapping: " << modeEntry["key_mapping"] << std::endl;
            if (modeEntry.contains("key_mapping"))
            {
                std::cout << "Found key_mapping for " << modeName << ", about to iterate over it" << std::endl;
                for (auto it = modeEntry["key_mapping"].begin(); it != modeEntry["key_mapping"].end(); ++it)
                {
                    std::string src = it.key(); // it.key() already returns a std::string, so no need to call get()
                    std::cout << "src: " << src << std::endl;
                    std::string dest = it.value().get<std::string>();
                    std::cout << "dest: " << dest << std::endl;
                    if (!src.empty() && !dest.empty())
                    {
                        std::cout << "Mapping " << src << " to " << dest << std::endl;
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
    // add Space mode
    Mode *spaceMode = new SpaceMode("Mouse Mode", {{VK_SPACE, VK_SPACE}}, {{VK_SPACE}});
    modes.push_back(spaceMode);
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
    if (Mode::currentMode == nullptr)
    {
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
    }

    return handled;
}
void Mode::Update() {}
