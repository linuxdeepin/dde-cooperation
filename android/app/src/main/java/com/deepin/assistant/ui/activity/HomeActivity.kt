package com.deepin.assistant.ui.activity

import android.app.Activity
import android.content.*
import android.os.Bundle
import android.util.Log
import androidx.core.content.ContextCompat
import androidx.preference.PreferenceManager
import androidx.recyclerview.widget.RecyclerView
import androidx.viewpager.widget.ViewPager
import com.gyf.immersionbar.ImmersionBar
import com.hjq.base.FragmentPagerAdapter
import com.deepin.assistant.R
import com.deepin.assistant.app.AppActivity
import com.deepin.assistant.app.AppFragment
import com.deepin.assistant.manager.*
import com.deepin.assistant.other.DoubleClickHelper
import com.deepin.assistant.ui.adapter.NavigationAdapter
import com.deepin.assistant.ui.fragment.HomeFragment
import com.deepin.assistant.ui.fragment.MessageFragment
import com.deepin.assistant.ui.fragment.MineFragment
import com.deepin.assistant.ui.fragment.ScanFragment
import com.deepin.cooperation.CooperationListener
import com.deepin.cooperation.JniCooperation
import net.christianbeier.droidvnc_ng.MainService

/**
 *    author : Android 轮子哥
 *    github : https://github.com/getActivity/AndroidProject-Kotlin
 *    time   : 2018/10/18
 *    desc   : 首页界面
 */
class HomeActivity : AppActivity(), NavigationAdapter.OnNavigationListener {

    companion object {
        private const val TAG = "HomeActivity"
        private const val INTENT_KEY_IN_FRAGMENT_INDEX: String = "fragmentIndex"
        private const val INTENT_KEY_IN_FRAGMENT_CLASS: String = "fragmentClass"

        @JvmOverloads
        fun start(context: Context, fragmentClass: Class<out AppFragment<*>?>? = ScanFragment::class.java) {
            val intent = Intent(context, HomeActivity::class.java)
            intent.putExtra(INTENT_KEY_IN_FRAGMENT_CLASS, fragmentClass)
            if (context !is Activity) {
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            context.startActivity(intent)
        }
    }

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
            }
        })
    }

    override fun initData() {
        pagerAdapter = FragmentPagerAdapter<AppFragment<*>>(this).apply {
            addFragment(HomeFragment.newInstance())
            addFragment(ScanFragment.newInstance())
            addFragment(MessageFragment.newInstance())
            addFragment(MineFragment.newInstance())
            viewPager?.adapter = this
        }
        onNewIntent(intent)
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
}