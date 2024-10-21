// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

package com.deepin.cooperation

import android.util.Log

class JniCooperation {
    private val TAG = "JniCooperation"

    private var listener: CooperationListener? = null


    /**
     * Native methods that are implemented by the 'cooperation' native library,
     * which is packaged with this application.
     */
    external fun nativeVersion(): String
    external fun initNative(ip: String)
    external fun setStorageFolder(dirName: String)
    external fun connectRemote(ip: String, port: Int, pin: String)
    external fun disconnectRemote()
    external fun sendProjection(niceName: String, vncPort: Int)
    external fun stopProjection(niceName: String)
    external fun getStatus(): Int

    companion object {
        // Used to load the 'cooperation' library on application startup.
        init {
            System.loadLibrary("cooperation")
        }
    }

    fun registerListener(listener: CooperationListener) {
        this.listener = listener
    }

    // @SuppressLint("WakelockTimeout")
    @Suppress("unused")
    fun onConnectChanged(result: Int, reason: String) {
        Log.d(TAG, "onConnectChanged: result $result reason:$reason")

        try {
            listener?.onConnectChanged(result, reason)
        } catch (e: Exception) {
            // instance probably null
            Log.e(TAG, "onClientConnected: error: $e")
        }
    }

    @Suppress("unused")
    fun onAsyncRpcResult(type: Int, response: String) {
        Log.d(TAG, "onAsyncRpcResult: type $type response: $response")

        try {
            listener?.onAsyncRpcResult(type, response)
        } catch (e: Exception) {
            // instance probably null
            Log.e(TAG, "onAsyncRpcResult: error: $e")
        }
    }
}