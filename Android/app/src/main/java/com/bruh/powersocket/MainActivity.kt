package com.bruh.powersocket

import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import com.bruh.powersocket.network.ApiClient
import com.bruh.powersocket.network.ApiService
import com.bruh.powersocket.network.models.PowerResponse
import com.bruh.powersocket.network.models.StatusResponse
import retrofit2.Call
import retrofit2.Callback
import retrofit2.Response

class MainActivity : AppCompatActivity() {

    private lateinit var api: ApiService  // Добавлено поле

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        api = ApiClient.retrofit.create(ApiService::class.java)  // Добавлена инициализация

        val powerButton = findViewById<Button>(R.id.powerButton)
        val statsButton = findViewById<Button>(R.id.statsButton)
        val voltageText = findViewById<TextView>(R.id.voltageText)
        val statusIndicator = findViewById<View>(R.id.statusIndicator)
    }
}