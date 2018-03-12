//
// Created by betairya on 18/02/07.
//

#include <errno.h>
#include "SocketClient.h"

void SocketClient::InitSocket(JNIEnv *env, jobject obj, jstring ipaddr, jint port)
{
    SocketThread::mp_pixelBuffer = mp_pixelBuffer;

    JavaVM* vm;
    jsize vmCount;
    if (env->GetJavaVM(&vm) != JNI_OK)
    {
        fprintf(stderr, "Could not get active VM\n");
        return;
    }

    SocketThread::javavm = vm;

    LOGI("socketClient: initSocket()");
    char *lip = jstringTostring(env,ipaddr);
    socketFd = socket(AF_INET, SOCK_STREAM, 0);

//    struct timeval timeout = {1,0};
//    setsockopt(socketFd,SOL_SOCKET,SO_SNDTIMEO,(char *)&timeout,sizeof(struct timeval));
//    setsockopt(socketFd,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));
//
//    int nRecvBuf = 32*1024;
//    setsockopt(socketFd,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
//    int nSendBuf = 32*1024;
//    setsockopt(socketFd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
//    int nZero=0;
//    setsockopt (socketFd,SOL_SOCKET,SO_SNDBUF,(const char *)&nZero,sizeof(nZero));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, lip, &servaddr.sin_addr);
    servaddr.sin_port = htons((u_short)port);

    if(connect(socketFd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        LOGI("connect failed with error No: %d", errno);
    }

    jObj = env->NewGlobalRef(obj);
    mSTh = new SocketThread(socketFd,jObj);
    LOGI("socketClient: initSocket finished,ip = %s,port = %d",lip,port);
}

char *SocketClient::jstringTostring(JNIEnv *env, jstring jstr) {
    char* rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("utf-8");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr= (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0)
    {
        rtn = (char*)malloc((size_t)alen + 1);

        memcpy(rtn, ba, (size_t)alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

void SocketClient::CloseSocket(JNIEnv *env, jobject obj)
{
    void *status;
    LOGI("SocketClient: closeSocket");
    mSTh->setIsShouldExit(true);
    mSTh->wakeUpThread();
    pthread_join(mSTh->getSocketThreadId(), &status);
    delete mSTh;
    env->DeleteGlobalRef(jObj);
    close(socketFd);
}

void SocketClient::SendData(JNIEnv *env, jobject obj, jbyteArray buffer)
{
    jbyte *lb = env->GetByteArrayElements(buffer, NULL);
    int length = env->GetArrayLength(buffer);
    LOGI("SocketClient: sendData,%s,length : %d",(char*)lb,length);
    mSTh->sendData((char *)lb,length);
    env->ReleaseByteArrayElements(buffer,lb,0);
}
