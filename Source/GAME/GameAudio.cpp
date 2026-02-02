#include "GameAudio.h"
#include <iostream>

bool GameAudio::Init()
{
    if (G_FAIL(m_audioEngine.Create()))
    {
        std::cerr << "Failed to create GAudio engine!" << std::endl;
        return false;
    }

    // Set master volume (this is good)
    m_audioEngine.SetMasterVolume(1.0f);
    std::cout << "GAudio engine created. Master volume set to 1.0." << std::endl;

    LoadSounds(); // Load all sounds on init
    return true;
}

void GameAudio::LoadSounds()
{
    // --- SFX ---
    if (G_FAIL(m_soundLibrary["blaster"].Create("../Assets/Audio/Blaster.wav", m_audioEngine, 0.1f)))
    {
        std::cerr << "Failed to load ../Assets/Audio/Blaster.wav!" << std::endl;
    }
    else
    {
        std::cout << "Successfully loaded Blaster.wav" << std::endl;
    }

    if (G_FAIL(m_soundLibrary["PowerUp"].Create("../Assets/Audio/PowerUp.wav", m_audioEngine, 1.0f)))
    {
        std::cerr << "Failed to load ../Assets/Audio/PowerUp.wav!" << std::endl;
    }
    else
    {
        std::cout << "Successfully loaded PowerUp.wav" << std::endl;
    }
    
    if (G_FAIL(m_soundLibrary["GameOver"].Create("../Assets/Audio/PowerUp.wav", m_audioEngine, 1.0f)))
    {
        std::cerr << "Failed to load ../Assets/Audio/GameOver.wav!" << std::endl;
    }
    else
    {
        std::cout << "Successfully loaded GameOver.wav" << std::endl;
    }

    if (G_FAIL(m_soundLibrary["EnemyHit"].Create("../Assets/Audio/EnemyHit.wav", m_audioEngine, 1.0f)))
    {
        std::cerr << "Failed to load ../Assets/Audio/EnemyHit.wav!" << std::endl;
    }
    else
    {
        std::cout << "Successfully loaded EnemyHit.wav" << std::endl;
    }

    if (G_FAIL(m_soundLibrary["PlayerHit"].Create("../Assets/Audio/PlayerHit.wav", m_audioEngine, 1.0f)))
    {
        std::cerr << "Failed to load ../Assets/Audio/PlayerHit.wav!" << std::endl;
    }
    else
    {
        std::cout << "Successfully loaded PlayerHit.wav" << std::endl;
    }

    if (G_FAIL(m_soundLibrary["EnemyExplosion"].Create("../Assets/Audio/EnemyExplosion.wav", m_audioEngine, 1.0f)))
    {
        std::cerr << "Failed to load ../Assets/Audio/EnemyExplosion.wav!" << std::endl;
    }
    else
    {
        std::cout << "Successfully loaded EnemyExplosion.wav" << std::endl;
    }

    if (G_FAIL(m_soundLibrary["NukeExplosion"].Create("../Assets/Audio/NukeExplosion.wav", m_audioEngine, 1.0f)))
    {
        std::cerr << "Failed to load ../Assets/Audio/NukeExplosion.wav!" << std::endl;
    }
    else
    {
        std::cout << "Successfully loaded NukeExplosion.wav" << std::endl;
    }

    if (G_FAIL(m_soundLibrary["ui_hover"].Create("../Assets/Audio/MenuSelect.wav", m_audioEngine, 0.7f))) // 0.7f = 70% volume
    {
        std::cerr << "Failed to load ../Assets/Audio/MenuSelect.wav!" << std::endl;
    }
    else
    {
        std::cout << "Successfully loaded MenuSelect.wav" << std::endl;
    }

    // --- Background Music ---
    // Create the music in the m_musicLibrary
    // GMusic::Create arguments are (path, audioEngine, volume)
    if (G_FAIL(m_musicLibrary["BackgroundMenuMix"].Create("../Assets/Audio/BackgroundMenuMix.wav", m_audioEngine, 0.1f))) // 0.5f = 50% volume
    {
        std::cerr << "Failed to load ../Assets/Audio/BackgroundMenuMix.wav!" << std::endl;
    }
    else
    {
        std::cout << "Successfully loaded BackgroundMenuMix.wav" << std::endl;
    }


}

void GameAudio::Play(std::string soundName)
{
    // Find the sound in our library
    if (m_soundLibrary.find(soundName) != m_soundLibrary.end())
    {
        GW::AUDIO::GSound& sound = m_soundLibrary[soundName];

        sound.Stop();
        sound.Play();
    }
    else
    {
        // This will tell you if you're trying to play a sound that failed to load
        std::cerr << "Tried to play unknown sound: '" << soundName << "'" << std::endl;
    }
}

void GameAudio::PlayMusic(std::string soundName)
{
    // Find the music in the music library
    if (m_musicLibrary.find(soundName) != m_musicLibrary.end())
    {
        GW::AUDIO::GMusic& music = m_musicLibrary[soundName];

        bool playing = true;
        GW::GReturn result = music.isPlaying(playing);

        // Only play if the call succeeded AND it's not already playing
        if (G_PASS(result) && !playing)
        {
            music.Play();
        }
    }
    else
    {
        std::cerr << "Tried to play unknown music: '" << soundName << "'" << std::endl;
    }
}

void GameAudio::Stop(std::string soundName)
{
    if (m_soundLibrary.find(soundName) != m_soundLibrary.end())
    {
        m_soundLibrary[soundName].Stop();
    }
}

void GameAudio::StopMusic(std::string soundName)
{
    // Find the music in the music library
    if (m_musicLibrary.find(soundName) != m_musicLibrary.end())
    {
        GW::AUDIO::GMusic& music = m_musicLibrary[soundName];

        bool playing = false;
        GW::GReturn result = music.isPlaying(playing);

        // Only stop it if the call succeeded AND it's currently playing
        if (G_PASS(result) && playing)
        {
            music.Stop();
        }
    }
    else
    {
        std::cerr << "Tried to stop unknown music: '" << soundName << "'" << std::endl;
    }
}