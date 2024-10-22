// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

package com.deepin.assistant.ui.activity

import android.app.Activity
import android.content.*
import android.os.Bundle
import android.util.Base64
import android.util.Log
import androidx.core.content.ContextCompat
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProvider
import androidx.viewpager.widget.ViewPager
import com.gyf.immersionbar.ImmersionBar
import com.hjq.base.FragmentPagerAdapter
import com.deepin.assistant.R
import com.deepin.assistant.app.AppActivity
import com.deepin.assistant.app.AppFragment
import com.deepin.assistant.manager.*
import com.deepin.assistant.model.SharedViewModel
import com.deepin.assistant.other.DoubleClickHelper
import com.deepin.assistant.ui.adapter.NavigationAdapter
import com.deepin.assistant.ui.fragment.HomeFragment
import com.deepin.assistant.ui.fragment.MessageFragment
import com.deepin.assistant.ui.fragment.MineFragment
import com.deepin.assistant.ui.fragment.FirstFragment
import com.deepin.cooperation.CooperationListener
import com.deepin.cooperation.JniCooperation
import net.christianbeier.droidvnc_ng.MainService
import net.christianbeier.droidvnc_ng.Utils
import org.json.JSONObject

class HomeActivity : AppActivity(), NavigationAdapter.OnNavigationListener {

