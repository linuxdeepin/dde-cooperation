// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * DroidVNC-NG main activity.
 *
 * Author: Christian Beier <info@christianbeier.net
 *
 * Copyright (C) 2020 Kitchen Armor.
 *
 * You can redistribute and/or modify this program under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place Suite 330, Boston, MA 02111-1307, USA.
 */
package com.deepin.assistant.ui.activity

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import android.text.Editable
import android.text.SpannableString
import android.text.Spanned
import android.text.TextWatcher
import android.text.method.LinkMovementMethod
import android.text.style.ClickableSpan
import android.util.Log
import android.util.Pair
import android.view.View
import android.view.View.OnFocusChangeListener
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.preference.PreferenceManager
import com.deepin.assistant.R
import com.deepin.assistant.app.AppFragment
import com.deepin.assistant.ui.fragment.HomeFragment
import com.deepin.cooperation.CooperationListener
import com.deepin.cooperation.JniCooperation
import com.hjq.permissions.Permission.POST_NOTIFICATIONS
import com.deepin.assistant.services.Constants
import com.deepin.assistant.services.Defaults
import com.deepin.assistant.services.InputRequestActivity
import com.deepin.assistant.services.InputService
import com.deepin.assistant.services.MainService
import com.deepin.assistant.services.MediaProjectionService

class MainActivity : AppCompatActivity() {
    private var mButtonToggle: Button? = null
    private var mAddress: TextView? = null
    private var mIsMainServiceRunning = false
    private var mMainServiceBroadcastReceiver: BroadcastReceiver? = null
    private var mOutgoingConnectionWaitDialog: AlertDialog? = null
    private var mLastMainServiceRequestId: String? = null
    private var mLastReverseHost: String? = null
    private var mLastReversePort = 0
    private var mLastRepeaterHost: String? = null
    private var mLastRepeaterPort = 0
    private var mLastRepeaterId: String? = null
    private var mDefaults: Defaults? = null

