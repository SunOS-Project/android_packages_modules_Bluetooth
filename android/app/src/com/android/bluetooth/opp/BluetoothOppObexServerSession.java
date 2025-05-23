/*
 * Copyright (c) 2008-2009, Motorola, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the name of the Motorola, Inc. nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package com.android.bluetooth.opp;

import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothProtoEnums;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.SystemClock;
import android.util.Log;
import android.webkit.MimeTypeMap;

import com.android.bluetooth.BluetoothMethodProxy;
import com.android.bluetooth.BluetoothMetricsProto;
import com.android.bluetooth.BluetoothObexTransport;
import com.android.bluetooth.BluetoothStatsLog;
import com.android.bluetooth.Utils;
import com.android.bluetooth.btservice.MetricsLogger;
import com.android.bluetooth.content_profiles.ContentProfileErrorReportUtils;
import com.android.obex.HeaderSet;
import com.android.obex.ObexTransport;
import com.android.obex.Operation;
import com.android.obex.ResponseCodes;
import com.android.obex.ServerRequestHandler;
import com.android.obex.ServerSession;

import com.google.common.annotations.VisibleForTesting;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Arrays;

/** This class runs as an OBEX server */
// Next tag value for ContentProfileErrorReportUtils.report(): 15
public class BluetoothOppObexServerSession extends ServerRequestHandler
        implements BluetoothOppObexSession {

    private static final String TAG = "BtOppObexServer";

    @VisibleForTesting public ObexTransport mTransport;

    @VisibleForTesting public Context mContext;

    @VisibleForTesting public Handler mCallback = null;

    /* status when server is blocking for user/auto confirmation */
    @VisibleForTesting public boolean mServerBlocking = true;

    /* the current transfer info */
    @VisibleForTesting public BluetoothOppShareInfo mInfo;

    /* info id when we insert the record */
    private int mLocalShareInfoId;

    @VisibleForTesting public int mAccepted = BluetoothShare.USER_CONFIRMATION_PENDING;

    private boolean mInterrupted = false;

    @VisibleForTesting public ServerSession mSession;

    private long mTimestamp;

    @VisibleForTesting BluetoothOppReceiveFileInfo mFileInfo;

    private WakeLock mPartialWakeLock;

    @VisibleForTesting boolean mTimeoutMsgSent = false;

    @VisibleForTesting public BluetoothOppService mBluetoothOppService;

    private int mNumFilesAttemptedToReceive;

    public BluetoothOppObexServerSession(
            Context context, ObexTransport transport, BluetoothOppService service) {
        mContext = context;
        mTransport = transport;
        mBluetoothOppService = service;
        PowerManager pm = mContext.getSystemService(PowerManager.class);
        mPartialWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);
        mPartialWakeLock.setReferenceCounted(false);
    }

    @Override
    public void unblock() {
        mServerBlocking = false;
    }

    /**
     * Called when connection is accepted from remote, to retrieve the first Header then wait for
     * user confirmation
     */
    public void preStart() {
        try {
            Log.d(TAG, "Create ServerSession with transport " + mTransport.toString());
            mSession = new ServerSession(mTransport, this, null);
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.OPP,
                    BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    0);
            Log.e(TAG, "Create server session error" + e);
        }
    }

    /** Called from BluetoothOppTransfer to start the "Transfer" */
    @Override
    public void start(Handler handler, int numShares) {
        Log.d(TAG, "Start!");
        mCallback = handler;
    }

    /**
     * Called from BluetoothOppTransfer to cancel the "Transfer" Otherwise, server should end by
     * itself.
     */
    @Override
    public void stop() {
        /*
         * TODO now we implement in a tough way, just close the socket.
         * maybe need nice way
         */
        Log.d(TAG, "Stop!");
        mInterrupted = true;
        if (mSession != null) {
            try {
                mSession.close();
                mTransport.close();
            } catch (IOException e) {
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.OPP,
                        BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                        1);
                Log.e(TAG, "close mTransport error" + e);
            }
        }
        mCallback = null;
        mSession = null;
    }

    @Override
    public void addShare(BluetoothOppShareInfo info) {
        Log.d(TAG, "addShare for id " + info.mId);
        mInfo = info;
        mFileInfo = processShareInfo();
    }

    @Override
    public int onPut(Operation op) {
        Log.d(TAG, "onPut " + op.toString());

        /* For multiple objects, reject further objects after the user denies the first one */
        if (mAccepted == BluetoothShare.USER_CONFIRMATION_DENIED) {
            return ResponseCodes.OBEX_HTTP_FORBIDDEN;
        }

        String destination;
        if (mTransport instanceof BluetoothObexTransport) {
            destination = ((BluetoothObexTransport) mTransport).getRemoteAddress();
        } else {
            destination = "FF:FF:FF:00:00:00";
        }
        boolean isAcceptlisted =
                BluetoothOppManager.getInstance(mContext).isAcceptlisted(destination);

        HeaderSet request;
        String name, mimeType;
        Long length;
        try {
            request = op.getReceivedHeader();
            Constants.logHeader(request);
            name = (String) request.getHeader(HeaderSet.NAME);
            length = (Long) request.getHeader(HeaderSet.LENGTH);
            mimeType = (String) request.getHeader(HeaderSet.TYPE);
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.OPP,
                    BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    2);
            Log.e(TAG, "onPut: getReceivedHeaders error " + e);
            return ResponseCodes.OBEX_HTTP_BAD_REQUEST;
        }

        if (length == 0) {
            Log.w(TAG, "length is 0, reject the transfer");
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.OPP,
                    BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                    3);
            return ResponseCodes.OBEX_HTTP_LENGTH_REQUIRED;
        }

        if (name == null || name.isEmpty()) {
            Log.w(TAG, "name is null or empty, reject the transfer");
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.OPP,
                    BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                    4);
            return ResponseCodes.OBEX_HTTP_BAD_REQUEST;
        }

        // First we look for the mime type in the Android map
        String extension, type;
        int dotIndex = name.lastIndexOf(".");
        if (dotIndex < 0 && mimeType == null) {
            Log.w(TAG, "There is no file extension or mime type, reject the transfer");
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.OPP,
                    BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                    5);
            return ResponseCodes.OBEX_HTTP_BAD_REQUEST;
        } else {
            extension = name.substring(dotIndex + 1).toLowerCase();
            MimeTypeMap map = MimeTypeMap.getSingleton();
            type = map.getMimeTypeFromExtension(extension);
            Log.v(TAG, "Mimetype guessed from extension " + extension + " is " + type);
            if (type != null) {
                mimeType = type;
            } else {
                if (mimeType == null) {
                    Log.w(TAG, "Can't get mimetype, reject the transfer");
                    ContentProfileErrorReportUtils.report(
                            BluetoothProfile.OPP,
                            BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                            BluetoothStatsLog
                                    .BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                            6);
                    return ResponseCodes.OBEX_HTTP_UNSUPPORTED_TYPE;
                }
            }
            mimeType = mimeType.toLowerCase();
        }
        if (Utils.isPtsTestMode() && mimeType.equals("text/x-vcalendar")) {
            Log.w(TAG, "Set Mime Type null to pass PTS BV-10-C ");
            mimeType = null;
        }

        // Reject anything outside the "acceptlist" plus unspecified MIME Types.
        if (mimeType == null
                || (!isAcceptlisted
                        && !Constants.mimeTypeMatches(
                                mimeType, Constants.ACCEPTABLE_SHARE_INBOUND_TYPES))) {
            Log.w(TAG, "mimeType is null or in unacceptable list, reject the transfer");
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.OPP,
                    BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                    7);
            return ResponseCodes.OBEX_HTTP_UNSUPPORTED_TYPE;
        }

        ContentValues values = new ContentValues();
        values.put(BluetoothShare.FILENAME_HINT, name);
        values.put(BluetoothShare.TOTAL_BYTES, length);
        values.put(BluetoothShare.MIMETYPE, mimeType);
        values.put(BluetoothShare.DESTINATION, destination);
        values.put(BluetoothShare.DIRECTION, BluetoothShare.DIRECTION_INBOUND);
        values.put(BluetoothShare.TIMESTAMP, mTimestamp);

        // It's not first put if !serverBlocking, so we auto accept it
        if (!mServerBlocking
                && (mAccepted == BluetoothShare.USER_CONFIRMATION_CONFIRMED
                        || mAccepted == BluetoothShare.USER_CONFIRMATION_AUTO_CONFIRMED)) {
            values.put(
                    BluetoothShare.USER_CONFIRMATION,
                    BluetoothShare.USER_CONFIRMATION_AUTO_CONFIRMED);
        }

        if (isAcceptlisted) {
            values.put(
                    BluetoothShare.USER_CONFIRMATION,
                    BluetoothShare.USER_CONFIRMATION_HANDOVER_CONFIRMED);
        }

        Uri contentUri =
                BluetoothMethodProxy.getInstance()
                        .contentResolverInsert(
                                mContext.getContentResolver(), BluetoothShare.CONTENT_URI, values);
        mLocalShareInfoId = Integer.parseInt(contentUri.getPathSegments().get(1));

        Log.v(TAG, "insert contentUri: " + contentUri);
        Log.v(TAG, "mLocalShareInfoId = " + mLocalShareInfoId);

        synchronized (this) {
            mPartialWakeLock.acquire();
            mServerBlocking = true;
            try {

                while (mServerBlocking) {
                    wait(1000);
                    if (mCallback != null && !mTimeoutMsgSent) {
                        mTimeoutMsgSent = true;
                        mCallback.sendMessageDelayed(
                                mCallback.obtainMessage(
                                        BluetoothOppObexSession.MSG_CONNECT_TIMEOUT),
                                BluetoothOppObexSession.SESSION_TIMEOUT);
                        Log.v(TAG, "MSG_CONNECT_TIMEOUT sent");
                    }
                }
            } catch (InterruptedException e) {
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.OPP,
                        BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                        8);
                Log.v(TAG, "Interrupted in onPut blocking");
            }
        }
        Log.d(TAG, "Server unblocked ");
        synchronized (this) {
            if (mCallback != null && mTimeoutMsgSent) {
                mCallback.removeMessages(BluetoothOppObexSession.MSG_CONNECT_TIMEOUT);
            }
        }

        /* we should have mInfo now */

        /*
         * TODO check if this mInfo match the one that we insert before server
         * blocking? just to make sure no error happens
         */
        if (mInfo.mId != mLocalShareInfoId) {
            Log.e(TAG, "Unexpected error!");
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.OPP,
                    BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_ERROR,
                    9);
        }
        mAccepted = mInfo.mConfirm;

        Log.v(TAG, "after confirm: userAccepted=" + mAccepted);
        int status = BluetoothShare.STATUS_SUCCESS;

        int obexResponse = ResponseCodes.OBEX_HTTP_OK;

        if (mAccepted == BluetoothShare.USER_CONFIRMATION_CONFIRMED
                || mAccepted == BluetoothShare.USER_CONFIRMATION_AUTO_CONFIRMED
                || mAccepted == BluetoothShare.USER_CONFIRMATION_HANDOVER_CONFIRMED) {
            /* Confirm or auto-confirm */
            mNumFilesAttemptedToReceive++;

            if (mFileInfo.mFileName == null) {
                status = mFileInfo.mStatus;
                /* TODO need to check if this line is correct */
                mInfo.mStatus = mFileInfo.mStatus;
                Constants.updateShareStatus(mContext, mInfo.mId, status);
                obexResponse = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            }

            if (mFileInfo.mFileName != null && mFileInfo.mInsertUri != null) {

                ContentValues updateValues = new ContentValues();
                contentUri = Uri.parse(BluetoothShare.CONTENT_URI + "/" + mInfo.mId);
                updateValues.put(BluetoothShare._DATA, mFileInfo.mFileName);
                updateValues.put(BluetoothShare.STATUS, BluetoothShare.STATUS_RUNNING);
                updateValues.put(BluetoothShare.URI, mFileInfo.mInsertUri.toString());
                BluetoothMethodProxy.getInstance()
                        .contentResolverUpdate(
                                mContext.getContentResolver(),
                                contentUri,
                                updateValues,
                                null,
                                null);

                mInfo.mUri = mFileInfo.mInsertUri;
                status = receiveFile(mFileInfo, op);
                /*
                 * TODO map status to obex response code
                 */
                if (status != BluetoothShare.STATUS_SUCCESS) {
                    obexResponse = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
                }
                Constants.updateShareStatus(mContext, mInfo.mId, status);
            }

            if (status == BluetoothShare.STATUS_SUCCESS) {
                Message msg = Message.obtain(mCallback, BluetoothOppObexSession.MSG_SHARE_COMPLETE);
                msg.obj = mInfo;
                msg.sendToTarget();
            } else {
                if (mCallback != null) {
                    Message msg =
                            Message.obtain(mCallback, BluetoothOppObexSession.MSG_SESSION_ERROR);
                    mInfo.mStatus = status;
                    msg.obj = mInfo;
                    msg.sendToTarget();
                }

                if (mFileInfo.mInsertUri != null) {
                    Log.d(TAG, "Download failed. Removing the file. Uri=" + mFileInfo.mInsertUri);
                    BluetoothMethodProxy.getInstance()
                            .contentResolverDelete(
                                    mContext.getContentResolver(),
                                    mFileInfo.mInsertUri,
                                    null,
                                    null);
                }
            }
        } else if (mAccepted == BluetoothShare.USER_CONFIRMATION_DENIED
                || mAccepted == BluetoothShare.USER_CONFIRMATION_TIMEOUT) {
            /* user actively deny the inbound transfer */
            /*
             * Note There is a question: what's next if user deny the first obj?
             * Option 1 :continue prompt for next objects
             * Option 2 :reject next objects and finish the session
             * Now we take option 2:
             */

            Log.i(TAG, "Rejected incoming request");
            if (mFileInfo.mInsertUri != null) {
                BluetoothMethodProxy.getInstance()
                        .contentResolverDelete(
                                mContext.getContentResolver(), mFileInfo.mInsertUri, null, null);
            }
            // set status as local cancel
            status = BluetoothShare.STATUS_CANCELED;
            Constants.updateShareStatus(mContext, mInfo.mId, status);
            obexResponse = ResponseCodes.OBEX_HTTP_FORBIDDEN;

            Message msg = Message.obtain(mCallback);
            msg.what = BluetoothOppObexSession.MSG_SHARE_INTERRUPTED;
            mInfo.mStatus = status;
            msg.obj = mInfo;
            msg.sendToTarget();
        }
        return obexResponse;
    }

    private int receiveFile(BluetoothOppReceiveFileInfo fileInfo, Operation op) {
        /*
         * implement receive file
         */
        int status = -1;
        OutputStream os = null;
        InputStream is = null;
        boolean error = false;
        try {
            is = op.openInputStream();
        } catch (IOException e1) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.OPP,
                    BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    10);
            Log.e(TAG, "Error when openInputStream");
            status = BluetoothShare.STATUS_OBEX_DATA_ERROR;
            error = true;
        }

        Uri contentUri = Uri.parse(BluetoothShare.CONTENT_URI + "/" + mInfo.mId);

        if (!error) {
            ContentValues updateValues = new ContentValues();
            updateValues.put(BluetoothShare._DATA, fileInfo.mFileName);
            BluetoothMethodProxy.getInstance()
                    .contentResolverUpdate(
                            mContext.getContentResolver(), contentUri, updateValues, null, null);
        }

        long position = 0;
        long percent;
        long prevPercent = 0;

        if (!error) {
            try {
                os =
                        BluetoothMethodProxy.getInstance()
                                .contentResolverOpenOutputStream(
                                        mContext.getContentResolver(), fileInfo.mInsertUri);
            } catch (FileNotFoundException e) {
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.OPP,
                        BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                        11);
                Log.e(TAG, "Error when openOutputStream");
                error = true;
            }
        }

        if (!error) {
            int outputBufferSize = op.getMaxPacketSize();
            byte[] b = new byte[outputBufferSize];
            int readLength;
            long timestamp = 0;
            long currentTime;
            long prevTimestamp = SystemClock.elapsedRealtime();
            try {
                while ((!mInterrupted) && (position != fileInfo.mLength)) {

                    timestamp = SystemClock.elapsedRealtime();

                    readLength = is.read(b);

                    if (readLength == -1) {
                        Log.d(TAG, "Receive file reached stream end at position" + position);
                        break;
                    }

                    os.write(b, 0, readLength);
                    position += readLength;
                    percent = position * 100 / fileInfo.mLength;
                    currentTime = SystemClock.elapsedRealtime();

                    Log.v(
                            TAG,
                            "Receive file position = "
                                    + position
                                    + " readLength "
                                    + readLength
                                    + " bytes took "
                                    + (currentTime - timestamp)
                                    + " ms");

                    // Update the Progress Bar only if there is change in percentage
                    // or once per a period to notify NFC of this transfer is still alive
                    if (percent > prevPercent
                            || currentTime - prevTimestamp > Constants.NFC_ALIVE_CHECK_MS) {
                        ContentValues updateValues = new ContentValues();
                        updateValues.put(BluetoothShare.CURRENT_BYTES, position);
                        BluetoothMethodProxy.getInstance()
                                .contentResolverUpdate(
                                        mContext.getContentResolver(),
                                        contentUri,
                                        updateValues,
                                        null,
                                        null);
                        prevPercent = percent;
                        prevTimestamp = currentTime;
                    }
                }
            } catch (IOException e1) {
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.OPP,
                        BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                        12);
                Log.e(TAG, "Error when receiving file: " + e1);
                /* OBEX Abort packet received from remote device */
                if ("Abort Received".equals(e1.getMessage())) {
                    status = BluetoothShare.STATUS_CANCELED;
                } else {
                    status = BluetoothShare.STATUS_OBEX_DATA_ERROR;
                }
                error = true;
            }
        }

        if (mInterrupted) {
            Log.d(TAG, "receiving file interrupted by user.");
            status = BluetoothShare.STATUS_CANCELED;
        } else {
            if (position == fileInfo.mLength) {
                Log.d(TAG, "Receiving file completed for " + fileInfo.mFileName);
                status = BluetoothShare.STATUS_SUCCESS;
            } else {
                Log.d(TAG, "Reading file failed at " + position + " of " + fileInfo.mLength);
                if (status == -1) {
                    status = BluetoothShare.STATUS_UNKNOWN_ERROR;
                }
            }
        }

        if (os != null) {
            try {
                os.flush();
                os.close();
            } catch (IOException e) {
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.OPP,
                        BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                        13);
                Log.e(TAG, "Error when closing stream after send");
            }
        }
        BluetoothOppUtility.cancelNotification(mContext);
        return status;
    }

    private BluetoothOppReceiveFileInfo processShareInfo() {
        Log.d(TAG, "processShareInfo() " + mInfo.mId);
        BluetoothOppReceiveFileInfo fileInfo =
                BluetoothOppReceiveFileInfo.generateFileInfo(mContext, mInfo.mId);
        Log.v(TAG, "Generate BluetoothOppReceiveFileInfo:");
        Log.v(TAG, "filename  :" + fileInfo.mFileName);
        Log.v(TAG, "length    :" + fileInfo.mLength);
        Log.v(TAG, "status    :" + fileInfo.mStatus);
        return fileInfo;
    }

    @Override
    public int onConnect(HeaderSet request, HeaderSet reply) {

        Log.d(TAG, "onConnect");
        Constants.logHeader(request);
        Long objectCount = null;
        try {
            byte[] uuid = (byte[]) request.getHeader(HeaderSet.TARGET);
            Log.v(TAG, "onConnect(): uuid =" + Arrays.toString(uuid));
            if (uuid != null) {
                return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
            }

            objectCount = (Long) request.getHeader(HeaderSet.COUNT);
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.OPP,
                    BluetoothProtoEnums.BLUETOOTH_OPP_OBEX_SERVER_SESSION,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    14);
            Log.e(TAG, e.toString());
            return ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
        }
        String destination;
        if (mTransport instanceof BluetoothObexTransport) {
            destination = ((BluetoothObexTransport) mTransport).getRemoteAddress();
        } else {
            destination = "FF:FF:FF:00:00:00";
        }
        boolean isHandover = BluetoothOppManager.getInstance(mContext).isAcceptlisted(destination);
        if (isHandover) {
            // Notify the handover requester file transfer has started
            Intent intent = new Intent(Constants.ACTION_HANDOVER_STARTED);
            if (objectCount != null) {
                intent.putExtra(Constants.EXTRA_BT_OPP_OBJECT_COUNT, objectCount.intValue());
            } else {
                intent.putExtra(
                        Constants.EXTRA_BT_OPP_OBJECT_COUNT, Constants.COUNT_HEADER_UNAVAILABLE);
            }
            intent.putExtra(Constants.EXTRA_BT_OPP_ADDRESS, destination);
            mContext.sendBroadcast(
                    intent,
                    Constants.HANDOVER_STATUS_PERMISSION,
                    Utils.getTempBroadcastOptions().toBundle());
        }
        mTimestamp = System.currentTimeMillis();
        mNumFilesAttemptedToReceive = 0;
        return ResponseCodes.OBEX_HTTP_OK;
    }

    @Override
    public void onDisconnect(HeaderSet req, HeaderSet resp) {
        Log.d(TAG, "onDisconnect");
        if (mNumFilesAttemptedToReceive > 0) {
            // Log incoming OPP transfer if more than one file is accepted by user
            MetricsLogger.logProfileConnectionEvent(BluetoothMetricsProto.ProfileId.OPP);
        }
        resp.responseCode = ResponseCodes.OBEX_HTTP_OK;
    }

    private synchronized void releaseWakeLocks() {
        if (mPartialWakeLock.isHeld()) {
            mPartialWakeLock.release();
        }
    }

    @Override
    public void onClose() {
        Log.d(TAG, "onClose");
        releaseWakeLocks();
        mBluetoothOppService.acceptNewConnections();
        BluetoothOppUtility.cancelNotification(mContext);
        /* onClose could happen even before start() where mCallback is set */
        if (mCallback != null) {
            Message msg = Message.obtain(mCallback);
            msg.what = BluetoothOppObexSession.MSG_SESSION_COMPLETE;
            msg.obj = mInfo;
            msg.sendToTarget();
        }
    }
}
