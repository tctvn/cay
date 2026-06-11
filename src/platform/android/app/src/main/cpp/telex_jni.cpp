#include <jni.h>
#include <string>
#include "CayEngine.h"
#include "CayTypes.h"

// Thread-local storage to capture callback result during processKeyNative
struct JniContext {
    int backspaceCount;
    std::wstring newText;
};

static thread_local JniContext g_jniContext;

void JNI_OnInjectText(int backspaceCount, const wchar_t* newText, int newTextLen) {
    g_jniContext.backspaceCount = backspaceCount;
    g_jniContext.newText = std::wstring(newText, newTextLen);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_cayime_TelexEngine_initNative(JNIEnv *env, jobject thiz) {
    auto* engine = new Cay::TelexEngine();
    engine->OnInjectText = JNI_OnInjectText;
    return reinterpret_cast<jlong>(engine);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_cayime_TelexEngine_destroyNative(JNIEnv *env, jobject thiz, jlong ptr) {
    auto* engine = reinterpret_cast<Cay::TelexEngine*>(ptr);
    delete engine;
}

extern "C" JNIEXPORT jobject JNICALL
Java_com_example_cayime_TelexEngine_processKeyNative(JNIEnv *env, jobject thiz, jlong ptr,
                                                     jint key_code, jchar character) {
    auto* engine = reinterpret_cast<Cay::TelexEngine*>(ptr);
    
    // Reset context before processing
    g_jniContext.backspaceCount = 0;
    g_jniContext.newText = L"";

    Cay::KeyEvent event;
    event.keyCode = static_cast<Cay::KeyCode>(key_code);
    event.character = static_cast<wchar_t>(character);
    event.handled = false;

    // The engine's OnKeyDown will synchronously call JNI_OnInjectText if needed
    engine->OnKeyDown(event);

    // Convert wstring to jstring
    // Note: Android's jchar is 16-bit (UTF-16), so we must be careful if wchar_t is 32-bit (Linux/Android)
    // Here we assume basic BMP characters or proper conversion.
    // For proper conversion from wchar_t to UTF-16 jchar string:
    std::wstring wstr = g_jniContext.newText;
    std::basic_string<jchar> utf16str;
    for (wchar_t wc : wstr) {
        // Simple cast for basic multilingual plane. If we have surrogates, this needs more logic.
        utf16str.push_back(static_cast<jchar>(wc));
    }
    
    jstring jNewText = env->NewString(utf16str.c_str(), utf16str.length());

    // Create KeyProcessResult object
    jclass resultClass = env->FindClass("com/example/cayime/KeyProcessResult");
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(ZILjava/lang/String;)V");
    jobject resultObj = env->NewObject(resultClass, constructor, 
                                       event.handled ? JNI_TRUE : JNI_FALSE, 
                                       g_jniContext.backspaceCount, 
                                       jNewText);
    
    env->DeleteLocalRef(resultClass);
    return resultObj;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_cayime_TelexEngine_resetNative(JNIEnv *env, jobject thiz, jlong ptr) {
    auto* engine = reinterpret_cast<Cay::TelexEngine*>(ptr);
    engine->ResetFull();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_cayime_TelexEngine_getComposingTextNative(JNIEnv *env, jobject thiz, jlong ptr) {
    // The C++ engine doesn't currently have a public 'GetComposingText' method that returns a string directly
    // based on the header. However, we might just return an empty string or whatever state is needed.
    // For now, return empty as the composing text state is mostly tracked by Android's InputConnection.
    return env->NewStringUTF("");
}
