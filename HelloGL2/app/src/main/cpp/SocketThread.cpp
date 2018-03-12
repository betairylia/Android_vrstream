//
// Created by betairya on 18/02/07.
//

#include <sys/socket.h>
#include <malloc.h>
#include <errno.h>
#include "SocketThread.h"

pthread_cond_t  SocketThread::cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t SocketThread::mutex = PTHREAD_MUTEX_INITIALIZER;
int             SocketThread::socketFd = 0;
char *          SocketThread::getBuffer = NULL;
bool            SocketThread::isShouldExit = false;
JNIEnv*         SocketThread::env=NULL;
jobject         SocketThread::obj=NULL;
JavaVM*         SocketThread::javavm = NULL;

char **         SocketThread::mp_pixelBuffer = NULL;
int             SocketThread::bufLen = 16384;
Header          SocketThread::header;

void *SocketThread::clientThread(void *args)
{
    javavm->AttachCurrentThread(&SocketThread::env,NULL);

    LOGI("SocketThread: SocketDataDealThread is running");
    while (!SocketThread::isShouldExit)
    {
//        pthread_mutex_lock(&SocketThread::mutex);
//        pthread_cond_wait(&SocketThread::cond,&SocketThread::mutex);
//        pthread_mutex_unlock(&SocketThread::mutex);
//        LOGI("SocketThread: clientThread wake");

        int len;
        len = (int) recv(socketFd, &header, sizeof(Header), 0);

        if(len < 0)
        {
            LOGI("SocketThread: Get header error - %d", errno);
        }

//        LOGI("SocketThread: Get header for frame #%d, data lenth = %d", header.frameId, header.lenth);

        if((*mp_pixelBuffer) == NULL)
        {
            (*mp_pixelBuffer) = (char *)malloc(sizeof(char) * header.lenth);
        }

        for(len = 0; len < header.lenth; )
        {
            len += (int) recv(socketFd, (*mp_pixelBuffer) + len, header.lenth - len, 0);

            if(len < 0)
            {
                LOGI("SocketThread: Get data error - %d", errno);
            }
        }

        char tmp = 'k';
        send(socketFd, &tmp, sizeof(char), 0);

//        if(SocketThread::env!=NULL && SocketThread::obj)
//        {
//            jclass cls = SocketThread::env->GetObjectClass(SocketThread::obj);
//            jmethodID mid = SocketThread::env->GetMethodID(cls,"setReceiveData","([B)V");
//            SocketThread::env->DeleteLocalRef(cls);
//            if(mid==NULL)
//            {
//                LOGI("SocketThread: find method1 error");
//                pthread_exit(NULL);
//            }
//            jbyteArray jArray = SocketThread::env->NewByteArray(len);
//            SocketThread::env->SetByteArrayRegion(jArray,0,len,(jbyte*)getBuffer);
//            SocketThread::env->CallVoidMethod(obj,mid,jArray);
//            SocketThread::env->DeleteLocalRef(jArray);
//        }
    }
    LOGI("SocketThread: SocketDataDealThread exit");
    return NULL;
}

SocketThread::SocketThread(int fd, jobject obj) : threadId(0), sendLength(0)
{
    if(pthread_create(&threadId,NULL,SocketThread::clientThread,NULL) != 0 ){
        LOGI("SocketThread: pthread_create error");
    }
    LOGI("SocketThread: getSocketThreadId():%lu",(long)threadId);
    SocketThread::obj = obj;
    getBuffer = new char[100];
    sendBuffer = new char[100];
    socketFd=fd;
}

SocketThread::~SocketThread()
{
    delete getBuffer;
    delete sendBuffer;
}

void SocketThread::sendData(char *buff, int length)
{
    LOGI("SocketThread: send data %s,len = %d",buff,length);
    int len = (int) send(socketFd, buff, (size_t) length, 0);

    if(len<0)
    {
        LOGI("SocketThread: send data error,len = %d",len);
    }

    wakeUpThread();
}

void SocketThread::setIsShouldExit(bool isShould)
{
    isShouldExit = isShould;
}

pthread_t SocketThread::getSocketThreadId()
{
    return threadId;
}

void SocketThread::wakeUpThread()
{
    pthread_mutex_lock(&SocketThread::mutex);
    pthread_cond_signal(&SocketThread::cond);
    pthread_mutex_unlock(&SocketThread::mutex);
}
