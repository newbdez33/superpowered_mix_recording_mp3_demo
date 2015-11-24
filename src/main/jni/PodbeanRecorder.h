#ifndef Header_PodbeanRecorder
#define Header_PodbeanRecorder

#include <math.h>
#include <pthread.h>

#include "Superpowered/SuperpoweredAdvancedAudioPlayer.h"
#include "Superpowered/SuperpoweredAndroidAudioIO.h"

#define NUM_BUFFERS 2
#define HEADROOM_DECIBEL 3.0f
static const float headroom = powf(10.0f, -HEADROOM_DECIBEL * 0.025);

const int MP3_SIZE = 8192;

class PodbeanRecorder {
public:

	PodbeanRecorder(const char *path, int *params, const char * recordfile);
	~PodbeanRecorder();

	bool process(short int *output, unsigned int numberOfSamples);
    void onPlayBackgroundMusic(bool play);
    void onMusicSelect(const char *path, int *params);
    void onVolume(int vol);

private:
    pthread_mutex_t mutex;
    SuperpoweredAndroidAudioIO *audioSystem;
    SuperpoweredAdvancedAudioPlayer *playerA, *playerB;
    float *stereoBuffer;
    float *recorderBuffer;  //J
    short int *mp3Output;
    //short int *mixOutput;

    float volume;

    //added object variables
    const char *tempPath;
    const char *destinationPath;

};

#endif
