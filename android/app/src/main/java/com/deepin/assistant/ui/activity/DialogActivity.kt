package com.deepin.assistant.ui.activity

import android.content.Intent
import android.view.*
import android.widget.*
import com.hjq.base.BaseDialog
import com.hjq.base.BasePopupWindow
import com.hjq.base.action.AnimAction
import com.deepin.assistant.R
import com.deepin.assistant.aop.SingleClick
import com.deepin.assistant.app.AppActivity
import com.deepin.assistant.manager.DialogManager
import com.deepin.assistant.ui.dialog.*
import com.deepin.assistant.ui.popup.ListPopup
import com.hjq.umeng.Platform
import com.hjq.umeng.UmengClient
import com.hjq.umeng.UmengShare.OnShareListener
import com.umeng.socialize.media.UMImage
import com.umeng.socialize.media.UMWeb
import java.util.*

/**
 *    author : Android 轮子哥
 *    github : https://github.com/getActivity/AndroidProject-Kotlin
 *    time   : 2018/12/02
 *    desc   : 对话框使用案例
 */
class DialogActivity : AppActivity() {

    /** 等待对话框 */
    private var waitDialog: BaseDialog? = null

    override fun getLayoutId(): Int {
        return R.layout.dialog_activity
    }

    override fun initView() {
        setOnClickListener(R.id.btn_dialog_message, R.id.btn_dialog_input,
            R.id.btn_dialog_bottom_menu, R.id.btn_dialog_center_menu,
            R.id.btn_dialog_single_select, R.id.btn_dialog_more_select,
            R.id.btn_dialog_succeed_toast, R.id.btn_dialog_fail_toast,
            R.id.btn_dialog_warn_toast, R.id.btn_dialog_wait,
            R.id.btn_dialog_update, R.id.btn_dialog_share,
            R.id.btn_dialog_custom,
            R.id.btn_dialog_multi
        )
    }

    override fun initData() {}

