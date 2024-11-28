/*
 * DroidVNC-NG activity for (automatically) requesting media projection
 * permission.
 *
 * Author: Christian Beier <info@christianbeier.net>
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

package net.christianbeier.droidvnc_ng;

import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.PreferenceManager;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.media.projection.MediaProjectionManager;
import android.os.Bundle;
import android.util.Log;

public class MediaProjectionRequestActivity extends AppCompatActivity {

    private static final String TAG = "MPRequestActivity";
    private static final int REQUEST_MEDIA_PROJECTION = 42;
    static final String EXTRA_UPGRADING_FROM_FALLBACK_SCREEN_CAPTURE = "upgrading_from_fallback_screen_capture";
    private boolean mIsUpgradingFromFallbackScreenCapture;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mIsUpgradingFromFallbackScreenCapture = getIntent().getBooleanExtra(EXTRA_UPGRADING_FROM_FALLBACK_SCREEN_CAPTURE, false);

        MediaProjectionManager mMediaProjectionManager = (MediaProjectionManager) getSystemService(Context.MEDIA_PROJECTION_SERVICE);

        if(!mIsUpgradingFromFallbackScreenCapture) {
            // ask for MediaProjection right away
            Log.i(TAG, "Requesting confirmation");
            // This initiates a prompt dialog for the user to confirm screen projection.
            startActivityForResult(
                    mMediaProjectionManager.createScreenCaptureIntent(),
                    REQUEST_MEDIA_PROJECTION);
        } else {
            // show user info dialog before asking
            new AlertDialog.Builder(this)
                    .setCancelable(false)
                    .setTitle(R.string.mediaprojection_request_activity_fallback_screen_capture_title)
                    .setMessage(R.string.mediaprojection_request_activity_fallback_screen_capture_msg)
                    .setPositiveButton(R.string.yes, (dialog, which) -> {
                        // This initiates a prompt dialog for the user to confirm screen projection.
                        startActivityForResult(
                                mMediaProjectionManager.createScreenCaptureIntent(),
                                REQUEST_MEDIA_PROJECTION);
                    })
                    .setNegativeButton(getString(R.string.no), (dialog, which) -> finish())
                    .show();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == REQUEST_MEDIA_PROJECTION) {
            if (resultCode != Activity.RESULT_OK)
                Log.i(TAG, "User cancelled");
            else
                Log.i(TAG, "User acknowledged");

            Intent intent = new Intent(this, MainService.class);
            intent.setAction(MainService.ACTION_HANDLE_MEDIA_PROJECTION_RESULT);
            intent.putExtra(MainService.EXTRA_ACCESS_KEY, PreferenceManager.getDefaultSharedPreferences(this).getString(Constants.PREFS_KEY_SETTINGS_ACCESS_KEY, new Defaults(this).getAccessKey()));
            intent.putExtra(MainService.EXTRA_MEDIA_PROJECTION_RESULT_CODE, resultCode);
            intent.putExtra(MainService.EXTRA_MEDIA_PROJECTION_RESULT_DATA, data);
            intent.putExtra(MainService.EXTRA_MEDIA_PROJECTION_UPGRADING_FROM_FALLBACK_SCREEN_CAPTURE, mIsUpgradingFromFallbackScreenCapture);
            startService(intent);
            finish();
        }
    }

}