// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

package com.deepin.assistant.services;

public class Constants {
    /*
        user settings
     */
    public static final String PREFS_KEY_SETTINGS_PORT = "settings_port";
    public static final String PREFS_KEY_SETTINGS_PASSWORD = "settings_password" ;
    public static final String PREFS_KEY_SETTINGS_START_ON_BOOT = "settings_start_on_boot" ;
    public static final String PREFS_KEY_SETTINGS_START_ON_BOOT_DELAY = "settings_start_on_boot_delay" ;
    public static final String PREFS_KEY_SETTINGS_SCALING = "settings_scaling" ;
    public static final String PREFS_KEY_SETTINGS_VIEW_ONLY = "settings_view_only" ;
    public static final String PREFS_KEY_SETTINGS_SHOW_POINTERS = "settings_show_pointers" ;
    public static final String PREFS_KEY_SETTINGS_ACCESS_KEY = "settings_access_key";
    public static final String PREFS_KEY_SETTINGS_FILE_TRANSFER = "settings_file_transfer";

    /*
        persisted runtime values shared between components
     */
    public static final String PREFS_KEY_SERVER_LAST_SCALING = "server_last_scaling" ;
    public static final String PREFS_KEY_INPUT_LAST_ENABLED = "input_last_enabled" ;
}
