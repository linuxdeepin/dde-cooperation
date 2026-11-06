// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

package com.deepin.assistant.model

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.tencent.mmkv.MMKV

class SharedViewModel : ViewModel() {

    private val mmkv: MMKV = MMKV.mmkvWithID(PEER_INFO_MMKV_ID)

    private val name: MutableLiveData<String> = MutableLiveData("")
    private val ip: MutableLiveData<String> = MutableLiveData("")
    private val selfIp: MutableLiveData<String> = MutableLiveData("")
    private val action: MutableLiveData<Int> = MutableLiveData(-1)
    private val isScreenMirroring: MutableLiveData<Boolean> = MutableLiveData(false)

    fun name(): LiveData<String> = name
    fun ip(): LiveData<String> = ip

    fun selfIp(): LiveData<String> = selfIp

    fun action(): LiveData<Int> = action

    fun isScreenMirroring(): LiveData<Boolean> = isScreenMirroring

    fun updateInfo(auth: String, addr: String) {
        name.postValue(auth)
        ip.postValue(addr)
        // set value failed by diff thread
//        name.value = auth
//        ip.value = addr
        // 落盘最近一次连接的远端，进程重启后首页仍能显示真实信息
        mmkv.encode(KEY_PEER_NAME, auth)
        mmkv.encode(KEY_PEER_IP, addr)
    }

    fun doAction(req: Int) {
        action.postValue(req)
    }

    fun updateSelfIp(addr: String) {
        selfIp.postValue(addr)
    }

    fun setScreenMirroring(req: Boolean) {
        isScreenMirroring.postValue(req)
    }

    init {
        // Setting default value，已连接过则恢复上次连接的远端信息
        name.value = mmkv.decodeString(KEY_PEER_NAME, "")
        ip.value = mmkv.decodeString(KEY_PEER_IP, "")
        action.value = -1
        isScreenMirroring.value = false
    }

    companion object {
        private const val PEER_INFO_MMKV_ID = "peer_device_info"
        private const val KEY_PEER_NAME = "peer_device_name"
        private const val KEY_PEER_IP = "peer_device_ip"
    }
}