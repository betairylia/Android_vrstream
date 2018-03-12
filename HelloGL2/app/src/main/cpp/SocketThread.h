//
// Created by betairya on 18/02/07.
//

#ifndef HELLOGL2_SOCKETTHREAD_H
#define HELLOGL2_SOCKETTHREAD_H

#include <pthread.h>
#include <jni.h>
#include <android/log.h>

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

typedef struct
{
    int lenth;

    int frameId;
    int width;
    int height;
    int format;
    int dataType;

    float state;
}Header;

class SocketThread
{
public:
    static void * clientThread(void *args);
    static pthread_cond_t cond;
    static JavaVM*  javavm;
    static pthread_mutex_t mutex;
    static bool isShouldExit;
    static JNIEnv* env;
    static jobject obj;

    SocketThread(int fd,jobject obj);
    ~SocketThread();
    void sendData(char buff[],int length);
    void setIsShouldExit(bool isShould);
    pthread_t getSocketThreadId();
    void wakeUpThread();

    static Header header;
    static char** mp_pixelBuffer;
    static int bufLen;

private:
    pthread_t threadId;
    static int socketFd;
    static char * getBuffer;
    char * sendBuffer;
    int sendLength=0;
};


#endif //HELLOGL2_SOCKETTHREAD_H
