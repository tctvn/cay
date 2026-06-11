package com.example.cayime

import android.app.AlertDialog
import android.content.Context
import android.content.Intent
import android.widget.Toast
import android.inputmethodservice.InputMethodService
import android.media.AudioManager
import android.text.InputType
import android.text.TextUtils
import android.view.HapticFeedbackConstants
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.ExtractedTextRequest
import android.widget.Button
import android.widget.PopupMenu
import android.widget.TextView
import android.util.Log
import kotlin.math.abs

private const val TAG = "CayIME"

enum class ShiftState {
    UNSHIFTED,
    SHIFTED,
    CAPS_LOCK
}

class CayIME : InputMethodService() {

    private lateinit var telexEngine: TelexEngine
    private var isVietnameseMode = true
    private lateinit var spaceKey: TextView
    private var shiftKeyView: TextView? = null
    private val letterKeys = mutableMapOf<Int, TextView>()
    private var currentShiftState = ShiftState.UNSHIFTED
    private var lastSelectionStart = -1
    private var isHandlingKey = false

    private lateinit var audioManager: AudioManager
    private var currentKeyboardLayout = R.layout.keyboard_view

    override fun onCreate() {
        super.onCreate()
        telexEngine = TelexEngine()
        audioManager = getSystemService(AUDIO_SERVICE) as AudioManager
    }

    override fun onStartInput(attribute: EditorInfo?, restarting: Boolean) {
        super.onStartInput(attribute, restarting)
        Log.d(TAG, "onStartInput restarting=$restarting")
        if (!restarting) {
            telexEngine.reset()
            lastSelectionStart = -1
        }
    }

    override fun onStartInputView(info: EditorInfo?, restarting: Boolean) {
        super.onStartInputView(info, restarting)
        
        var layoutResId = R.layout.keyboard_view
        var isWebInput = false
        
        if (info != null) {
            val inputType = info.inputType
            val typeClass = inputType and android.text.InputType.TYPE_MASK_CLASS
            val typeVariation = inputType and android.text.InputType.TYPE_MASK_VARIATION
            
            if (typeClass == android.text.InputType.TYPE_CLASS_NUMBER || typeClass == android.text.InputType.TYPE_CLASS_PHONE || typeClass == android.text.InputType.TYPE_CLASS_DATETIME) {
                layoutResId = R.layout.keyboard_view_numpad
            } else if (typeClass == android.text.InputType.TYPE_CLASS_TEXT) {
                if (typeVariation == android.text.InputType.TYPE_TEXT_VARIATION_URI || 
                    typeVariation == android.text.InputType.TYPE_TEXT_VARIATION_WEB_EDIT_TEXT || 
                    typeVariation == android.text.InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS ||
                    typeVariation == android.text.InputType.TYPE_TEXT_VARIATION_WEB_EMAIL_ADDRESS) {
                    isWebInput = true
                }
            }
        }
        
        currentKeyboardLayout = layoutResId
        val keyboardView = loadKeyboardLayout(layoutResId)
        setInputView(keyboardView)
        
        if (layoutResId == R.layout.keyboard_view) {
            val webDotKey = keyboardView.findViewById<View>(R.id.key_web_dot)
            webDotKey?.visibility = if (isWebInput) View.VISIBLE else View.GONE
        }
        
        updateAutoCaps()
    }

    override fun onFinishInput() {
        super.onFinishInput()
    }

    override fun onUpdateSelection(
        oldSelStart: Int, oldSelEnd: Int, newSelStart: Int, newSelEnd: Int,
        candidatesStart: Int, candidatesEnd: Int
    ) {
        super.onUpdateSelection(oldSelStart, oldSelEnd, newSelStart, newSelEnd, candidatesStart, candidatesEnd)
        
        Log.d(TAG, "onUpdateSelection old=$oldSelStart new=$newSelStart isHandling=$isHandlingKey lastSel=$lastSelectionStart")
        
        // While we are processing a key, ignore all selection updates
        if (isHandlingKey) {
            lastSelectionStart = newSelStart
            return
        }
        
        if (newSelStart != newSelEnd) {
            // Text selection — reset engine
            Log.d(TAG, "  -> RESET (text selection)")
            telexEngine.reset()
            lastSelectionStart = newSelStart
        } else if (lastSelectionStart != -1 && newSelStart != lastSelectionStart) {
            // Manual cursor jump detected
            Log.d(TAG, "  -> RESET (cursor jump: expected=$lastSelectionStart got=$newSelStart)")
            telexEngine.reset()
            lastSelectionStart = newSelStart
        } else {
            lastSelectionStart = newSelStart
        }
        updateAutoCaps()
    }

