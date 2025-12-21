package com.bruh.powersocket

import android.graphics.drawable.Drawable
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.bruh.powersocket.network.ApiClient
import com.bruh.powersocket.network.ApiService
import com.bruh.powersocket.network.models.PowerResponse
import com.bruh.powersocket.network.models.StatusResponse
import retrofit2.Call
import retrofit2.Callback
import retrofit2.Response

class MainActivity : AppCompatActivity() {

    private lateinit var api: ApiService

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        api = ApiClient.retrofit.create(ApiService::class.java)

        val powerButton = findViewById<Button>(R.id.powerButton)
        val statsButton = findViewById<Button>(R.id.statsButton)
        val voltageText = findViewById<TextView>(R.id.voltageText)
        val statusIndicator = findViewById<View>(R.id.statusIndicator)

        //  Включить / Выключить
        powerButton.setOnClickListener {
            api.toggle().enqueue(object : Callback<StatusResponse> {
                override fun onResponse(
                    call: Call<StatusResponse>,
                    response: Response<StatusResponse>
                ) {
                    response.body()?.let {
                        updateStatus(it.state, statusIndicator)
                    }
                }

                override fun onFailure(call: Call<StatusResponse>, t: Throwable) {}
            })
        }

        //  Получить статистику
        statsButton.setOnClickListener {
            api.getPower().enqueue(object : Callback<PowerResponse> {
                override fun onResponse(
                    call: Call<PowerResponse>,
                    response: Response<PowerResponse>
                ) {
                    response.body()?.let {
                        voltageText.text = "Напряжение: ${it.data.voltage} В"
                    }
                }

                override fun onFailure(call: Call<PowerResponse>, t: Throwable) {}
            })
        }
    }

    private fun updateStatus(state: String, indicator: View) {
        val drawable: Drawable? = if (state == "on") {
            ContextCompat.getDrawable(this, R.drawable.status_green)
        } else {
            ContextCompat.getDrawable(this, R.drawable.status_red)
        }
        indicator.background = drawable
    }
}
