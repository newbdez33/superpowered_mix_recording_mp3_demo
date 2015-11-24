#include "PodbeanRecorder.h"
#include "Superpowered/SuperpoweredSimple.h"
#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
#include <android/log.h>
#include "libmp3lame/lame.h"

static lame_global_flags *lame = NULL;
static FILE *mp3 = NULL;
static unsigned char mp3Buffer[MP3_SIZE];
static bool isRecording;
static unsigned int samplerate;
static char mp3FilePath[1024];

static void playerEventCallbackA(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
    if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
    	SuperpoweredAdvancedAudioPlayer *playerA = *((SuperpoweredAdvancedAudioPlayer **)clientData);
        playerA->setBpm(0);
        playerA->setFirstBeatMs(0);
        playerA->setPosition(playerA->firstBeatMs, false, false);
    };
}

static void playerEventCallbackB(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
     if (event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
     	SuperpoweredAdvancedAudioPlayer *playerB = *((SuperpoweredAdvancedAudioPlayer **)clientData);
         playerB->setBpm(0);
         playerB->setFirstBeatMs(0);
         playerB->setPosition(playerB->firstBeatMs, false, false);
     };
}

static bool audioProcessing(void *clientdata, short int *audioIO, int numberOfSamples, int samplerate) {
	return ((PodbeanRecorder *)clientdata)->process(audioIO, numberOfSamples);
}

PodbeanRecorder::PodbeanRecorder(const char *path, int *params, const char * recordfile) {

    pthread_mutex_init(&mutex, NULL); // This will keep our player volumes and playback states in sync.
    samplerate = params[4];
    unsigned int buffersize = params[5];
    stereoBuffer = (float *)memalign(16, (buffersize + 16) * sizeof(float) * 2);
    recorderBuffer = (float *) memalign(16, (buffersize + 16) * sizeof(float) * 2); //J
    mp3Output = (short int *) memalign(16, (buffersize + 16) * sizeof(short int) * 2); //J
    //mixOutput = (short int *) memalign(16, (buffersize + 16) * sizeof(short int) * 2); //J

    strcpy(mp3FilePath, recordfile);
    __android_log_print(ANDROID_LOG_ERROR, "RecordUtil", "recording path: %s", mp3FilePath);

    playerA = new SuperpoweredAdvancedAudioPlayer(&playerA , playerEventCallbackA, samplerate, 0);
    playerA->open(path, params[0], params[1]);

    playerB = new SuperpoweredAdvancedAudioPlayer(&playerB, playerEventCallbackB, samplerate, 0);
    playerB->open(path, params[0], params[1]);

    playerA->syncMode = playerB->syncMode = SuperpoweredAdvancedAudioPlayerSyncMode_TempoAndBeat;

    audioSystem = new SuperpoweredAndroidAudioIO(samplerate, buffersize, true, true, audioProcessing, this, buffersize * 2);

    isRecording = false;
    volume = 0.5;

    //this->onPlayBackgroundMusic(true);
}

PodbeanRecorder::~PodbeanRecorder() {
    delete playerA;
    delete playerB;
    delete audioSystem;
    free(stereoBuffer);
    free(recorderBuffer);   //J
    free(mp3Output);
    pthread_mutex_destroy(&mutex);
    __android_log_print(ANDROID_LOG_ERROR, "TEST", "Offffffffffffliiineeee!");
}

void PodbeanRecorder::onPlayBackgroundMusic(bool play) {
    pthread_mutex_lock(&mutex);
    if (!play) {
        playerA->pause();
        playerB->pause();
    } else {
        playerA->play(true);
        playerB->play(true);
    };
    pthread_mutex_unlock(&mutex);
}

