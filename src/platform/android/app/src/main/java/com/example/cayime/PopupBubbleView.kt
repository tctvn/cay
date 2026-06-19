package com.example.cayime

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.util.AttributeSet
import android.view.View
import androidx.core.content.ContextCompat

/**
 * Draws an iOS-style chat bubble: rounded rectangle body + small downward triangle pointer.
 * The triangle points at the key being long-pressed.
 */
class PopupBubbleView @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.key_bg)
        style = Paint.Style.FILL
        setShadowLayer(12f, 0f, 4f, 0x44000000)
    }

    private val path = Path()

    /** X center of the triangle pointer, relative to this view's left edge */
    var stemCenterX = 0f

    /** Height of the downward triangle pointer in pixels */
    var triangleHeight = 24f

    /** Half-width of the triangle base in pixels */
    var triangleHalfWidth = 18f

    /** Corner radius of the rounded rectangle body */
    var cornerRadius = 24f

    init {
        setLayerType(LAYER_TYPE_SOFTWARE, paint)
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        path.reset()

        val bodyBottom = height.toFloat() - triangleHeight
        val bodyRect = RectF(0f, 0f, width.toFloat(), bodyBottom)

        // Rounded rectangle body
        path.addRoundRect(bodyRect, cornerRadius, cornerRadius, Path.Direction.CW)

        // Downward triangle pointer
        val triLeft = (stemCenterX - triangleHalfWidth).coerceAtLeast(cornerRadius)
        val triRight = (stemCenterX + triangleHalfWidth).coerceAtMost(width.toFloat() - cornerRadius)
        val triTip = stemCenterX.coerceIn(triLeft, triRight)

        val triPath = Path()
        triPath.moveTo(triLeft, bodyBottom)
        triPath.lineTo(triTip, height.toFloat())
        triPath.lineTo(triRight, bodyBottom)
        triPath.close()
        path.addPath(triPath)

        canvas.drawPath(path, paint)
    }
}
