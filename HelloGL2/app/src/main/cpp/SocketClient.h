//
// Created by betairya on 18/02/07.
//

#ifndef HELLOGL2_SOCKETCLIENT_H
#define HELLOGL2_SOCKETCLIENT_H

#include <stdlib.h>
#include <stdio.h>
#include <jni.h>
#include <assert.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <jni.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <android/log.h>

#include "SocketThread.h"

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

class SocketClient
{
public:
    void InitSocket(JNIEnv * env, jobject obj, jstring ipaddr, jint port);
    void CloseSocket(JNIEnv * env, jobject obj);
    void SendData(JNIEnv * env, jobject obj, jbyteArray buffer);

    static char* jstringTostring(JNIEnv* env, jstring jstr);

    char** mp_pixelBuffer;

private:
    int socketFd=0;
    jobject jObj;
    JNIEnv *jEnv;
    SocketThread *mSTh;
};

#endif //HELLOGL2_SOCKETCLIENT_H