    private var mTestBtn: Button? = null
    private var mProjectionBtn: Button? = null
    private var mCooperation: JniCooperation? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.main_activity)

        val prefs = PreferenceManager.getDefaultSharedPreferences(this)
        mDefaults = Defaults(this)

        mButtonToggle = findViewById<Button>(R.id.toggle)
        mButtonToggle?.setOnClickListener(View.OnClickListener { view: View? ->
//            val intent = Intent(this@MainActivity, MainService::class.java)
//            intent.putExtra(
//                MainService.EXTRA_PORT,
//                prefs.getInt(Constants.PREFS_KEY_SETTINGS_PORT, mDefaults!!.port)
//            )
//            intent.putExtra(
//                MainService.EXTRA_PASSWORD,
//                prefs.getString(Constants.PREFS_KEY_SETTINGS_PASSWORD, mDefaults!!.password)
//            )
//            intent.putExtra(
//                MainService.EXTRA_FILE_TRANSFER,
//                prefs.getBoolean(
//                    Constants.PREFS_KEY_SETTINGS_FILE_TRANSFER,
//                    mDefaults!!.fileTransfer
//                )
//            )
//            intent.putExtra(
//                MainService.EXTRA_VIEW_ONLY,
//                prefs.getBoolean(Constants.PREFS_KEY_SETTINGS_VIEW_ONLY, mDefaults!!.viewOnly)
//            )
//            intent.putExtra(
//                MainService.EXTRA_SHOW_POINTERS,
//                prefs.getBoolean(
//                    Constants.PREFS_KEY_SETTINGS_SHOW_POINTERS,
//                    mDefaults!!.showPointers
//                )
//            )
//            intent.putExtra(
//                MainService.EXTRA_SCALING,
//                prefs.getFloat(Constants.PREFS_KEY_SETTINGS_SCALING, mDefaults!!.scaling)
//            )
//            intent.putExtra(
//                MainService.EXTRA_ACCESS_KEY,
//                prefs.getString(Constants.PREFS_KEY_SETTINGS_ACCESS_KEY, mDefaults!!.accessKey)
//            )
//            if (mIsMainServiceRunning) {
//                intent.setAction(MainService.ACTION_STOP)
//            } else {
//                intent.setAction(MainService.ACTION_START)
//            }
//            mButtonToggle?.setEnabled(false)
//            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
//                startForegroundService(intent)
//            } else {
//                startService(intent)
//            }
        })

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
        
        mTestBtn = findViewById<Button>(R.id.test)
        mTestBtn?.setOnClickListener(View.OnClickListener { view: View? ->
//            val targetip = "10.8.11.52"
//            val port = 51598
            mCooperation?.connectRemote("10.8.11.52", 51598, "515616")
        })

        mProjectionBtn = findViewById<Button>(R.id.projection)
        mProjectionBtn?.setOnClickListener(View.OnClickListener { view: View? ->
            mCooperation?.sendProjection("Vivo", 5900);
        })

        mAddress = findViewById<TextView>(R.id.address)

        val port = findViewById<EditText>(R.id.settings_port)
        if (prefs.getInt(Constants.PREFS_KEY_SETTINGS_PORT, mDefaults!!.port) < 0) {
            port.setHint(R.string.main_activity_settings_port_not_listening)
        } else {
            port.setText(
                prefs.getInt(Constants.PREFS_KEY_SETTINGS_PORT, mDefaults!!.port).toString()
            )
        }
        port.addTextChangedListener(object : TextWatcher {
            override fun beforeTextChanged(charSequence: CharSequence, i: Int, i1: Int, i2: Int) {
            }

            override fun onTextChanged(charSequence: CharSequence, i: Int, i1: Int, i2: Int) {
                try {
                    val ed = prefs.edit()
                    ed.putInt(Constants.PREFS_KEY_SETTINGS_PORT, charSequence.toString().toInt())
                    ed.apply()
                } catch (e: NumberFormatException) {
                    // nop
                }
            }

            override fun afterTextChanged(editable: Editable) {
                if (port.text.length == 0) {
                    // hint that not listening
                    port.setHint(R.string.main_activity_settings_port_not_listening)
                    // and set default
                    val ed = prefs.edit()
                    ed.putInt(Constants.PREFS_KEY_SETTINGS_PORT, -1)
                    ed.apply()
                }
            }
        })
        port.onFocusChangeListener = OnFocusChangeListener { v: View?, hasFocus: Boolean ->
            // move cursor to end of text
            port.setSelection(port.text.length)
        }

        mMainServiceBroadcastReceiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context, intent: Intent) {
                if (MainService.ACTION_START == intent.action) {
                    if (intent.getBooleanExtra(MainService.EXTRA_REQUEST_SUCCESS, false)) {
                        // was a successful START requested by anyone (but sent by MainService, as the receiver is not exported!)
                        Log.d(TAG, "got MainService started success event")
                        onServerStarted()
                    } else {
                        // was a failed START requested by anyone (but sent by MainService, as the receiver is not exported!)
                        Log.d(TAG, "got MainService started fail event")
                        // if it was, by us, re-enable the button!
                        mButtonToggle?.setEnabled(true)
                        // let focus stay on button
                        mButtonToggle?.requestFocus()
                    }
                }

                if (MainService.ACTION_STOP == intent.action && (intent.getBooleanExtra(
                        MainService.EXTRA_REQUEST_SUCCESS,
                        true
                    ))
                ) {
                    // was a successful STOP requested by anyone (but sent by MainService, as the receiver is not exported!)
                    // or a STOP without any extras
                    Log.d(TAG, "got MainService stopped event")
                    onServerStopped()
                }

                if (MainService.ACTION_CONNECT_REVERSE == intent.action && mLastMainServiceRequestId != null && mLastMainServiceRequestId == intent.getStringExtra(
                        MainService.EXTRA_REQUEST_ID
                    )
                ) {
                    // was a CONNECT_REVERSE requested by us
                    if (intent.getBooleanExtra(MainService.EXTRA_REQUEST_SUCCESS, false)) {
                        Toast.makeText(
                            this@MainActivity,
                            getString(
                                R.string.main_activity_reverse_vnc_success,
                                mLastReverseHost,
                                mLastReversePort
                            ),
                            Toast.LENGTH_LONG
                        )
                            .show()
                        val ed = prefs.edit()
                        ed.putString(
                            PREFS_KEY_REVERSE_VNC_LAST_HOST,
                            "$mLastReverseHost:$mLastReversePort"
                        )
                        ed.apply()
                    } else Toast.makeText(
                        this@MainActivity,
                        getString(
                            R.string.main_activity_reverse_vnc_fail,
                            mLastReverseHost,
                            mLastReversePort
                        ),
                        Toast.LENGTH_LONG
                    )
                        .show()

                    // reset this
                    mLastMainServiceRequestId = null
                    try {
                        mOutgoingConnectionWaitDialog!!.dismiss()
                    } catch (ignored: NullPointerException) {
                    }
                }

                if (MainService.ACTION_CONNECT_REPEATER == intent.action && mLastMainServiceRequestId != null && mLastMainServiceRequestId == intent.getStringExtra(
                        MainService.EXTRA_REQUEST_ID
                    )
                ) {
                    // was a CONNECT_REPEATER requested by us
                    if (intent.getBooleanExtra(MainService.EXTRA_REQUEST_SUCCESS, false)) {
                        Toast.makeText(
                            this@MainActivity,
                            getString(
                                R.string.main_activity_repeater_vnc_success,
                                mLastRepeaterHost,
                                mLastRepeaterPort,
                                mLastRepeaterId
                            ),
                            Toast.LENGTH_LONG
                        )
                            .show()
                        val ed = prefs.edit()
                        ed.putString(
                            PREFS_KEY_REPEATER_VNC_LAST_HOST,
                            "$mLastRepeaterHost:$mLastRepeaterPort"
                        )
                        ed.putString(
                            PREFS_KEY_REPEATER_VNC_LAST_ID,
                            mLastRepeaterId
                        )
                        ed.apply()
                    } else Toast.makeText(
                        this@MainActivity,
                        getString(
                            R.string.main_activity_repeater_vnc_fail,
                            mLastRepeaterHost,
                            mLastRepeaterPort,
                            mLastRepeaterId
                        ),
                        Toast.LENGTH_LONG
                    )
                        .show()

                    // reset this
                    mLastMainServiceRequestId = null
                    try {
                        mOutgoingConnectionWaitDialog!!.dismiss()
                    } catch (ignored: NullPointerException) {
                    }
                }
            }
        }
        val filter = IntentFilter()
        filter.addAction(MainService.ACTION_START)
        filter.addAction(MainService.ACTION_STOP)
        filter.addAction(MainService.ACTION_CONNECT_REVERSE)
        filter.addAction(MainService.ACTION_CONNECT_REPEATER)
        // register the receiver as NOT_EXPORTED so it only receives broadcasts sent by MainService,
        // not a malicious fake broadcaster like
        // `adb shell am broadcast -a net.christianbeier.droidvnc_ng.ACTION_STOP --ez net.christianbeier.droidvnc_ng.EXTRA_REQUEST_SUCCESS true`
        // for instance
        // androidx 1.8.0 以上高版本，SDK34