void PodbeanRecorder::onMusicSelect(const char *path, int *params) {
    playerA->pause();
    playerB->pause();
    delete playerA;
    delete playerB;

    playerA = new SuperpoweredAdvancedAudioPlayer(&playerA , playerEventCallbackA, samplerate, 0);
    playerA->open(path, params[0], params[1]);

    playerB = new SuperpoweredAdvancedAudioPlayer(&playerB, playerEventCallbackB, samplerate, 0);
    playerB->open(path, params[0], params[1]);

    playerA->syncMode = playerB->syncMode = SuperpoweredAdvancedAudioPlayerSyncMode_TempoAndBeat;

    this->onPlayBackgroundMusic(true);
	__android_log_print(ANDROID_LOG_ERROR, "RecordUtil", "background music %s", path);
}

void PodbeanRecorder::onVolume(int vol) {
    float v = (float)vol;
    volume = v/100;
    __android_log_print(ANDROID_LOG_ERROR, "TEST", "volume music %f", volume);
}

bool PodbeanRecorder::process(short int *output, unsigned int numberOfSamples) {

    pthread_mutex_lock(&mutex);

    bool silence = !playerB->process(recorderBuffer, false, numberOfSamples, volume);

    if (!silence) {

        //memset(mixOutput, 0, sizeof(mixOutput));
        SuperpoweredShortIntToFloat(output, stereoBuffer, numberOfSamples);
        silence = !playerA->process(stereoBuffer, true, numberOfSamples, volume);

        if(isRecording && mp3 != NULL && lame != NULL) {
            memset(mp3Output, 0, sizeof(mp3Output));
            SuperpoweredFloatToShortInt(stereoBuffer, mp3Output, numberOfSamples);
            /* LAME */
            int result = lame_encode_buffer_interleaved(lame, mp3Output, numberOfSamples, mp3Buffer, MP3_SIZE);
            if (result > 0) {
                __android_log_print(ANDROID_LOG_ERROR, "TEST", "lame result = %d", result);
                fwrite(mp3Buffer, result, 1, mp3);
            }
            /* LAME */
        }else {
            __android_log_print(ANDROID_LOG_ERROR, "TEST", "recording == false");
        }

        //OUTPUT
        SuperpoweredFloatToShortInt(recorderBuffer, output, numberOfSamples);
    }
    pthread_mutex_unlock(&mutex);

    return !silence;
}

extern "C" {
	JNIEXPORT void Java_com_podbean_app_podcast_utils_RecorderUtil_PodbeanRecorder(JNIEnv *javaEnvironment, jobject self, jstring apkPath, jlongArray offsetAndLength, jstring recordfile);
	JNIEXPORT void Java_com_podbean_app_podcast_utils_RecorderUtil_onPlayBackgroundMusic(JNIEnv *javaEnvironment, jobject self, jboolean play);
    JNIEXPORT void Java_com_podbean_app_podcast_utils_RecorderUtil_onRecord(JNIEnv *javaEnvironment, jobject self, jboolean record);
    JNIEXPORT void Java_com_podbean_app_podcast_utils_RecorderUtil_onVolume(JNIEnv *javaEnvironment, jobject self, jint vol);
	JNIEXPORT void Java_com_podbean_app_podcast_utils_RecorderUtil_onMusicSelect(JNIEnv *javaEnvironment, jobject self, jstring apkPath, jlongArray offsetAndLength);
    JNIEXPORT jstring JNICALL Java_com_podbean_app_podcast_utils_RecorderUtil_getCLanguageString(JNIEnv *javaEnvironment, jobject self);
    JNIEXPORT void Java_com_podbean_app_podcast_utils_RecorderUtil_offline(JNIEnv *javaEnvironment, jobject self);
}

static PodbeanRecorder *util = NULL;

// Android is not passing more than 2 custom parameters, so we had to pack file offsets and lengths into an array.
JNIEXPORT void Java_com_podbean_app_podcast_utils_RecorderUtil_PodbeanRecorder(JNIEnv *javaEnvironment, jobject self, jstring apkPath, jlongArray params, jstring recordfile) {
    // Convert the input jlong array to a regular int array.
    jlong *longParams = javaEnvironment->GetLongArrayElements(params, JNI_FALSE);
    int arr[6];
    for (int n = 0; n < 6; n++) arr[n] = longParams[n];
    javaEnvironment->ReleaseLongArrayElements(params, longParams, JNI_ABORT);

    const char *path = javaEnvironment->GetStringUTFChars(apkPath, JNI_FALSE);
    const char *rp = javaEnvironment->GetStringUTFChars(recordfile, JNI_FALSE);
    util = new PodbeanRecorder(path, arr, rp);
    javaEnvironment->ReleaseStringUTFChars(apkPath, path);
    javaEnvironment->ReleaseStringUTFChars(recordfile, rp);
}

