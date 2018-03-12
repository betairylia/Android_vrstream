/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// OpenGL ES 2.0 code

/*
 * TODO:
 * Sync
 * Multithread socket streaming
 */

#include "SocketClient.h"
#include "SocketThread.h"

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

SocketClient socketClient;

static char* pixelBuffer = NULL;
static GLint texPixels;
static GLuint texId;

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

auto gVertexShader =
    "attribute vec4 vPosition;\n"
    "varying vec2 uv;"
    "void main() {\n"
    "uv = vec2(1.0 - 0.5 * (vPosition.y + 1.0), 0.5 * (vPosition.x + 1.0));"
    "  gl_Position = vPosition;\n"
    "}\n";

auto gFragmentShader =
    "precision mediump float;\n"
    "varying vec2 uv;"
    "uniform sampler2D texture;"
    "void main() {\n"
    "  gl_FragColor = texture2D(texture, uv);\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLuint gvPositionHandle;
int surfaceWidth, surfaceHeight;

bool setupGraphics(int w, int h)
{
    while(!pixelBuffer){} //wait for a header

    surfaceWidth = w;
    surfaceHeight = h;

    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    gvPositionHandle = (GLuint) glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
            gvPositionHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");

    glGenTextures(1, &texId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 SocketThread::header.format,
                 SocketThread::header.width, SocketThread::header.height,
                 0, SocketThread::header.format, SocketThread::header.dataType, 0);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glUseProgram(gProgram);
    texPixels = glGetUniformLocation(gProgram, "texture");
    glUniform1i(texPixels, 0);

    return true;
}

const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f,
        0.5f, -0.5f };

const GLfloat g_quad_vertex_buffer_data[] =
{
        -1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f,
        1.0f,  1.0f, 0.0f, 1.0f,
};

void renderFrame()
{
//    static int frame_count = 0;
//    static clock_t last_time = clock();
//    static int last_frame_count = 0;
//
//    frame_count++;
//    if (clock()-last_time > 1e7 && frame_count % 20 == 0) {
//        __android_log_print(ANDROID_LOG_INFO, "libgl2jni", "fps: %f", ((float)frame_count-last_frame_count)/(clock()-last_time)*1e6);
//        last_time = clock();
//        last_frame_count = frame_count;
//    }

    glBindTexture(GL_TEXTURE_2D, texId);
    if(pixelBuffer)
    {
//        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surfaceWidth, surfaceHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        SocketThread::header.width, SocketThread::header.height,
                        SocketThread::header.format, SocketThread::header.dataType, pixelBuffer);
    }

    static float grey;
    grey += 0.01f;
    if (grey > 1.0f) {
        grey = 0.0f;
    }
    glClearColor(grey, grey, grey, 1.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 4, GL_FLOAT, GL_FALSE, 0, g_quad_vertex_buffer_data);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");
    glDrawArrays(GL_TRIANGLES, 0, 6);
    checkGlError("glDrawArrays");
}

extern "C" {
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_testPrint(JNIEnv * env, jobject obj);

    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_InitSocket(JNIEnv * env, jobject obj, jstring ipaddr, jint port);
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_CloseSocket(JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_SendData(JNIEnv * env, jobject obj, jbyteArray buffer);
};

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jobject obj)
{
    renderFrame();
}

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_testPrint(JNIEnv * env, jobject obj)
{
    LOGI("Hi");
}

JNIEXPORT void Java_com_android_gl2jni_GL2JNILib_InitSocket(JNIEnv *env, jobject obj, jstring ipaddr, jint port)
{
    socketClient.mp_pixelBuffer = &pixelBuffer;
    socketClient.InitSocket(env, obj, ipaddr, port);
}

JNIEXPORT void Java_com_android_gl2jni_GL2JNILib_CloseSocket(JNIEnv *env, jobject obj)
{
    socketClient.CloseSocket(env, obj);
}

JNIEXPORT void Java_com_android_gl2jni_GL2JNILib_SendData(JNIEnv *env, jobject obj, jbyteArray buffer)
{
    socketClient.SendData(env, obj, buffer);
}
