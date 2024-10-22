// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

package com.deepin.assistant.ui.fragment

import android.content.Intent
import android.view.View
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.AppCompatButton
import com.deepin.assistant.R
import com.deepin.assistant.aop.SingleClick
import com.deepin.assistant.app.TitleBarFragment
import com.deepin.assistant.ui.activity.HomeActivity
import com.deepin.assistant.utils.Utils

import com.journeyapps.barcodescanner.CaptureActivity

class ScanFragment : TitleBarFragment<HomeActivity>() {

    companion object {
        private const val TAG = "ScanFragment"

        fun newInstance(): ScanFragment {
            return ScanFragment()
        }
    }

    // 初始化视图
    private val button: AppCompatButton? by lazy { findViewById(R.id.button) }
    private val tTextView: TextView? by lazy { findViewById(R.id.title) }
    private val deviceInfoTextView: TextView? by lazy { findViewById(R.id.device_info) }


    override fun getLayoutId(): Int {
        return R.layout.scan_fragment
    }

    override fun initView() {
        setOnClickListener(button)
    }

    override fun initData() {
        val deviceName = Utils.getDeviceName(requireContext())
        val ipAddress = Utils.getIPAddress(requireContext())
        deviceInfoTextView?.text = "$deviceName | IP：${ipAddress ?: ""}"
    }

    private val barcodeLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        if (result.resultCode == AppCompatActivity.RESULT_OK) {
            val data = result.data
            data?.let {
                val code = it.getStringExtra("SCAN_RESULT")
                val resultIntent = Intent()
                resultIntent.putExtra("SCAN_RESULT", code)
                requireActivity().setResult(AppCompatActivity.RESULT_OK, resultIntent)
            }
        }
    }

    private fun scanForDevices() {
        val intent = Intent(requireActivity(), CaptureActivity::class.java)
        barcodeLauncher.launch(intent)
    }

    @SingleClick
    override fun onClick(view: View) {
        if (view === button) {
            scanForDevices()
        }
    }

    override fun isStatusBarEnabled(): Boolean {
        // 使用沉浸式状态栏
        return !super.isStatusBarEnabled()
    }
}