    companion object {
        private const val TAG = "HomeActivity"
        private const val INTENT_KEY_IN_FRAGMENT_INDEX: String = "fragmentIndex"
        private const val INTENT_KEY_IN_FRAGMENT_CLASS: String = "fragmentClass"

        @JvmOverloads
        fun start(context: Context, fragmentClass: Class<out AppFragment<*>?>? = FirstFragment::class.java) {
            val intent = Intent(context, HomeActivity::class.java)
            intent.putExtra(INTENT_KEY_IN_FRAGMENT_CLASS, fragmentClass)
            if (context !is Activity) {
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            context.startActivity(intent)
        }
    }

    private lateinit var viewModel: SharedViewModel
    private var mCooperation: JniCooperation? = null

    private val viewPager: ViewPager? by lazy { findViewById(R.id.vp_home_pager) }
//    private val navigationView: RecyclerView? by lazy { findViewById(R.id.rv_home_navigation) }
    private var navigationAdapter: NavigationAdapter? = null
    private var pagerAdapter: FragmentPagerAdapter<AppFragment<*>>? = null

    override fun getLayoutId(): Int {
        return R.layout.home_activity
    }

    override fun initView() {
        navigationAdapter = NavigationAdapter(this).apply {
            addItem(NavigationAdapter.MenuItem(getString(R.string.home_nav_index),
                ContextCompat.getDrawable(this@HomeActivity, R.drawable.home_home_selector)))
            addItem(NavigationAdapter.MenuItem(getString(R.string.home_nav_found),
                ContextCompat.getDrawable(this@HomeActivity, R.drawable.home_found_selector)))
            addItem(NavigationAdapter.MenuItem(getString(R.string.home_nav_message),
                ContextCompat.getDrawable(this@HomeActivity, R.drawable.home_message_selector)))
            addItem(NavigationAdapter.MenuItem(getString(R.string.home_nav_me),
                ContextCompat.getDrawable(this@HomeActivity, R.drawable.home_me_selector)))
            setOnNavigationListener(this@HomeActivity)
//            navigationView?.adapter = this
        }

        mCooperation = JniCooperation()
        val hosts = MainService.getIPv4s()
        val host = hosts[0]
        mCooperation?.initNative(host)

        mCooperation?.registerListener(object : CooperationListener {
            override fun onConnectChanged(result: Int, reason: String) {
                Log.d(TAG, "Connection changed: result $result, reason: $reason")
            }

            override fun onAsyncRpcResult(type: Int, response: String) {
                Log.d(TAG, "Async RPC result: type $type, response: $response")
                when (type) {
                    JniCooperation.RPC_APPLY_LOGIN -> {
                        //type 1000, response: {"auth":"thatsgood","name":"10.8.11.52"}
                        if (response.isNullOrEmpty()) {
                            Log.e(TAG, "FAIL to connect...")
                            // TODO: show connection failed diaglog.
                            return
                        }
                        val jsonObject = JSONObject(response)
                        val auth = jsonObject.getString("auth")
                        val name = jsonObject.getString("name")
                        if (auth.isNullOrEmpty()) {
                            Log.e(TAG, "FAIL to connect...")
                            // TODO: show connection failed diaglog.
                            return
                        }

                        Log.d(TAG, "Login success: name $name, auth: $auth")
                        this@HomeActivity.viewModel.updateInfo(auth, name)
                        start(this@HomeActivity, HomeFragment::class.java)
                    }
                    JniCooperation.RPC_APPLY_PROJECTION_RESULT -> {
                    }
                    JniCooperation.RPC_APPLY_PROJECTION_STOP -> {
                    }
                }
            }
        })
    }

    override fun initData() {
        pagerAdapter = FragmentPagerAdapter<AppFragment<*>>(this).apply {
            addFragment(HomeFragment.newInstance())
            addFragment(FirstFragment.newInstance())
            addFragment(MessageFragment.newInstance())
            addFragment(MineFragment.newInstance())
            viewPager?.adapter = this
        }
        onNewIntent(intent)

        val deviceName = Utils.getDeviceName(this)
        deviceName?.let { mCooperation?.setDeviceName(it) }

        viewModel = ViewModelProvider(this).get(SharedViewModel::class.java)
        viewModel.action().observe(this, Observer {
            Log.d(TAG, "will do the action: $it")
            when (it) {
                JniCooperation.ACTION_PROJECTION_START -> {
                    deviceName?.let { it -> mCooperation?.sendProjection(it, 5900) }
                }

                JniCooperation.ACTION_PROJECTION_STOP -> {
                    deviceName?.let { it -> mCooperation?.stopProjection(it) }
                }
            }
        })
    }

    override fun onNewIntent(intent: Intent?) {
        super.onNewIntent(intent)
        pagerAdapter?.let {
            switchFragment(it.getFragmentIndex(getSerializable(INTENT_KEY_IN_FRAGMENT_CLASS)))
        }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        viewPager?.let {
            // 保存当前 Fragment 索引位置
            outState.putInt(INTENT_KEY_IN_FRAGMENT_INDEX, it.currentItem)
        }
    }

    override fun onRestoreInstanceState(savedInstanceState: Bundle) {
        super.onRestoreInstanceState(savedInstanceState)
        // 恢复当前 Fragment 索引位置
        switchFragment(savedInstanceState.getInt(INTENT_KEY_IN_FRAGMENT_INDEX))
    }

    private fun switchFragment(fragmentIndex: Int) {
        if (fragmentIndex == -1) {
            return
        }
        when (fragmentIndex) {
            0, 1, 2, 3 -> {
                viewPager?.currentItem = fragmentIndex
                navigationAdapter?.setSelectedPosition(fragmentIndex)
            }
        }
    }

    /**
     * [NavigationAdapter.OnNavigationListener]
     */
    override fun onNavigationItemSelected(position: Int): Boolean {
        return when (position) {
            0, 1, 2, 3 -> {
                viewPager?.currentItem = position
                true
            }
            else -> false
        }
    }

    override fun createStatusBarConfig(): ImmersionBar {
        return super.createStatusBarConfig() // 指定导航栏背景颜色
            .navigationBarColor(R.color.white)
    }

    override fun onBackPressed() {
        if (!DoubleClickHelper.isOnDoubleClick()) {
            toast(R.string.home_exit_hint)
            return
        }

        // 移动到上一个任务栈，避免侧滑引起的不良反应
        moveTaskToBack(false)
        postDelayed({
            // 进行内存优化，销毁掉所有的界面
            ActivityManager.getInstance().finishAllActivities()
        }, 300)
    }

    override fun onDestroy() {
        super.onDestroy()
        viewPager?.adapter = null
//        navigationView?.adapter = null
        navigationAdapter?.setOnNavigationListener(null)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode == RESULT_OK) {
            val resultCodeFromScan = data?.getStringExtra("SCAN_RESULT")
            Log.d(TAG, "扫描结果传回 HomeActivity: $resultCodeFromScan")

            resultCodeFromScan?.let {
                // 先进行 base64 解码
                val decodedBytes = Base64.decode(it, Base64.DEFAULT)
                val decodedString = String(decodedBytes)

                // 按照"&"分割字符串
                val parameters = decodedString.split("&")
                if (parameters.size == 3) {
                    // 创建一个存储解析结果的 Map
                    val resultMap = mutableMapOf<String, String>()

                    // 遍历参数并进行分割
                    for (parameter in parameters) {
                        val keyValue = parameter.split("=")
                        if (keyValue.size == 2) {
                            resultMap[keyValue[0]] = keyValue[1]
                        } else {
                            Log.e(TAG, "解析出错，参数格式不正确：$parameter")
                        }
                    }

                    // 提取出 ip、port 和 pin
                    val ip = resultMap["host"]
                    val portString = resultMap["port"]
                    val pin = resultMap["pin"]

                    // 将 port 转换为 Int，如果失败默认为 0
                    val port = portString?.toIntOrNull() ?: 0

                    if (ip != null && pin != null) {
                        // 调用连接方法
                        connectCooperation(ip, port, pin)
                    } else {
                        Log.e(TAG, "解析出错，参数不完整：host=$ip, port=$portString, pin=$pin")
                    }
                } else {
                    Log.e(TAG, "解析出错，参数数量不匹配：$decodedString")
                }
            }
        }
    }

    fun connectCooperation(ip: String, port: Int, pin: String) {
        mCooperation?.connectRemote(ip, port, pin)
    }
}