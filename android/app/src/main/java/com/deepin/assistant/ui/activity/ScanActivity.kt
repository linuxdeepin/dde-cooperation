// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

package com.deepin.assistant.ui.activity

import android.content.ContentValues.TAG
import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.deepin.assistant.R
import android.util.Log
import androidx.activity.result.contract.ActivityResultContracts
import com.deepin.assistant.utils.Utils
import com.journeyapps.barcodescanner.CaptureActivity

class ScanActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.scan_activity)

        // 初始化视图
        val button: Button = findViewById(R.id.button)
        val tTextView: TextView = findViewById(R.id.title)
        val deviceInfoTextView: TextView = findViewById(R.id.device_info)

        val deviceName = Build.MANUFACTURER
        val ipAddress = Utils.getIPAddress(this)

        //设置设备信息（可根据实际情况动态获取）
        deviceInfoTextView.text = "$deviceName | IP：${ipAddress ?: ""}"

        button.setOnClickListener {
            scanForDevices()
        }
    }

    private val barcodeLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        if (result.resultCode == RESULT_OK) {
            val data = result.data
            data?.let {
                val code = it.getStringExtra("SCAN_RESULT")
                Log.d(TAG, "扫描结果: $code")
                val intent = Intent(this, CooperationActivity::class.java)
                startActivity(intent)
            }
        }
    }

    private fun scanForDevices() {
        val intent = Intent(this, CaptureActivity::class.java)
        barcodeLauncher.launch(intent)
    }
}