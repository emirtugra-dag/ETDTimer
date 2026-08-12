#ifndef AUDIO_H
#define AUDIO_H

#include <string>

class AudioManager {
public:
    static AudioManager& Instance();
    
    void PlayAlertSound(const std::string& customPath = "");
    void StopSound();

private:
    AudioManager() = default;
};

#endif // AUDIO_H
