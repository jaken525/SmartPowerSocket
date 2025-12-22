package com.bruh.powersocket.network

import com.bruh.powersocket.network.models.*
import retrofit2.Call
import retrofit2.http.GET
import retrofit2.http.Path

interface ApiService {

    @GET("on")
    fun turnOn(): Call<StatusResponse>

    @GET("off")
    fun turnOff(): Call<StatusResponse>

    @GET("toggle")
    fun toggle(): Call<StatusResponse>

    @GET("status")
    fun getStatus(): Call<StatusResponse>

    @GET("power")
    fun getPower(): Call<PowerResponse>

    @GET("energy")
    fun getEnergy(): Call<EnergyResponse>

    @GET("stats/{period}")
    fun getStats(@Path("period") period: String): Call<EnergyResponse>
}