    @SingleClick
    override fun onClick(view: View) {
        when (view.id) {
            R.id.btn_dialog_message -> {

                // 消息对话框
                MessageDialog.Builder(this)
                    // 标题可以不用填写
                    .setTitle("我是标题")
                    // 内容必须要填写
                    .setMessage("我是内容")
                    // 确定按钮文本
                    .setConfirm(getString(R.string.common_confirm))
                    // 设置 null 表示不显示取消按钮
                    .setCancel(getString(R.string.common_cancel))
                    // 设置点击按钮后不关闭对话框
                    //.setAutoDismiss(false)
                    .setListener(object : MessageDialog.OnListener {

                        override fun onConfirm(dialog: BaseDialog?) {
                            toast("确定了")
                        }

                        override fun onCancel(dialog: BaseDialog?) {
                            toast("取消了")
                        }
                    })
                    .show()
            }
            R.id.btn_dialog_input -> {

                // 输入对话框
                InputDialog.Builder(this)
                    // 标题可以不用填写
                    .setTitle("我是标题")
                    // 内容可以不用填写
                    .setContent("我是内容")
                    // 提示可以不用填写
                    .setHint("我是提示")
                    // 确定按钮文本
                    .setConfirm(getString(R.string.common_confirm))
                    // 设置 null 表示不显示取消按钮
                    .setCancel(getString(R.string.common_cancel))
                    // 设置点击按钮后不关闭对话框
                    //.setAutoDismiss(false)
                    .setListener(object : InputDialog.OnListener {

                        override fun onConfirm(dialog: BaseDialog?, content: String) {
                            toast("确定了：$content")
                        }

                        override fun onCancel(dialog: BaseDialog?) {
                            toast("取消了")
                        }
                    })
                    .show()
            }
            R.id.btn_dialog_bottom_menu -> {

                val data = ArrayList<String>()
                for (i in 0..9) {
                    data.add("我是数据" + (i + 1))
                }

                // 底部选择框
                MenuDialog.Builder(this)
                    // 设置 null 表示不显示取消按钮
                    //.setCancel(getString(R.string.common_cancel))
                    // 设置点击按钮后不关闭对话框
                    //.setAutoDismiss(false)
                    .setList(data)
                    .setListener(object : MenuDialog.OnListener<String> {

                        override fun onSelected(dialog: BaseDialog?, position: Int, data: String) {
                            toast("位置：$position，文本：$data")
                        }

                        override fun onCancel(dialog: BaseDialog?) {
                            toast("取消了")
                        }
                    })
                    .show()

            }
            R.id.btn_dialog_center_menu -> {

                val data = ArrayList<String>()
                for (i in 0..9) {
                    data.add("我是数据" + (i + 1))
                }
                // 居中选择框
                MenuDialog.Builder(this)
                    .setGravity(Gravity.CENTER)
                    // 设置 null 表示不显示取消按钮
                    //.setCancel(null)
                    // 设置点击按钮后不关闭对话框
                    //.setAutoDismiss(false)
                    .setList(data)
                    .setListener(object : MenuDialog.OnListener<String> {

                        override fun onSelected(dialog: BaseDialog?, position: Int, data: String) {
                            toast("位置：$position，文本：$data")
                        }

                        override fun onCancel(dialog: BaseDialog?) {
                            toast("取消了")
                        }
                    })
                    .show()

            }
            R.id.btn_dialog_single_select -> {

                // 单选对话框
                SelectDialog.Builder(this)
                    .setTitle("请选择你的性别")
                    .setList("男", "女")
                    // 设置单选模式
                    .setSingleSelect()
                    // 设置默认选中
                    .setSelect(0)
                    .setListener(object : SelectDialog.OnListener<String> {

                        override fun onSelected(dialog: BaseDialog?, data: HashMap<Int, String>) {
                            toast("确定了：$data")
                        }

                        override fun onCancel(dialog: BaseDialog?) {
                            toast("取消了")
                        }
                    })
                    .show()

            }
            R.id.btn_dialog_more_select -> {

                // 多选对话框
                SelectDialog.Builder(this)
                    .setTitle("请选择工作日")
                    .setList("星期一", "星期二", "星期三", "星期四", "星期五")
                    // 设置最大选择数
                    .setMaxSelect(3)
                    // 设置默认选中
                    .setSelect(2, 3, 4)
                    .setListener(object : SelectDialog.OnListener<String> {

                        override fun onSelected(dialog: BaseDialog?, data: HashMap<Int, String>) {
                            toast("确定了：$data")
                        }

                        override fun onCancel(dialog: BaseDialog?) {
                            toast("取消了")
                        }
                    })
                    .show()

            }
            R.id.btn_dialog_succeed_toast -> {

                // 成功对话框
                TipsDialog.Builder(this)
                    .setIcon(TipsDialog.ICON_FINISH)
                    .setMessage("完成")
                    .show()

            }
            R.id.btn_dialog_fail_toast -> {

                // 失败对话框
                TipsDialog.Builder(this)
                    .setIcon(TipsDialog.ICON_ERROR)
                    .setMessage("错误")
                    .show()

            }
            R.id.btn_dialog_warn_toast -> {

                // 警告对话框
                TipsDialog.Builder(this)
                    .setIcon(TipsDialog.ICON_WARNING)
                    .setMessage("警告")
                    .show()

            }
            R.id.btn_dialog_wait -> {

                if (waitDialog == null) {
                    waitDialog = WaitDialog.Builder(this) // 消息文本可以不用填写
                        .setMessage(getString(R.string.common_loading))
                        .create()
                }
                waitDialog?.apply {
                    if (!isShowing) {
                        show()
                        postDelayed({ dismiss() }, 2000)
                    }
                }

            }
            R.id.btn_dialog_share -> {

                toast("记得改好第三方 AppID 和 Secret，否则会调不起来哦")
                val content = UMWeb("https://github.com/getActivity/AndroidProject-Kotlin")
                content.title = "Github"
                content.setThumb(UMImage(this, R.mipmap.launcher_ic))
                content.description = getString(R.string.app_name)

                // 分享对话框
                ShareDialog.Builder(this)
                    .setShareLink(content)
                    .setListener(object : OnShareListener {

                        override fun onSucceed(platform: Platform?) {
                            toast("分享成功")
                        }

                        override fun onError(platform: Platform?, t: Throwable) {
                            toast(t.message)
                        }

                        override fun onCancel(platform: Platform?) {
                            toast("分享取消")
                        }
                    })
                    .show()

            }
            R.id.btn_dialog_update -> {

                // 升级对话框
                UpdateDialog.Builder(this)
                    // 版本名
                    .setVersionName("5.2.0")
                    // 是否强制更新
                    .setForceUpdate(false)
                    // 更新日志
                    .setUpdateLog("到底更新了啥\n到底更新了啥\n到底更新了啥\n到底更新了啥\n到底更新了啥\n到底更新了啥")
                    // 下载 URL
                    .setDownloadUrl("https://dldir1.qq.com/weixin/android/weixin807android1920_arm64.apk")
                    // 文件 MD5
                    .setFileMd5("df2f045dfa854d8461d9cefe08b813c8")
                    .show()

            }
//            R.id.btn_dialog_custom -> {
//
//                // 自定义对话框
//                BaseDialog.Builder<BaseDialog.Builder<*>>(this)
//                    .setContentView(R.layout.custom_dialog)
//                    .setAnimStyle(AnimAction.ANIM_SCALE) //.setText(id, "我是预设置的文本")
//                    .setOnClickListener(R.id.btn_dialog_custom_ok, object : BaseDialog.OnClickListener<Button> {
//                        override fun onClick(dialog: BaseDialog?, view: Button) {
//                            dialog?.dismiss()
//                        }
//                    })
//                    .setOnCreateListener(object : BaseDialog.OnCreateListener {
//
//                        override fun onCreate(dialog: BaseDialog?) {
//                            toast("Dialog 创建了")
//                        }
//                    })
//                    .addOnShowListener(object : BaseDialog.OnShowListener {
//
//                        override fun onShow(dialog: BaseDialog?) {
//                            toast("Dialog 显示了")
//                        }
//                    })
//                    .addOnCancelListener(object : BaseDialog.OnCancelListener {
//
//                        override fun onCancel(dialog: BaseDialog?) {
//                            toast("Dialog 取消了")
//                        }
//                    })
//                    .addOnDismissListener(object : BaseDialog.OnDismissListener {
//
//                        override fun onDismiss(dialog: BaseDialog?) {
//                            toast("Dialog 销毁了")
//                        }
//                    })
//                    .setOnKeyListener(object : BaseDialog.OnKeyListener {
//
//                        override fun onKey(dialog: BaseDialog?, event: KeyEvent?): Boolean {
//                            toast("按键代码：" + event?.keyCode)
//                            return false
//                        }
//                    })
//                    .show()
//
//            }
            R.id.btn_dialog_multi -> {

                val dialog1: BaseDialog = MessageDialog.Builder(this)
                    .setTitle("温馨提示")
                    .setMessage("我是第一个弹出的对话框")
                    .setConfirm(getString(R.string.common_confirm))
                    .setCancel(getString(R.string.common_cancel))
                    .create()

                val dialog2: BaseDialog = MessageDialog.Builder(this)
                    .setTitle("温馨提示")
                    .setMessage("我是第二个弹出的对话框")
                    .setConfirm(getString(R.string.common_confirm))
                    .setCancel(getString(R.string.common_cancel))
                    .create()
                DialogManager.getInstance(this).addShow(dialog1)
                DialogManager.getInstance(this).addShow(dialog2)
            }
        }
    }

    override fun onRightClick(view: View) {
        // 菜单弹窗
        ListPopup.Builder(this)
            .setList("选择拍照", "选取相册")
            .addOnShowListener(object : BasePopupWindow.OnShowListener {

                override fun onShow(popupWindow: BasePopupWindow?) {
                    toast("PopupWindow 显示了")
                }
            })
            .addOnDismissListener(object : BasePopupWindow.OnDismissListener {

                override fun onDismiss(popupWindow: BasePopupWindow?) {
                    toast("PopupWindow 销毁了")
                }
            })
            .setListener(object : ListPopup.OnListener<String> {

                override fun onSelected(popupWindow: BasePopupWindow?, position: Int, data: String) {
                    toast("点击了：$data")
                }
            })
            .showAsDropDown(view)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        // 友盟回调
        UmengClient.onActivityResult(this, requestCode, resultCode, data)
    }
}