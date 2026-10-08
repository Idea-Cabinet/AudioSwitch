#pragma once
struct ChordResult { bool trigger; bool suppress; };
struct ChordState {
    bool prefixHeld = false;
    bool keyHeld = false;
    bool consuming = false;
    ChordResult Process(bool prefix, bool key, bool down) {
        if (prefix) { prefixHeld = down; return {false, false}; }
        if (!key) return {false, false};
        if (down) {
            bool trigger = !keyHeld && prefixHeld;
            keyHeld = true;
            if (trigger) consuming = true;
            return {trigger, consuming};
        }
        bool suppress = consuming;
        keyHeld = consuming = false;
        return {false, suppress};
    }
};
