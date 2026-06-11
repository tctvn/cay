package com.example.cayime

data class KeyProcessResult(
    val isHandled: Boolean,
    val backspaceCount: Int,
    val newText: String
)

class TelexEngine {
    companion object {
        init {
            System.loadLibrary("cayime")
        }
    }

    private var nativeEnginePtr: Long = 0

    init {
        nativeEnginePtr = initNative()
    }

    fun processKey(keyCode: Int, character: Char): KeyProcessResult {
        return processKeyNative(nativeEnginePtr, keyCode, character)
    }

    fun reset() {
        resetNative(nativeEnginePtr)
    }
    
    fun getComposingText(): String {
        return getComposingTextNative(nativeEnginePtr)
    }

    protected fun finalize() {
        destroyNative(nativeEnginePtr)
    }

    private external fun initNative(): Long
    private external fun destroyNative(ptr: Long)
    private external fun processKeyNative(ptr: Long, keyCode: Int, character: Char): KeyProcessResult
    private external fun resetNative(ptr: Long)
    private external fun getComposingTextNative(ptr: Long): String
}
