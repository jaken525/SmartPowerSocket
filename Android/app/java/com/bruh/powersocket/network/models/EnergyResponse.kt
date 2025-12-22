package com.bruh.powersocket.network.models

data class EnergyResponse(
    val status: String,
    val data: EnergyData?
)

data class EnergyData(
    val energy: Float,
    val timestamp: Long
)