JNIEXPORT void Java_com_podbean_app_podcast_utils_RecorderUtil_onVolume(JNIEnv *javaEnvironment, jobject self, jint vol) {
    util->onVolume(vol);
}

JNIEXPORT void Java_com_podbean_app_podcast_utils_RecorderUtil_onRecord(JNIEnv *javaEnvironment, jobject self, jboolean record) {
    if (record) {
        //LAME
        if (lame != NULL) {
            lame_close(lame);
            lame = NULL;
        }
        lame = lame_init();
        lame_set_in_samplerate(lame, samplerate);
        lame_set_num_channels(lame, 2);//输入流的声道
        lame_set_out_samplerate(lame, 44100);
        //lame_set_brate(lame, buffersize);
        lame_set_quality(lame, 7);
        lame_init_params(lame);

        util->onPlayBackgroundMusic(true);
        __android_log_print(ANDROID_LOG_ERROR, "TEST", "RECORD STARTED!");
        mp3 = fopen(mp3FilePath, "ab+");
        isRecording = true;

    }else {

        util->onPlayBackgroundMusic(false);
        isRecording = false;
        if(lame == NULL || mp3 == NULL) {
            __android_log_print(ANDROID_LOG_ERROR, "TEST", "lame == NULL || mp3 == NULL!");
            return;
        }
        __android_log_print(ANDROID_LOG_ERROR, "TEST", "STOP: %s", mp3FilePath);
        int result = lame_encode_flush(lame,mp3Buffer, MP3_SIZE);
        if(result > 0) {
            __android_log_print(ANDROID_LOG_ERROR, "TEST", "STOP with %d", result);
            fwrite(mp3Buffer, result, 1, mp3);
        }
        lame_close(lame);
        fclose(mp3);
        delete mp3;
        mp3 = NULL;
    }
    __android_log_print(ANDROID_LOG_ERROR, "TEST", "Recording STOPPED!");

}

JNIEXPORT void Java_com_podbean_app_podcast_utils_RecorderUtil_onPlayBackgroundMusic(JNIEnv *javaEnvironment, jobject self, jboolean play) {
    util->onPlayBackgroundMusic(play);
}

JNIEXPORT void Java_com_podbean_app_podcast_utils_RecorderUtil_offline(JNIEnv *javaEnvironment, jobject self) {

    isRecording = false;

    if(lame) {
        lame_close(lame);
    }

    if (mp3) {
        fclose(mp3);
        mp3 = NULL;
    }

    if (util) {
        delete util;
        util = NULL;
    }

}

JNIEXPORT void Java_com_podbean_app_podcast_utils_RecorderUtil_onMusicSelect(JNIEnv *javaEnvironment, jobject self, jstring apkPath, jlongArray params) {

    jlong *longParams = javaEnvironment->GetLongArrayElements(params, JNI_FALSE);
    int arr[6];
    for (int n = 0; n < 6; n++) arr[n] = longParams[n];
    javaEnvironment->ReleaseLongArrayElements(params, longParams, JNI_ABORT);

    const char *path = javaEnvironment->GetStringUTFChars(apkPath, JNI_FALSE);
    util->onMusicSelect(path, arr);
    javaEnvironment->ReleaseStringUTFChars(apkPath, path);


}

JNIEXPORT jstring JNICALL Java_com_podbean_app_podcast_utils_RecorderUtil_getCLanguageString (JNIEnv *javaEnvironment, jobject self){
    return javaEnvironment->NewStringUTF("This just a test for Android Studio NDK JNI developer!");
}
