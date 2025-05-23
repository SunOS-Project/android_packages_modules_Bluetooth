/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.bluetooth.btservice;

import android.app.Application;
import android.util.Log;

import com.android.bluetooth.btservice.storage.BluetoothDatabaseU2VMigration;

public class AdapterApp extends Application {
    private static final String TAG = "BluetoothAdapterApp";
    // For Debugging only
    private static int sRefCount = 0;

    public AdapterApp() {
        super();
        synchronized (AdapterApp.class) {
            sRefCount++;
            Log.d(TAG, "REFCOUNT: Constructed " + this + " Instance Count = " + sRefCount);
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "onCreate");

        try {
            BluetoothDatabaseU2VMigration.run(this);
        } catch (Exception e) {
            Log.e(TAG, "U2VMigration failure: ", e);
        }

        try {
            DataMigration.run(this);
        } catch (Exception e) {
            Log.e(TAG, "Migration failure: ", e);
        }
    }

    @Override
    protected void finalize() {
        synchronized (AdapterApp.class) {
            sRefCount--;
            Log.d(TAG, "REFCOUNT: Finalized: " + this + ", Instance Count = " + sRefCount);
        }
    }
}
