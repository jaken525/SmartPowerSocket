package com.bruh.powersocket

import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import android.view.View
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val powerButton = findViewById<Button>(R.id.powerButton)
        val statsButton = findViewById<Button>(R.id.statsButton)
        val voltageText = findViewById<TextView>(R.id.voltageText)
        val statusIndicator = findViewById<View>(R.id.statusIndicator)
    }
}