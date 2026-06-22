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
import android.widget.PopupWindow
import android.view.Gravity
import android.widget.LinearLayout
import android.graphics.Color
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
    private var shiftKeyView: View? = null
    private val letterKeys = mutableMapOf<Int, TextView>()
    private var currentShiftState = ShiftState.UNSHIFTED
    private var lastSelectionStart = -1
    private var isHandlingKey = false
    private var lastShiftTime = 0L

    private var deleteHandler: android.os.Handler? = null
    private var deleteRunnable: Runnable? = null

    private lateinit var audioManager: AudioManager
    private var isHapticEnabled = true
    private var currentKeyboardLayout = R.layout.keyboard_view
    private var popupWindow: PopupWindow? = null
    
    private var previewPopupWindow: PopupWindow? = null
    private var previewTextView: TextView? = null
    private var activePreviewKey: View? = null

    private val longPressVariants = mapOf(
        'a' to listOf('a', 'á', 'à', 'ả', 'ã', 'ạ', 'ă', 'ắ', 'ằ', 'ẳ', 'ẵ', 'ặ', 'â', 'ấ', 'ầ', 'ẩ', 'ẫ', 'ậ'),
        'e' to listOf('e', 'é', 'è', 'ẻ', 'ẽ', 'ẹ', 'ê', 'ế', 'ề', 'ể', 'ễ', 'ệ'),
        'i' to listOf('i', 'í', 'ì', 'ỉ', 'ĩ', 'ị'),
        'o' to listOf('o', 'ó', 'ò', 'ỏ', 'õ', 'ọ', 'ô', 'ố', 'ồ', 'ổ', 'ỗ', 'ộ', 'ơ', 'ớ', 'ờ', 'ở', 'ỡ', 'ợ'),
        'u' to listOf('u', 'ú', 'ù', 'ủ', 'ũ', 'ụ', 'ư', 'ứ', 'ừ', 'ử', 'ữ', 'ự'),
        'y' to listOf('y', 'ý', 'ỳ', 'ỷ', 'ỹ', 'ỵ'),
        'd' to listOf('d', 'đ')
    )

    private var isSymbolsPage2 = false

    private fun updateSymbolsPage(keyboardView: View) {
        val row1Keys = listOf(R.id.key_1, R.id.key_2, R.id.key_3, R.id.key_4, R.id.key_5, R.id.key_6, R.id.key_7, R.id.key_8, R.id.key_9, R.id.key_0)
        val row2Keys = listOf(R.id.key_minus, R.id.key_slash, R.id.key_colon, R.id.key_semicolon, R.id.key_lparen, R.id.key_rparen, R.id.key_dollar, R.id.key_ampersand, R.id.key_at, R.id.key_doublequote)
        
        val page1Row1 = listOf("1", "2", "3", "4", "5", "6", "7", "8", "9", "0")
        val page1Row2 = listOf("-", "/", ":", ";", "(", ")", "$", "&", "@", "\"")
        
        val page2Row1 = listOf("[", "]", "{", "}", "#", "%", "^", "*", "+", "=")
        val page2Row2 = listOf("_", "\\", "|", "~", "<", ">", "€", "£", "¥", "•")
        
        val r1 = if (isSymbolsPage2) page2Row1 else page1Row1
        val r2 = if (isSymbolsPage2) page2Row2 else page1Row2
        
        for (i in 0 until 10) {
            keyboardView.findViewById<TextView>(row1Keys[i])?.text = r1[i]
            keyboardView.findViewById<TextView>(row2Keys[i])?.text = r2[i]
        }
        
        keyboardView.findViewById<TextView>(R.id.key_hash)?.text = if (isSymbolsPage2) "123" else "#+="
    }

    private fun updateContextualKeys(keyboardView: View) {
        val info = currentInputEditorInfo ?: return
        
        val typeVariation = info.inputType and android.text.InputType.TYPE_MASK_VARIATION
        val isWebInput = typeVariation == android.text.InputType.TYPE_TEXT_VARIATION_URI ||
                         typeVariation == android.text.InputType.TYPE_TEXT_VARIATION_WEB_EDIT_TEXT ||
                         typeVariation == android.text.InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS
                         
        val webDotKey = keyboardView.findViewById<View>(R.id.key_web_dot)
        webDotKey?.visibility = if (isWebInput) View.VISIBLE else View.GONE
        
        val enterKey = keyboardView.findViewById<TextView>(R.id.key_enter)
        if (enterKey != null) {
            val action = info.imeOptions and EditorInfo.IME_MASK_ACTION
            when (action) {
                EditorInfo.IME_ACTION_GO -> enterKey.text = "Đi"
                EditorInfo.IME_ACTION_SEARCH -> enterKey.text = "Tìm"
                EditorInfo.IME_ACTION_SEND -> enterKey.text = "Gửi"
                EditorInfo.IME_ACTION_NEXT -> enterKey.text = "Tiếp"
                EditorInfo.IME_ACTION_DONE -> enterKey.text = "Xong"
                else -> enterKey.text = "↵"
            }
        }
    }

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
        
        val prefs = getSharedPreferences("CayIMEPrefs", Context.MODE_PRIVATE)
        isHapticEnabled = prefs.getBoolean("haptic_feedback", true)
        
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
        
        currentShiftState = ShiftState.UNSHIFTED
        updateShiftUI()
        
        updateContextualKeys(keyboardView)
        
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
                if (key is android.widget.ImageView) {
                    key.setImageResource(R.drawable.ic_shift)
                } else if (key is TextView) {
                    key.text = "⇧"
                }
            } else if (currentShiftState == ShiftState.SHIFTED) {
                key.setBackgroundResource(R.drawable.key_bg_selector) // Light up
                if (key is android.widget.ImageView) {
                    key.setImageResource(R.drawable.ic_shift_filled)
                } else if (key is TextView) {
                    key.text = "⇧"
                }
            } else {
                key.setBackgroundResource(R.drawable.key_bg_selector)
                if (key is android.widget.ImageView) {
                    key.setImageResource(R.drawable.ic_capslock)
                } else if (key is TextView) {
                    key.text = "⇪"
                }
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
        val keyboardView = loadKeyboardLayout(layoutResId)
        setInputView(keyboardView)
        updateContextualKeys(keyboardView)
        if (layoutResId == R.layout.keyboard_view) {
            updateShiftUI()
        }
    }

    private fun setupDeleteKey(deleteKey: View?, deleteAction: () -> Unit) {
        if (deleteKey == null) return
        
        deleteKey.setOnTouchListener { v, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> {
                    v.isPressed = true
                    playClickFeedback(v, AudioManager.FX_KEYPRESS_DELETE)
                    deleteAction()
                    
                    if (deleteHandler == null) {
                        deleteHandler = android.os.Handler(android.os.Looper.getMainLooper())
                    }
                    deleteRunnable = object : Runnable {
                        override fun run() {
                            deleteAction()
                            deleteHandler?.postDelayed(this, 50)
                        }
                    }
                    deleteHandler?.postDelayed(deleteRunnable!!, 400)
                    true
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                    v.isPressed = false
                    deleteRunnable?.let { deleteHandler?.removeCallbacks(it) }
                    true
                }
                else -> false
            }
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

        setupDeleteKey(keyboardView.findViewById(R.id.key_delete)) {
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
                setupLetterKey(view, char)
            }
        }
        
        val keyQ = keyboardView.findViewById<View>(R.id.key_q)
        val squareKeys = listOf(R.id.key_shift, R.id.key_delete, R.id.key_123, R.id.key_emoji, R.id.key_web_dot)
        
        keyQ?.post {
            val h = keyQ.height
            if (h > 0) {
                for (id in squareKeys) {
                    val v = keyboardView.findViewById<View>(id)
                    val lp = v?.layoutParams
                    if (lp != null) {
                        lp.width = h
                        lp.height = h
                        if (lp is LinearLayout.LayoutParams) {
                            lp.weight = 0f
                        }
                        v.layoutParams = lp
                    }
                }
            }
        }
        shiftKeyView = keyboardView.findViewById(R.id.key_shift)
        shiftKeyView?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
            val now = System.currentTimeMillis()
            if (now - lastShiftTime < 300) {
                if (currentShiftState == ShiftState.CAPS_LOCK) {
                    currentShiftState = ShiftState.UNSHIFTED
                } else {
                    currentShiftState = ShiftState.CAPS_LOCK
                }
            } else {
                currentShiftState = when (currentShiftState) {
                    ShiftState.UNSHIFTED -> ShiftState.SHIFTED
                    ShiftState.SHIFTED -> ShiftState.UNSHIFTED
                    ShiftState.CAPS_LOCK -> ShiftState.UNSHIFTED
                }
            }
            lastShiftTime = now
            updateShiftUI()
        }

        setupDeleteKey(keyboardView.findViewById(R.id.key_delete)) {
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

        keyboardView.findViewById<View>(R.id.key_web_dot)?.let { webDotKey ->
            webDotKey.setOnClickListener { view ->
                playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
                handleCharacter('.'.code, '.')
            }
            webDotKey.setOnLongClickListener { view ->
                showStringPopup(view, listOf(".", ".vn", ".com", ".net", ".org", ".edu"))
                true
            }
        }

        setupBottomBar(keyboardView)

        updateSpacebarLabel()
        updateShiftUI()
    }

    private fun setupSymbolsKeyboard(keyboardView: View) {
        val keyQ = keyboardView.findViewById<View>(R.id.key_1)
        val squareKeys = listOf(R.id.key_hash, R.id.key_delete, R.id.key_abc, R.id.key_emoji, R.id.key_web_dot)
        
        keyQ?.post {
            val h = keyQ.height
            if (h > 0) {
                for (id in squareKeys) {
                    val v = keyboardView.findViewById<View>(id)
                    val lp = v?.layoutParams
                    if (lp != null) {
                        lp.width = h
                        lp.height = h
                        if (lp is LinearLayout.LayoutParams) {
                            lp.weight = 0f
                        }
                        v.layoutParams = lp
                    }
                }
            }
        }
        
        isSymbolsPage2 = false
        updateSymbolsPage(keyboardView)
        
        keyboardView.findViewById<View>(R.id.key_hash)?.setOnClickListener { view ->
            playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
            isSymbolsPage2 = !isSymbolsPage2
            updateSymbolsPage(keyboardView)
        }

        val symbolKeyIds = listOf(
            R.id.key_1, R.id.key_2, R.id.key_3, R.id.key_4, R.id.key_5, R.id.key_6, R.id.key_7, R.id.key_8, R.id.key_9, R.id.key_0,
            R.id.key_minus, R.id.key_slash, R.id.key_colon, R.id.key_semicolon, R.id.key_lparen, R.id.key_rparen, R.id.key_dollar, R.id.key_ampersand, R.id.key_at, R.id.key_doublequote,
            R.id.key_plus, R.id.key_equal, R.id.key_period, R.id.key_comma, R.id.key_question, R.id.key_exclamation, R.id.key_quote
        )
        
        for (id in symbolKeyIds) {
            keyboardView.findViewById<TextView>(id)?.setOnClickListener { view ->
                playClickFeedback(view, AudioManager.FX_KEYPRESS_STANDARD)
                telexEngine.reset()
                isHandlingKey = true
                val charStr = (view as TextView).text.toString()
                if (charStr.isNotEmpty()) {
                    currentInputConnection?.commitText(charStr, 1)
                    if (lastSelectionStart != -1) {
                        lastSelectionStart += charStr.length
                    }
                }
                isHandlingKey = false
            }
        }

        setupDeleteKey(keyboardView.findViewById(R.id.key_delete)) {
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

        setupDeleteKey(keyboardView.findViewById(R.id.key_delete)) {
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
        if (isHapticEnabled) {
            @Suppress("DEPRECATION")
            view.performHapticFeedback(
                HapticFeedbackConstants.KEYBOARD_TAP,
                HapticFeedbackConstants.FLAG_IGNORE_GLOBAL_SETTING
            )
        }
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

    private fun isPointInsideView(x: Float, y: Float, view: View): Boolean {
        val slop = 20f
        return x >= -slop && x <= view.width + slop && y >= -slop && y <= view.height + slop
    }

    private fun setupLetterKey(view: TextView, char: Char) {
        var longPressRunnable: Runnable? = null
        var isLongPressTriggered = false
        
        view.setOnTouchListener { v, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> {
                    v.isPressed = true
                    isLongPressTriggered = false
                    showKeyPreview(v, char)
                    
                    if (longPressVariants.containsKey(char)) {
                        longPressRunnable = Runnable {
                            isLongPressTriggered = true
                            hideKeyPreview(v)
                            showLongPressPopup(v, char)
                        }
                        v.handler?.postDelayed(longPressRunnable!!, 400)
                    }
                    true
                }
                MotionEvent.ACTION_MOVE -> {
                    if (!isPointInsideView(event.x, event.y, v)) {
                        if (!isLongPressTriggered) {
                            hideKeyPreview(v)
                            v.isPressed = false
                            longPressRunnable?.let { v.handler?.removeCallbacks(it) }
                        }
                    }
                    true
                }
                MotionEvent.ACTION_UP -> {
                    v.isPressed = false
                    longPressRunnable?.let { v.handler?.removeCallbacks(it) }
                    
                    if (!isLongPressTriggered) {
                        hideKeyPreview(v)
                        if (isPointInsideView(event.x, event.y, v)) {
                            playClickFeedback(v, AudioManager.FX_KEYPRESS_STANDARD)
                            handleCharacter(char.uppercaseChar().code, char)
                        }
                    }
                    true
                }
                MotionEvent.ACTION_CANCEL -> {
                    v.isPressed = false
                    longPressRunnable?.let { v.handler?.removeCallbacks(it) }
                    if (!isLongPressTriggered) hideKeyPreview(v)
                    true
                }
                else -> false
            }
        }
    }

    private fun initPreviewPopup() {
        if (previewPopupWindow == null) {
            val inflater = layoutInflater
            val popupRoot = inflater.inflate(R.layout.keyboard_popup, null) as android.widget.FrameLayout
            val popupContainer = popupRoot.findViewById<LinearLayout>(R.id.popup_container)
            
            previewTextView = TextView(this).apply {
                textSize = 36f
                gravity = Gravity.CENTER
                setTextColor(resources.getColor(R.color.key_text, null))
                setPadding(32, 16, 32, 16)
                setBackgroundResource(R.drawable.popup_item_bg)
            }
            popupContainer.addView(previewTextView)
            
            val density = resources.displayMetrics.density
            val triangleHeight = (10 * density).toInt()
            val contentPadding = (8 * density).toInt()
            popupContainer.setPadding(contentPadding, contentPadding, contentPadding, contentPadding + triangleHeight)
            
            val popupBubbleView = popupRoot.findViewById<PopupBubbleView>(R.id.popup_bubble_view)
            popupBubbleView.triangleHeight = triangleHeight.toFloat()
            
            popupRoot.measure(View.MeasureSpec.UNSPECIFIED, View.MeasureSpec.UNSPECIFIED)
            
            previewPopupWindow = PopupWindow(popupRoot, ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, false)
            previewPopupWindow?.isTouchable = false
            previewPopupWindow?.animationStyle = 0
            previewPopupWindow?.setBackgroundDrawable(android.graphics.drawable.ColorDrawable(Color.TRANSPARENT))
        }
    }

    private fun showKeyPreview(anchorView: View, char: Char) {
        initPreviewPopup()
        activePreviewKey = anchorView
        
        val isUpper = currentShiftState == ShiftState.SHIFTED || currentShiftState == ShiftState.CAPS_LOCK
        val displayChar = if (isUpper) char.uppercaseChar() else char.lowercaseChar()
        
        previewTextView?.text = displayChar.toString()
        
        val popupRoot = previewPopupWindow?.contentView as? android.widget.FrameLayout ?: return
        popupRoot.measure(View.MeasureSpec.UNSPECIFIED, View.MeasureSpec.UNSPECIFIED)
        val popupWidth = popupRoot.measuredWidth
        val popupHeight = popupRoot.measuredHeight

        val location = IntArray(2)
        anchorView.getLocationInWindow(location)
        val anchorX = location[0]
        val anchorCenterX = anchorX + anchorView.width / 2

        val density = resources.displayMetrics.density
        val margin = (4 * density).toInt()
        val screenWidth = resources.displayMetrics.widthPixels

        var absolutePopupX = anchorCenterX - popupWidth / 2
        if (absolutePopupX < margin) absolutePopupX = margin
        if (absolutePopupX + popupWidth > screenWidth - margin) absolutePopupX = screenWidth - margin - popupWidth

        val popupBubbleView = popupRoot.findViewById<PopupBubbleView>(R.id.popup_bubble_view)
        popupBubbleView.stemCenterX = (anchorCenterX - absolutePopupX).toFloat()

        val yOffset = location[1] - popupHeight
        
        val rootView = anchorView.rootView
        if (previewPopupWindow?.isShowing == true) {
            previewPopupWindow?.update(absolutePopupX, yOffset, popupWidth, popupHeight)
        } else {
            previewPopupWindow?.width = popupWidth
            previewPopupWindow?.height = popupHeight
            previewPopupWindow?.showAtLocation(rootView, Gravity.NO_GRAVITY, absolutePopupX, yOffset)
        }
    }

    private fun hideKeyPreview(anchorView: View) {
        if (activePreviewKey == anchorView) {
            previewPopupWindow?.dismiss()
            activePreviewKey = null
        }
    }

    private fun showBubblePopup(anchorView: View, contentBuilder: (LinearLayout) -> Unit) {
        val inflater = layoutInflater
        val popupRoot = inflater.inflate(R.layout.keyboard_popup, null) as android.widget.FrameLayout
        val popupContainer = popupRoot.findViewById<LinearLayout>(R.id.popup_container)
        val popupBubbleView = popupRoot.findViewById<PopupBubbleView>(R.id.popup_bubble_view)

        // Let caller populate the container
        contentBuilder(popupContainer)

        // Triangle pointer dimensions (in px)
        val density = resources.displayMetrics.density
        val triangleHeight = (10 * density).toInt()
        val contentPadding = (8 * density).toInt()

        // Set padding on container: space for content + triangle at bottom
        popupContainer.setPadding(contentPadding, contentPadding, contentPadding, contentPadding + triangleHeight)

        // Measure
        popupRoot.measure(View.MeasureSpec.UNSPECIFIED, View.MeasureSpec.UNSPECIFIED)
        val popupWidth = popupRoot.measuredWidth
        val popupHeight = popupRoot.measuredHeight

        // Create PopupWindow with exact size
        popupWindow = PopupWindow(popupRoot, popupWidth, popupHeight, true)
        popupWindow?.setBackgroundDrawable(android.graphics.drawable.ColorDrawable(Color.TRANSPARENT))

        // Calculate horizontal position: center popup on anchor, clamp to screen
        val location = IntArray(2)
        anchorView.getLocationInWindow(location)
        val anchorX = location[0]
        val anchorCenterX = anchorX + anchorView.width / 2
        val screenWidth = resources.displayMetrics.widthPixels
        val margin = (4 * density).toInt()

        var xOffset = (anchorView.width - popupWidth) / 2
        var absolutePopupX = anchorX + xOffset

        if (absolutePopupX < margin) {
            xOffset = margin - anchorX
            absolutePopupX = margin
        } else if (absolutePopupX + popupWidth > screenWidth - margin) {
            val newX = screenWidth - margin - popupWidth
            xOffset = newX - anchorX
            absolutePopupX = newX
        }

        // Set triangle center to point at the anchor key center
        val stemCenterInPopup = (anchorCenterX - absolutePopupX).toFloat()
        popupBubbleView.stemCenterX = stemCenterInPopup
        popupBubbleView.triangleHeight = triangleHeight.toFloat()

        // Show right above the anchor key (popup bottom touching key top)
        val yOffset = -(anchorView.height + popupHeight)
        popupWindow?.showAsDropDown(anchorView, xOffset, yOffset)
        playClickFeedback(anchorView, AudioManager.FX_KEYPRESS_STANDARD)
    }

    private fun showLongPressPopup(anchorView: View, baseChar: Char) {
        val variants = longPressVariants[baseChar] ?: return
        val isUpper = currentShiftState == ShiftState.SHIFTED || currentShiftState == ShiftState.CAPS_LOCK

        showBubblePopup(anchorView) { container ->
            for (c in variants) {
                val displayChar = if (isUpper) c.uppercaseChar() else c
                val textView = TextView(this).apply {
                    text = displayChar.toString()
                    textSize = 28f
                    setTextColor(resources.getColor(R.color.key_text, null))
                    setPadding(24, 8, 24, 8)
                    setBackgroundResource(R.drawable.popup_item_bg)
                    setOnClickListener {
                        playClickFeedback(it, AudioManager.FX_KEYPRESS_STANDARD)
                        handleCharacter(displayChar.uppercaseChar().code, displayChar)
                        popupWindow?.dismiss()
                    }
                }
                container.addView(textView)
            }
        }
    }

    private fun showStringPopup(anchorView: View, options: List<String>) {
        showBubblePopup(anchorView) { container ->
            for (str in options) {
                val textView = TextView(this).apply {
                    text = str
                    textSize = 22f
                    setTextColor(resources.getColor(R.color.key_text, null))
                    setPadding(24, 8, 24, 8)
                    setBackgroundResource(R.drawable.popup_item_bg)
                    setOnClickListener {
                        playClickFeedback(it, AudioManager.FX_KEYPRESS_STANDARD)
                        telexEngine.reset()
                        isHandlingKey = true
                        currentInputConnection?.commitText(str, 1)
                        if (lastSelectionStart != -1) {
                            lastSelectionStart += str.length
                        }
                        isHandlingKey = false
                        popupWindow?.dismiss()
                    }
                }
                container.addView(textView)
            }
        }
    }
}
