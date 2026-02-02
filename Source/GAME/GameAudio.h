#pragma once
#include "../gateware-main/gateware.h"
#include <string>
#include <map>
#include <iostream> // Added for logging

// A simple class to manage all our sounds
class GameAudio
{
public:
    // Call this once at startup
    bool Init();

    // Call this to load all sounds
    void LoadSounds();

    // Call this from anywhere to play a sound
    void Play(std::string soundName);

    // Plays a sound only if it's not already playing.
    void PlayMusic(std::string soundName);

    // Call this from anywhere to stop a sound
    void Stop(std::string soundName);
    void GameAudio::StopMusic(std::string soundName);

private:
    GW::AUDIO::GAudio m_audioEngine;

    // A library to hold all our loaded sounds and music
    std::map<std::string, GW::AUDIO::GSound> m_soundLibrary;

    std::map<std::string, GW::AUDIO::GMusic> m_musicLibrary;
};