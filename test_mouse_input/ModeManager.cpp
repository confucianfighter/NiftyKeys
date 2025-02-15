#include "ModeManager.h"
#include <fstream>
#include <iostream>
#include "KeyState.h"
#include "CharToVK.h"
#include "InputSimulator.h"

// Define the static activationMap using VK codes as keys.
std::unordered_map<int, Mode*> Mode::activationMap;

Mode::Mode(
    const std::string& name,
    const std::unordered_map<int, int>& keyMapping,
    const std::vector<int>& activationKeys)
    : name(name), keyMapping(keyMapping), activationKeys(activationKeys)
{
    // Register each activation key in the static activationMap,
    // converting each key (if needed) to its corresponding VK code.
    // Since activationKeys are already ints (VK codes), this step might be redundant,
    // but if CharToVK needs to be applied, you can do so.
    for (int key : activationKeys) {
        // For demonstration, assume key is already a VK code.
        activationMap[key] = this;
    }
}

void Mode::handleKeyUpEvent(int keycode) {
    auto it = keyMapping.find(keycode);
    if (it != keyMapping.end()) {
        std::lock_guard<std::mutex> lock(keyStatesMutex);
        auto it = keyStates.find(keycode);
        KeyState state = it->second;
        state.timeReleased = GetTickCount64();
        state.held = false;
        std::cout << "Key down: " << keycode << " remapped to " << it->first << std::endl;
    }
    // Additional logic for key up events can go here.
}

void Mode::handleKeyDownEvent(int keycode) {

    auto key = keyMapping.find(keycode);
    if (key != keyMapping.end()) {
        std::lock_guard<std::mutex> lock(keyStatesMutex);
        auto it = keyStates.find(keycode);
        KeyState state = it->second;
        state.timePressed = GetTickCount64();
        state.held = true;
        InputSimulator::simulateKeyTap(key->second);
        std::cout << "Key down: " << keycode << " remapped to " << it->first << std::endl;

    }
    // Additional logic for key down events can go here.
}

const std::string& Mode::getName() const {
    return name;
}

const std::unordered_map<int, int>& Mode::getKeyMapping() const {
    return keyMapping;
}

const std::vector<int>& Mode::getActivationKeys() const {
    return activationKeys;
}

std::vector<Mode*> Mode::loadModes(const std::string& filename) {
    std::vector<Mode*> modes;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening modes JSON file: " << filename << std::endl;
        return modes;
    }

    try {
        nlohmann::json jsonData;
        file >> jsonData;
        if (!jsonData.contains("modes") || !jsonData["modes"].is_array()) {
            std::cerr << "Invalid JSON format: 'modes' array missing." << std::endl;
            return modes;
        }

        for (const auto& modeEntry : jsonData["modes"]) {
            std::string modeName = modeEntry.value("name", "UnnamedMode");
            std::cout << "Loading mode " << modeName << std::endl;

            // Read and convert activation keys to VK codes.
            std::vector<int> activationKeys;
            if (modeEntry.contains("activation_keys") && modeEntry["activation_keys"].is_array()) {
                for (const auto& keyVal : modeEntry["activation_keys"]) {
                    std::string keyStr = keyVal.get<std::string>();
                    if (!keyStr.empty()) {
                        int vk = CharToVK(keyStr[0]);
                        activationKeys.push_back(vk);
                    }
                }
            }

            // Read key mapping and convert both keys and mapped values.
            std::unordered_map<int, int> keyMapping;
            if (modeEntry.contains("key_mapping") && modeEntry["key_mapping"].is_object()) {
                for (auto it = modeEntry["key_mapping"].begin(); it != modeEntry["key_mapping"].end(); ++it) {
                    std::string src = it.key();
                    std::string dest = it.value().get<std::string>();
                    if (!src.empty() && !dest.empty()) {
                        int srcVK = CharToVK(src[0]);
                        int destVK = CharToVK(dest[0]);
                        keyMapping[srcVK] = destVK;
                    }
                }
            }
            // Create a new Mode object and add it to our vector.
            Mode* mode = new Mode(modeName, keyMapping, activationKeys);
            modes.push_back(mode);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing modes JSON: " << e.what() << std::endl;
    }
    return modes;
}

Mode* Mode::checkActiveMode(const std::unordered_map<int, KeyState>& keyStates) {
    for (const auto& entry : Mode::activationMap) {
        int actVK = entry.first;
        auto it = keyStates.find(actVK);
        if (it != keyStates.end() && it->second.held == true) {
            return entry.second;
        }
    }
    return nullptr;
}