//        ContextCompat.registerReceiver(
//            this,
//            mMainServiceBroadcastReceiver,
//            filter,
//            ContextCompat.RECEIVER_NOT_EXPORTED
//        )
        registerReceiver(mMainServiceBroadcastReceiver, filter)

        // setup UI initial state
        if (MainService.isServerActive()) {
            Log.d(TAG, "Found server to be started")
            onServerStarted()
        } else {
            Log.d(TAG, "Found server to be stopped")
            onServerStopped()
        }
    }

    @SuppressLint("SetTextI18n")
    override fun onResume() {
        super.onResume()

        /*
            Update Input permission display.
         */
        val inputStatus = findViewById<TextView>(R.id.permission_status_input)
        if (InputService.isConnected) {
            inputStatus.setText(R.string.main_activity_granted)
            inputStatus.setTextColor(getColor(R.color.granted))
        } else {
            inputStatus.setText(R.string.main_activity_denied)
            inputStatus.setTextColor(getColor(R.color.denied))
        }


        /*
            Update File Access permission display. Only show on < Android 13.
         */
        if (Build.VERSION.SDK_INT < 33) {
            val fileAccessStatus = findViewById<TextView>(R.id.permission_status_file_access)
            if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
                fileAccessStatus.setText(R.string.main_activity_granted)
                fileAccessStatus.setTextColor(getColor(R.color.granted))
            } else {
                fileAccessStatus.setText(R.string.main_activity_denied)
                fileAccessStatus.setTextColor(getColor(R.color.denied))
            }
        } else {
            findViewById<View>(R.id.permission_row_file_access).visibility = View.GONE
        }


        /*
            Update Notification permission display. Only show on >= Android 13.
         */
        if (Build.VERSION.SDK_INT >= 33) {
            val notificationStatus = findViewById<TextView>(R.id.permission_status_notification)
            if (checkSelfPermission(POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED) {
                notificationStatus.setText(R.string.main_activity_granted)
                notificationStatus.setTextColor(getColor(R.color.granted))
            } else {
                notificationStatus.setText(R.string.main_activity_denied)
                notificationStatus.setTextColor(getColor(R.color.denied))
            }
            notificationStatus.setOnClickListener { view: View? ->
                val intent = Intent(
                    Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Uri.parse(
                        "package:$packageName"
                    )
                )
                startActivity(intent)
            }
        } else {
            findViewById<View>(R.id.permission_row_notification).visibility = View.GONE
        }


        /*
           Update Screen Capturing permission display.
        */
        val screenCapturingStatus = findViewById<TextView>(R.id.permission_status_screen_capturing)
        if (MediaProjectionService.isMediaProjectionEnabled()) {
            screenCapturingStatus.setText(R.string.main_activity_granted)
            screenCapturingStatus.setTextColor(getColor(R.color.granted))
        }
        if (!MediaProjectionService.isMediaProjectionEnabled()) {
            screenCapturingStatus.setText(R.string.main_activity_denied)
            screenCapturingStatus.setTextColor(getColor(R.color.denied))
        }
        if (!MediaProjectionService.isMediaProjectionEnabled() && InputService.isTakingScreenShots) {
            screenCapturingStatus.setText(R.string.main_activity_fallback)
            screenCapturingStatus.setTextColor(getColor(R.color.fallback))
        }

        /*
            Update start-on-boot permission display. Only show on >= Android 10.
         */
        if (Build.VERSION.SDK_INT >= 30) {
            val startOnBootStatus = findViewById<TextView>(R.id.permission_status_start_on_boot)
            if (PreferenceManager.getDefaultSharedPreferences(this)
                    .getBoolean(Constants.PREFS_KEY_SETTINGS_START_ON_BOOT, mDefaults!!.startOnBoot)
                && InputService.isConnected) {
                startOnBootStatus.setText(R.string.main_activity_granted)
                startOnBootStatus.setTextColor(getColor(R.color.granted))
            } else {
                startOnBootStatus.setText(R.string.main_activity_denied)
                startOnBootStatus.setTextColor(getColor(R.color.denied))
                // wire this up only for denied status
                startOnBootStatus.setOnClickListener { view: View? ->
                    if (PreferenceManager.getDefaultSharedPreferences(
                            this
                        ).getBoolean(
                            Constants.PREFS_KEY_SETTINGS_START_ON_BOOT,
                            mDefaults!!.startOnBoot
                        )
                    ) {
                        val inputRequestIntent = Intent(this, InputRequestActivity::class.java)
                        inputRequestIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                        inputRequestIntent.putExtra(
                            InputRequestActivity.EXTRA_DO_NOT_START_MAIN_SERVICE_ON_FINISH,
                            true
                        )
                        startActivity(inputRequestIntent)
                    }
                }
            }
        } else {
            findViewById<View>(R.id.permission_row_start_on_boot).visibility = View.GONE
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.d(TAG, "onDestroy")
        unregisterReceiver(mMainServiceBroadcastReceiver)
    }

    private fun onServerStarted() {
        mButtonToggle!!.post {
            mButtonToggle!!.setText(R.string.stop)
            mButtonToggle!!.isEnabled = true
            // let focus stay on button
            mButtonToggle!!.requestFocus()
        }

        if (MainService.getPort() >= 0) {
            val spans = HashMap<ClickableSpan, Pair<Int, Int>>()
            // uhh there must be a nice functional way for this
            val hosts = MainService.getIPv4s()
            val sb = StringBuilder()
            sb.append(getString(R.string.main_activity_address)).append(" ")
            for (i in hosts.indices) {
                val host = hosts[i]
                sb.append(host).append(":").append(MainService.getPort())
            }
            // done with string and span creation, put it all together
            val spannableString = SpannableString(sb)
            spans.forEach { (clickableSpan: ClickableSpan?, startEnd: Pair<Int, Int>) ->
                spannableString.setSpan(
                    clickableSpan,
                    startEnd.first,
                    startEnd.second,
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE
                )
            }
            mAddress!!.post {
                mAddress!!.text = spannableString
                mAddress!!.movementMethod = LinkMovementMethod.getInstance()
            }
        } else {
            mAddress?.post { mAddress!!.setText(R.string.main_activity_not_listening) }
        }

        // indicate that changing these settings does not have an effect when the server is running
        findViewById<View>(R.id.settings_port).isEnabled = false
        findViewById<View>(R.id.settings_password).isEnabled = false
        findViewById<View>(R.id.settings_access_key).isEnabled = false

        mIsMainServiceRunning = true
    }

    private fun onServerStopped() {
        mButtonToggle!!.post {
            mButtonToggle!!.setText(R.string.start)
            mButtonToggle!!.isEnabled = true
            // let focus stay on button
            mButtonToggle!!.requestFocus()
        }
        mAddress!!.post { mAddress!!.text = "" }

        // indicate that changing these settings does have an effect when the server is stopped
        findViewById<View>(R.id.settings_port).isEnabled = true
        findViewById<View>(R.id.settings_password).isEnabled = true
        findViewById<View>(R.id.settings_access_key).isEnabled = true

        mIsMainServiceRunning = false
    }

    companion object {
        private const val TAG = "MainActivity"
        private const val PREFS_KEY_REVERSE_VNC_LAST_HOST = "reverse_vnc_last_host"
        private const val PREFS_KEY_REPEATER_VNC_LAST_HOST = "repeater_vnc_last_host"
        private const val PREFS_KEY_REPEATER_VNC_LAST_ID = "repeater_vnc_last_id"

        private const val INTENT_KEY_IN_FRAGMENT_INDEX: String = "fragmentIndex"
        private const val INTENT_KEY_IN_FRAGMENT_CLASS: String = "fragmentClass"

        @JvmOverloads
        fun start(context: Context, fragmentClass: Class<out AppFragment<*>?>? = HomeFragment::class.java) {
            val intent = Intent(context, MainActivity::class.java)
            intent.putExtra(MainActivity.INTENT_KEY_IN_FRAGMENT_CLASS, fragmentClass)
            if (context !is Activity) {
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            context.startActivity(intent)
        }
    }
}