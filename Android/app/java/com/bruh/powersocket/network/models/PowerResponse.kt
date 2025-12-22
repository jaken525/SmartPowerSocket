package com.bruh.powersocket.network.models

data class PowerResponse(
    val status: String,
    val data: PowerData
)

data class PowerData(
    val voltage: Float,
    val current: Float,
    val power: Float,
    val energy: Float
)
