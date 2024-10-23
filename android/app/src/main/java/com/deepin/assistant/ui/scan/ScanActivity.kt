package com.deepin.assistant.ui.scan

import android.content.Context
import android.content.Intent
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.AppCompatButton
import com.deepin.assistant.R

/**
 * 扫描二维码Activity
 *
 * @author re2zero
 * @use NBZxing
 */
class ScanActivity : AppCompatActivity() {
    private var scan_view: ScanView? = null
    private var button: AppCompatButton? = null


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContentView(R.layout.activity_scan)
        initView()
        initData()
    }

    companion object {
        fun startSelf(context: Context) {
            context.startActivity(Intent(context, ScanActivity::class.java))
        }
    }

    //必须调用
    fun initView() {
        scan_view = findViewById<ScanView>(R.id.nbscanview)
        button = findViewById<AppCompatButton>(R.id.myButton)
    }

    //必须调用
    fun initData() {
        //绑定结果回调和启动
        scan_view!!.setResultCallback(this)
        scan_view!!.synchLifeStart(this)

        button?.setOnClickListener {
            finish()
        }
    }

    fun setResult(result: String) {
        setResult(RESULT_OK, Intent().putExtra("SCAN_RESULT", result))
        finish()
    }

    override fun onDestroy() {
        super.onDestroy()
    }
}