#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include "nlohmann/json.hpp" // Make sure the include path is correct
#include "KeyState.h"

// The Mode class encapsulates a mode that remaps keys.
// For example, a mode might map "ASDFGHJKL;" to "1234567890".
class Mode
{
public:
    // Constructor:
    //   name - name of the mode.
    //   keyMapping - mapping from source keys to target keys.
    //   activationKeys - list of keys that activate this mode.
    Mode(const std::string &name,
         const std::unordered_map<int, int> &keyMapping,
         const std::vector<int> &activationKeys);

    // Getters:
    const std::string &getName() const;
    const std::unordered_map<int, int> &getKeyMapping() const;
    const std::vector<int> &getActivationKeys() const;

    // Static method to load modes from a JSON file.
    // Returns a vector of pointers to Mode objects.
    static std::vector<Mode *> loadModes(const std::string &filename);

    // A static dictionary mapping each activation key to a Mode pointer.
    // If a mode has more than one activation key, each is a key in this dictionary.
    static std::vector<Mode *> modes;

    // Static method that loops through the activationMap and checks
    // if that key is currently held down. 'keyStates' is a dictionary (from your main code)
    // mapping a key (char) to a bool indicating whether the key is currently held.
    // Returns a pointer to the active Mode, or nullptr if none.
    // In Mode.h, add the declaration:
    static bool checkIfActivatesMode(int vkCode);
    static bool checkActiveModeEnded(int vkCode);
    virtual bool handleKeyUpEvent(int keycode);
    virtual bool handleKeyDownEvent(int keycode);
    int keyCodeActivatedBy;
    static Mode *currentMode;
    std::vector<int> activationKeys;
    virtual void Update();

private:
    std::string name;
    std::unordered_map<int, int> keyMapping;

    // Optionally, you can add more fields if needed.
};

#pragma once
