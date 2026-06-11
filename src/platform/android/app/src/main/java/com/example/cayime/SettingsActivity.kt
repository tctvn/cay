package com.example.cayime

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.provider.Settings
import android.view.inputmethod.InputMethodManager
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class SettingsActivity : AppCompatActivity() {

    private lateinit var btnEnableKeyboard: Button
    private lateinit var tvStatus: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_settings)

        btnEnableKeyboard = findViewById(R.id.btnEnableKeyboard)
        tvStatus = findViewById(R.id.tvStatus)
        val btnOpenSettings = findViewById<Button>(R.id.btnOpenSettings)

        btnEnableKeyboard.setOnClickListener {
            startActivity(Intent(Settings.ACTION_INPUT_METHOD_SETTINGS))
        }

        btnOpenSettings.setOnClickListener {
            startActivity(Intent(Settings.ACTION_INPUT_METHOD_SETTINGS))
        }

        if (intent.getBooleanExtra("show_voice_dialog", false)) {
            val builder = android.app.AlertDialog.Builder(this)
            builder.setTitle("Cần bật Nhập giọng nói")
            builder.setMessage("Để dùng micro, hãy tìm và BẬT mục 'Nhập giọng nói của Google' (Google Voice Typing) trong cài đặt sắp mở.\n\n(Nếu không có, bạn cần cài đặt ứng dụng Google từ Play Store)")
            builder.setPositiveButton("Mở Cài đặt") { _, _ ->
                startActivity(Intent(Settings.ACTION_INPUT_METHOD_SETTINGS))
            }
            builder.setNegativeButton("Hủy", null)
            builder.show()
        }
    }

    override fun onResume() {
        super.onResume()
        checkKeyboardStatus()
    }

    private fun checkKeyboardStatus() {
        val imm = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
        val enabledMethods = imm.enabledInputMethodList
        
        // Check if our CayIME is in the enabled list
        val isEnabled = enabledMethods.any { it.packageName == packageName }

        if (isEnabled) {
            tvStatus.text = "Status: Keyboard is enabled!"
            tvStatus.setTextColor(resources.getColor(android.R.color.holo_green_dark, theme))
            btnEnableKeyboard.text = "Keyboard Enabled"
            btnEnableKeyboard.isEnabled = false
        } else {
            tvStatus.text = "Status: Keyboard is NOT enabled."
            tvStatus.setTextColor(resources.getColor(android.R.color.holo_red_dark, theme))
            btnEnableKeyboard.text = getString(R.string.enable_keyboard)
            btnEnableKeyboard.isEnabled = true
        }
    }
}
