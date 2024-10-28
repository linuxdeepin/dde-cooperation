package net.christianbeier.droidvnc_ng

import android.util.Log
import java.nio.ByteBuffer

class JniDroidVnc {
//    private val TAG = "JniDroidVnc"
//
//    private var mClientListener: DroidManager.ClientListener? = null
//    private var mInputListener: DroidManager.InputListener? = null
//
//    private val instance: JniDroidVnc? = null
//
//    fun registerClientListener(listener: DroidManager.ClientListener) {
//        this.mClientListener = listener
//    }
//
//    fun registerInputListener(listener: DroidManager.InputListener) {
//        this.mInputListener = listener
//    }

    external fun vncStartServer(width: Int, height: Int, port: Int, desktopName: String,
        password: String, httpRootDir: String
    ): Boolean

    external fun vncStopServer(): Boolean
    external fun vncIsActive(): Boolean
    external fun vncConnectReverse(host: String, port: Int): Long
    external fun vncConnectRepeater(host: String, port: Int, repeaterIdentifier: String): Long

    external fun vncNewFramebuffer(width: Int, height: Int): Boolean
    external fun vncUpdateFramebuffer(buf: ByteBuffer?): Boolean
    external fun vncGetFramebufferWidth(): Int
    external fun vncGetFramebufferHeight(): Int

    companion object {
        private val TAG = "JniDroidVnc"

        private var mClientListener: DroidManager.ClientListener? = null
        private var mInputListener: DroidManager.InputListener? = null

        // Used to load the 'droidvnc-ng' library on application startup.
        init {
            System.loadLibrary("droidvnc-ng")
        }

        @JvmStatic
        fun registerClientListener(listener: DroidManager.ClientListener) {
            this.mClientListener = listener
        }

        @JvmStatic
        fun registerInputListener(listener: DroidManager.InputListener) {
            this.mInputListener = listener
        }

        @Suppress("unused")
        @JvmStatic
        fun callbackClientConnected(client: Long) {
            try {
                mClientListener?.onClientConnected(client)
            } catch (e: Exception) {
                // instance probably null
                Log.e(TAG, "callbackClientConnected: error: $e")
            }
        }

        @Suppress("unused")
        @JvmStatic
        fun callbackClientDisconnected(client: Long) {
            try {
                mClientListener?.onClientDisconnected(client)
            } catch (e: Exception) {
                // instance probably null
                Log.e(TAG, "callbackClientDisconnected: error: $e")
            }
        }

        @Suppress("unused")
        @JvmStatic
        fun callbackPointerEvent(buttonMask: Int, x: Int, y: Int, client: Long) {
            try {
                mInputListener?.onInputPointerEvent(buttonMask, x, y, client)
            } catch (e: Exception) {
                // instance probably null
                Log.e(TAG, "callbackPointerEvent: error: $e")
            }
        }

        @Suppress("unused")
        @JvmStatic
        fun callbackKeyEvent(down: Int, keysym: Long, client: Long) {
            try {
                mInputListener?.onInputKeyEvent(down, keysym, client)
            } catch (e: Exception) {
                // instance probably null
                Log.e(TAG, "callbackKeyEvent: error: $e")
            }
        }

        @Suppress("unused")
        @JvmStatic
        fun callbackCutText(text: String?, client: Long) {
            try {
                mInputListener?.onInputCutText(text, client)
            } catch (e: Exception) {
                // instance probably null
                Log.e(TAG, "callbackCutText: error: $e")
            }
        }
    }
}