    private fun updateAutoCaps() {
        if (currentShiftState == ShiftState.CAPS_LOCK) return
        val ic = currentInputConnection ?: return
        
        val capsMode = ic.getCursorCapsMode(TextUtils.CAP_MODE_SENTENCES)
        val newState = if (capsMode != 0) ShiftState.SHIFTED else ShiftState.UNSHIFTED
        
        if (currentShiftState != newState) {
            currentShiftState = newState
            updateShiftUI()
        }
    }

    private fun updateShiftUI() {
        val isUpper = currentShiftState == ShiftState.SHIFTED || currentShiftState == ShiftState.CAPS_LOCK
        
        for ((_, view) in letterKeys) {
            val charStr = view.text.toString()
            if (charStr.isNotEmpty()) {
                val char = charStr[0]
                view.text = if (isUpper) char.uppercaseChar().toString() else char.lowercaseChar().toString()
            }
        }

        shiftKeyView?.let { key ->
            if (currentShiftState == ShiftState.UNSHIFTED) {
                key.setBackgroundResource(R.drawable.key_special_bg_selector)
            } else {
                key.setBackgroundResource(R.drawable.key_bg_selector) // Light up
            }
        }
    }

    override fun onCreateInputView(): View {
        // onCreateInputView is usually called before onStartInputView.
        // We'll initialize with the default layout, and onStartInputView will override if needed.
        return loadKeyboardLayout(currentKeyboardLayout)
    }

    private fun loadKeyboardLayout(layoutResId: Int): View {
        val keyboardView = layoutInflater.inflate(layoutResId, null)
        when (layoutResId) {
            R.layout.keyboard_view -> setupQwertyKeyboard(keyboardView)
            R.layout.keyboard_view_symbols -> setupSymbolsKeyboard(keyboardView)
            R.layout.keyboard_view_emoji -> setupEmojiKeyboard(keyboardView)
            R.layout.keyboard_view_numpad -> setupNumpadKeyboard(keyboardView)
        }
        return keyboardView
    }

    private fun switchKeyboardLayout(layoutResId: Int) {
        currentKeyboardLayout = layoutResId
        setInputView(loadKeyboardLayout(layoutResId))
        if (layoutResId == R.layout.keyboard_view) {
            updateShiftUI()
        }
    }

