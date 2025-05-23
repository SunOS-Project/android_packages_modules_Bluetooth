/*
 * Copyright 2022 The Android Open Source Project
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

/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package com.android.bluetooth.gatt;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.le.DistanceMeasurementParams;
import android.bluetooth.le.IDistanceMeasurementCallback;
import android.bluetooth.le.ChannelSoundingParams;
import android.annotation.FlaggedApi;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.SystemApi;
import android.os.Handler;
import android.os.Looper;

import java.util.Objects;
import java.util.UUID;

/** Manages information of apps that registered distance measurement */
class DistanceMeasurementTracker {
    private static final String TAG = "DistanceMeasurementTracker";

    final DistanceMeasurementManager mManager;
    final BluetoothDevice mDevice;
    final String mIdentityAddress;
    final UUID mUuid;
    final int mInterval; // Report interval in ms
    final int mDuration; // Report duration in s
    final int mMethod;
    final int mFrequency;
    final IDistanceMeasurementCallback mCallback;
    boolean mStarted = false;
    private Handler mHandler;
    private ChannelSoundingParams mChannelSoundingParams = null;

    DistanceMeasurementTracker(
            DistanceMeasurementManager manager,
            DistanceMeasurementParams params,
            String identityAddress,
            UUID uuid,
            int interval, int Frequency,
            IDistanceMeasurementCallback callback) {
        mManager = manager;
        mDevice = params.getDevice();
        mIdentityAddress = identityAddress;
        mUuid = uuid;
        mInterval = interval;
        mDuration = params.getDurationSeconds();
        mMethod = params.getMethodId();
        mCallback = callback;
        mFrequency = Frequency;
	mChannelSoundingParams = params.getChannelSoundingParams();
    }

    void startTimer(Looper looper) {
        mHandler = new Handler(looper);
        mHandler.postDelayed(
                new Runnable() {
                    @Override
                    public void run() {
                        mManager.stopDistanceMeasurement(mUuid, mDevice, mMethod, true);
                    }
                },
                mDuration * 1000L);
    }

    void cancelTimer() {
        if (mHandler != null) {
            mHandler.removeCallbacksAndMessages(null);
        }
    }

    public boolean equals(UUID uuid, String identityAddress) {
        if (!Objects.equals(mUuid, uuid)) {
            return false;
        }
        if (!Objects.equals(mIdentityAddress, identityAddress)) {
            return false;
        }
        return true;
    }

    @Override
    public boolean equals(Object o) {
        if (o == null) return false;

        if (!(o instanceof DistanceMeasurementTracker)) return false;

        final DistanceMeasurementTracker u = (DistanceMeasurementTracker) o;

        if (!Objects.equals(mIdentityAddress, u.mIdentityAddress)) {
            return false;
        }

        if (!Objects.equals(mUuid, u.mUuid)) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        return Objects.hash(mIdentityAddress, mUuid);
    }
    @SystemApi
    public @Nullable ChannelSoundingParams getChannelSoundingParams() {
        return mChannelSoundingParams;
    }
}
