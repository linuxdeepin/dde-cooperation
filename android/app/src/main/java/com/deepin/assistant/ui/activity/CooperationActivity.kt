// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

package com.deepin.assistant.ui.activity

import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.deepin.assistant.R
import com.deepin.assistant.utils.Utils

class CooperationActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.cooperation_activity)

        // 初始化视图
        val screenCastingButton: Button = findViewById(R.id.screen_casting)
        val backButton: Button = findViewById(R.id.back_button)
        val titleTextView: TextView = findViewById(R.id.title)
        val deviceInfoTextView: TextView = findViewById(R.id.device_info)

        val deviceName = Build.MANUFACTURER // 动态获取设备名称
        val ipAddress = Utils.getIPAddress(this)// IP地址

        //设置设备信息（可根据实际情况动态获取）
        deviceInfoTextView.text = "$deviceName | IP：${ipAddress ?: ""}"

        val screenCastingMessage = getString(R.string.cooperation_activity_screen_casting)
        val endScreenCastingMessage = getString(R.string.cooperation_activity_end_screen)
        val cooperationMessage = getString(R.string.cooperation_activity_cooperation)
        val screenCast = getString(R.string.cooperation_activity_screencast)

        screenCastingButton.setOnClickListener {
            val condition = screenCastingButton.text == screenCast
            screenCastingButton.text = if (condition) endScreenCastingMessage else screenCast
            titleTextView.text = if (condition) screenCastingMessage else cooperationMessage
        }
        backButton.setOnClickListener {
            val intent = Intent(this, HomeActivity::class.java)
            startActivity(intent)
        }

    }
}