    private fun setupNumpadKeyboard(keyboardView: View) {
        val keys = listOf(
            R.id.key_1 to '1', R.id.key_2 to '2', R.id.key_3 to '3',
            R.id.key_4 to '4', R.id.key_5 to '5', R.id.key_6 to '6',
            R.id.key_7 to '7', R.id.key_8 to '8', R.id.key_9 to '9',
            R.id.key_0 to '0'
        )

        for ((id, char) in keys) {
            keyboardView.findViewById<View>(id)?.setOnClickListener { view ->
                playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
                telexEngine.reset()
                isHandlingKey = true
                currentInputConnection?.commitText(char.toString(), 1)
                if (lastSelectionStart != -1) {
                    lastSelectionStart += 1
                }
                isHandlingKey = false
            }
        }

        keyboardView.findViewById<View>(R.id.key_delete)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_DELETE)
            isHandlingKey = true
            currentInputConnection?.deleteSurroundingText(1, 0)
            if (lastSelectionStart != -1) {
                lastSelectionStart -= 1
                if (lastSelectionStart < 0) lastSelectionStart = 0
            }
            isHandlingKey = false
        }

        keyboardView.findViewById<View>(R.id.key_abc)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
            switchKeyboardLayout(R.layout.keyboard_view)
        }
    }

    private fun setupQwertyKeyboard(keyboardView: View) {
        val keys = listOf(
            R.id.key_q to 'q', R.id.key_w to 'w', R.id.key_e to 'e', R.id.key_r to 'r', R.id.key_t to 't',
            R.id.key_y to 'y', R.id.key_u to 'u', R.id.key_i to 'i', R.id.key_o to 'o', R.id.key_p to 'p',
            R.id.key_a to 'a', R.id.key_s to 's', R.id.key_d to 'd', R.id.key_f to 'f', R.id.key_g to 'g',
            R.id.key_h to 'h', R.id.key_j to 'j', R.id.key_k to 'k', R.id.key_l to 'l',
            R.id.key_z to 'z', R.id.key_x to 'x', R.id.key_c to 'c', R.id.key_v to 'v', R.id.key_b to 'b',
            R.id.key_n to 'n', R.id.key_m to 'm'
        )

        letterKeys.clear()
        for ((id, char) in keys) {
            val view = keyboardView.findViewById<TextView>(id)
            if (view != null) {
                letterKeys[id] = view
                view.setOnClickListener { v ->
                    playClickFeedback(v, AudioManager.FX_KEYPRESS_STANDARD)
                    handleCharacter(char.uppercaseChar().code, char)
                }
            }
        }
        
        shiftKeyView = keyboardView.findViewById(R.id.key_shift)
        shiftKeyView?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
            currentShiftState = when (currentShiftState) {
                ShiftState.UNSHIFTED -> ShiftState.SHIFTED
                ShiftState.SHIFTED -> ShiftState.UNSHIFTED
                ShiftState.CAPS_LOCK -> ShiftState.UNSHIFTED
            }
            updateShiftUI()
        }

        keyboardView.findViewById<View>(R.id.key_delete)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_DELETE)
            handleCharacter(8, '\b')
        }

        keyboardView.findViewById<View>(R.id.key_enter)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_RETURN)
            currentInputConnection?.sendKeyEvent(KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER))
            currentInputConnection?.sendKeyEvent(KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_ENTER))
            telexEngine.reset()
        }

        spaceKey = keyboardView.findViewById(R.id.key_space)
        setupSpacebarTouchListener()
        keyboardView.findViewById<View>(R.id.key_123)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
            switchKeyboardLayout(R.layout.keyboard_view_symbols)
        }

        keyboardView.findViewById<View>(R.id.key_emoji)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
            switchKeyboardLayout(R.layout.keyboard_view_emoji)
        }

        keyboardView.findViewById<View>(R.id.key_web_dot)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
            handleCharacter('.'.code, '.')
        }

        setupBottomBar(keyboardView)

        updateSpacebarLabel()
        updateShiftUI()
    }

    private fun setupSymbolsKeyboard(keyboardView: View) {
        val keys = listOf(
            R.id.key_1 to '1', R.id.key_2 to '2', R.id.key_3 to '3', R.id.key_4 to '4', R.id.key_5 to '5',
            R.id.key_6 to '6', R.id.key_7 to '7', R.id.key_8 to '8', R.id.key_9 to '9', R.id.key_0 to '0',
            R.id.key_at to '@', R.id.key_hash to '#', R.id.key_dollar to '$',
            R.id.key_ampersand to '&', R.id.key_minus to '-', R.id.key_plus to '+',
            R.id.key_equal to '=', R.id.key_slash to '/',
            R.id.key_quote to '\'', R.id.key_doublequote to '"', R.id.key_colon to ':', R.id.key_semicolon to ';',
            R.id.key_exclamation to '!', R.id.key_question to '?', R.id.key_comma to ',', R.id.key_period to '.',
            R.id.key_lparen to '(', R.id.key_rparen to ')'
        )

        for ((id, char) in keys) {
            keyboardView.findViewById<View>(id)?.setOnClickListener { view ->
                playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
                telexEngine.reset()
                isHandlingKey = true
                currentInputConnection?.commitText(char.toString(), 1)
                if (lastSelectionStart != -1) {
                    lastSelectionStart += 1
                }
                isHandlingKey = false
            }
        }

        keyboardView.findViewById<View>(R.id.key_delete)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_DELETE)
            isHandlingKey = true
            currentInputConnection?.deleteSurroundingText(1, 0)
            if (lastSelectionStart != -1) {
                lastSelectionStart -= 1
                if (lastSelectionStart < 0) lastSelectionStart = 0
            }
            isHandlingKey = false
        }

        keyboardView.findViewById<View>(R.id.key_enter)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_RETURN)
            currentInputConnection?.sendKeyEvent(KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER))
            currentInputConnection?.sendKeyEvent(KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_ENTER))
        }

        keyboardView.findViewById<View>(R.id.key_space)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_SPACEBAR)
            isHandlingKey = true
            currentInputConnection?.commitText(" ", 1)
            if (lastSelectionStart != -1) {
                lastSelectionStart += 1
            }
            isHandlingKey = false
        }

        keyboardView.findViewById<View>(R.id.key_abc)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
            switchKeyboardLayout(R.layout.keyboard_view)
        }

        keyboardView.findViewById<View>(R.id.key_emoji)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
            switchKeyboardLayout(R.layout.keyboard_view_emoji)
        }

        setupBottomBar(keyboardView)
    }

    private fun setupEmojiKeyboard(keyboardView: View) {
        val emojiContainer = keyboardView.findViewById<ViewGroup>(R.id.emoji_container)
        
        for (i in 0 until emojiContainer.childCount) {
            val child = emojiContainer.getChildAt(i)
            if (child is ViewGroup) {
                for (j in 0 until child.childCount) {
                    val btn = child.getChildAt(j)
                    if (btn is TextView && btn.tag != null) {
                        btn.setOnClickListener { view ->
                            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
                            val emojiText = btn.tag.toString()
                            isHandlingKey = true
                            currentInputConnection?.commitText(emojiText, 1)
                            if (lastSelectionStart != -1) {
                                lastSelectionStart += emojiText.length
                            }
                            isHandlingKey = false
                        }
                    }
                }
            }
        }

        keyboardView.findViewById<View>(R.id.key_delete)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_DELETE)
            isHandlingKey = true
            currentInputConnection?.deleteSurroundingText(1, 0)
            if (lastSelectionStart != -1) {
                lastSelectionStart -= 1
                if (lastSelectionStart < 0) lastSelectionStart = 0
            }
            isHandlingKey = false
        }

        keyboardView.findViewById<View>(R.id.key_abc)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
            switchKeyboardLayout(R.layout.keyboard_view)
        }

        setupBottomBar(keyboardView)
    }

    private fun setupBottomBar(keyboardView: View) {
        val globeKey = keyboardView.findViewById<View>(R.id.key_globe)
        globeKey?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
            isVietnameseMode = !isVietnameseMode
            updateSpacebarLabel()
        }
        
        globeKey?.setOnLongClickListener { view ->
            val popup = PopupMenu(this, view)
            popup.menu.add("Cay: VI (Telex)")
            popup.menu.add("Cay: EN (English)")
            popup.menu.add("Cấu hình bàn phím")
            popup.setOnMenuItemClickListener { item ->
                when (item.title.toString()) {
                    "Cay: VI (Telex)" -> {
                        isVietnameseMode = true
                        updateSpacebarLabel()
                    }
                    "Cay: EN (English)" -> {
                        isVietnameseMode = false
                        updateSpacebarLabel()
                    }
                    "Cấu hình bàn phím" -> {
                        val intent = Intent(android.provider.Settings.ACTION_INPUT_METHOD_SETTINGS)
                        intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK
                        startActivity(intent)
                    }
                }
                true
            }
            popup.show()
            true
        }

        keyboardView.findViewById<View>(R.id.key_mic)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
            launchGoogleVoiceTyping()
        }
    }

    private fun setupSpacebarTouchListener() {
        var initialX = 0f
        var isDragging = false
        val dragThreshold = 40f
        val stepThreshold = 30f
        var lastCursorMoveX = 0f

        spaceKey.setOnTouchListener { v, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> {
                    initialX = event.rawX
                    lastCursorMoveX = initialX
                    isDragging = false
                    v.isPressed = true
                    true
                }
                MotionEvent.ACTION_MOVE -> {
                    val deltaX = event.rawX - initialX
                    if (abs(deltaX) > dragThreshold) {
                        if (!isDragging) {
                            isDragging = true
                            telexEngine.reset()
                        }
                        
                        val moveDelta = event.rawX - lastCursorMoveX
                        if (abs(moveDelta) > stepThreshold) {
                            moveCursor(if (moveDelta > 0) 1 else -1)
                            lastCursorMoveX = event.rawX
                        }
                    }
                    true
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                    v.isPressed = false
                    if (!isDragging && event.action == MotionEvent.ACTION_UP) {
                        playClickFeedback(v, AudioManager.FX_KEYPRESS_SPACEBAR)
                        telexEngine.reset()
                        handleCharacter(32, ' ')
                    }
                    true
                }
                else -> false
            }
        }
    }
    
    private fun moveCursor(offset: Int) {
        val ic = currentInputConnection ?: return
        val extText = ic.getExtractedText(ExtractedTextRequest(), 0) ?: return
        val currentStart = extText.selectionStart
        val currentEnd = extText.selectionEnd
        
        if (currentStart == currentEnd) {
            val newPos = (currentStart + offset).coerceIn(0, extText.text.length)
            ic.setSelection(newPos, newPos)
        }
    }

    private fun playClickFeedback(view: View, soundEffect: Int) {
        audioManager.playSoundEffect(soundEffect)
        @Suppress("DEPRECATION")
        view.performHapticFeedback(
            HapticFeedbackConstants.KEYBOARD_TAP,
            HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING
        )
    }

    private fun updateSpacebarLabel() {
        if (::spaceKey.isInitialized) {
            spaceKey.text = if (isVietnameseMode) "Cay: VI" else "Cay: EN"
        }
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        event?.let {
            val unicodeChar = it.unicodeChar.toChar()
            if (unicodeChar.isLetter()) {
                handleCharacter(keyCode, unicodeChar)
                return true
            } else if (keyCode == KeyEvent.KEYCODE_DEL) {
                handleCharacter(8, '\b')
                return true
            } else if (keyCode == KeyEvent.KEYCODE_SPACE) {
                handleCharacter(32, ' ')
                return true
            }
        }
        return super.onKeyDown(keyCode, event)
    }

    fun handleCharacter(keyCode: Int, char: Char) {
        val ic = currentInputConnection ?: return

        var finalChar = char
        if (char.isLetter()) {
            if (currentShiftState == ShiftState.SHIFTED || currentShiftState == ShiftState.CAPS_LOCK) {
                finalChar = char.uppercaseChar()
            }
        }

        // English Mode Check
        if (!isVietnameseMode && finalChar.isLetter()) {
            ic.commitText(finalChar.toString(), 1)
            telexEngine.reset()
            checkShiftRevert(char)
            return
        }

        // Map Key Code for Engine
        val mappedKeyCode = when (finalChar) {
            '\b' -> 1 // Cay::KeyCode::Backspace
            '\n' -> 3 // Cay::KeyCode::Enter
            ' ' -> 5 // Cay::KeyCode::Space
            else -> finalChar.uppercaseChar().code
        }

        Log.d(TAG, "handleChar BEFORE processKey: char='$finalChar' keyCode=$mappedKeyCode")

        // Process Key using Telex Engine
        val result = telexEngine.processKey(mappedKeyCode, finalChar)

        Log.d(TAG, "handleChar AFTER processKey: bs=${result.backspaceCount} newText='${result.newText}' handled=${result.isHandled}")

        isHandlingKey = true
        ic.beginBatchEdit()

        if (result.backspaceCount > 0) {
            ic.deleteSurroundingText(result.backspaceCount, 0)
            if (lastSelectionStart != -1) {
                lastSelectionStart -= result.backspaceCount
                if (lastSelectionStart < 0) lastSelectionStart = 0
            }
        }
        
        if (result.newText.isNotEmpty()) {
            ic.commitText(result.newText, 1)
            if (lastSelectionStart != -1) {
                lastSelectionStart += result.newText.length
            }
        } else if (finalChar == '\b' && result.backspaceCount == 0 && !result.isHandled) {
            ic.deleteSurroundingText(1, 0)
            if (lastSelectionStart != -1) {
                lastSelectionStart -= 1
                if (lastSelectionStart < 0) lastSelectionStart = 0
            }
        } else if (!result.isHandled && finalChar != '\b') {
            Log.d(TAG, "handleChar FALLBACK: committing '$finalChar' directly")
            ic.commitText(finalChar.toString(), 1)
            if (lastSelectionStart != -1) {
                lastSelectionStart += 1
            }
        }
        
        ic.endBatchEdit()
        isHandlingKey = false

        checkShiftRevert(char)
    }

    private fun checkShiftRevert(char: Char) {
        // Revert shift state after a letter is typed, unless caps lock is on
        if (char.isLetter() && currentShiftState == ShiftState.SHIFTED) {
            currentShiftState = ShiftState.UNSHIFTED
            updateShiftUI()
        }
    }

    private fun launchGoogleVoiceTyping() {
        try {
            val imm = getSystemService(INPUT_METHOD_SERVICE) as android.view.inputmethod.InputMethodManager
            val token = window.window?.attributes?.token
            
            var targetImiId: String? = null
            
            // Prefer Google Voice Typing or Speech Services by Google to avoid Samsung Keyboard hijack
            for (imi in imm.enabledInputMethodList) {
                if (imi.id.contains("googlequicksearchbox", ignoreCase = true) ||
                    imi.id.contains("com.google.android.tts", ignoreCase = true) ||
                    imi.id.contains("voice", ignoreCase = true)) {
                    targetImiId = imi.id
                    break
                }
            }
            
            if (targetImiId != null) {
                imm.setInputMethod(token, targetImiId)
                return
            } else {
                val intent = Intent(this, SettingsActivity::class.java)
                intent.putExtra("show_voice_dialog", true)
                intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK
                startActivity(intent)
            }
            
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
}
