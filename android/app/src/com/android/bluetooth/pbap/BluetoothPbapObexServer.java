/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.bluetooth.pbap;

import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothProtoEnums;
import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.os.Handler;
import android.os.Message;
import android.os.UserManager;
import android.provider.CallLog;
import android.provider.CallLog.Calls;
import android.text.TextUtils;
import android.util.Log;

import com.android.bluetooth.BluetoothMethodProxy;
import com.android.bluetooth.BluetoothStatsLog;
import com.android.bluetooth.content_profiles.ContentProfileErrorReportUtils;
import com.android.internal.annotations.VisibleForTesting;
import com.android.obex.ApplicationParameter;
import com.android.obex.HeaderSet;
import com.android.obex.Operation;
import com.android.obex.ResponseCodes;
import com.android.obex.ServerRequestHandler;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.text.CharacterIterator;
import java.text.StringCharacterIterator;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;

// Next tag value for ContentProfileErrorReportUtils.report(): 34
public class BluetoothPbapObexServer extends ServerRequestHandler {

    private static final String TAG = "BluetoothPbapObexServer";

    private static final int UUID_LENGTH = 16;

    public static final long INVALID_VALUE_PARAMETER = -1;

    // The length of suffix of vcard name - ".vcf" is 5
    private static final int VCARD_NAME_SUFFIX_LENGTH = 5;

    // 128 bit UUID for PBAP
    @VisibleForTesting
    public static final byte[] PBAP_TARGET =
            new byte[] {
                0x79,
                0x61,
                0x35,
                (byte) 0xf0,
                (byte) 0xf0,
                (byte) 0xc5,
                0x11,
                (byte) 0xd8,
                0x09,
                0x66,
                0x08,
                0x00,
                0x20,
                0x0c,
                (byte) 0x9a,
                0x66
            };

    private static final String[] LEGAL_PATH = {
        "/telecom",
        "/telecom/pb",
        "/telecom/fav",
        "/telecom/ich",
        "/telecom/och",
        "/telecom/mch",
        "/telecom/cch",
    };

    // SIM card is only supported when SIM feature is enabled
    // (i.e. when the property bluetooth.profile.pbap.sim.enabled is set to true)
    private static final String[] LEGAL_PATH_WITH_SIM = {
        "/telecom",
        "/telecom/pb",
        "/telecom/fav",
        "/telecom/ich",
        "/telecom/och",
        "/telecom/mch",
        "/telecom/cch",
        "/SIM1",
        "/SIM1/telecom",
        "/SIM1/telecom/ich",
        "/SIM1/telecom/och",
        "/SIM1/telecom/mch",
        "/SIM1/telecom/cch",
        "/SIM1/telecom/pb"
    };

    // SIM card
    @VisibleForTesting public static final String SIM1 = "SIM1";

    // missed call history
    @VisibleForTesting public static final String MCH = "mch";

    // incoming call history
    @VisibleForTesting public static final String ICH = "ich";

    // outgoing call history
    @VisibleForTesting public static final String OCH = "och";

    // combined call history
    @VisibleForTesting public static final String CCH = "cch";

    // phone book
    @VisibleForTesting public static final String PB = "pb";

    // favorites
    @VisibleForTesting public static final String FAV = "fav";

    @VisibleForTesting public static final String TELECOM_PATH = "/telecom";

    @VisibleForTesting public static final String ICH_PATH = "/telecom/ich";

    @VisibleForTesting public static final String OCH_PATH = "/telecom/och";

    @VisibleForTesting public static final String MCH_PATH = "/telecom/mch";

    @VisibleForTesting public static final String CCH_PATH = "/telecom/cch";

    @VisibleForTesting public static final String PB_PATH = "/telecom/pb";

    @VisibleForTesting public static final String FAV_PATH = "/telecom/fav";

    // SIM Support
    private static final String SIM_PATH = "/SIM1/telecom";

    private static final String SIM_ICH_PATH = "/SIM1/telecom/ich";

    private static final String SIM_OCH_PATH = "/SIM1/telecom/och";

    private static final String SIM_MCH_PATH = "/SIM1/telecom/mch";

    private static final String SIM_CCH_PATH = "/SIM1/telecom/cch";

    private static final String SIM_PB_PATH = "/SIM1/telecom/pb";

    // type for list vcard objects
    @VisibleForTesting public static final String TYPE_LISTING = "x-bt/vcard-listing";

    // type for get single vcard object
    @VisibleForTesting public static final String TYPE_VCARD = "x-bt/vcard";

    // to indicate if need send body besides headers
    private static final int NEED_SEND_BODY = -1;

    // type for download all vcard objects
    @VisibleForTesting public static final String TYPE_PB = "x-bt/phonebook";

    // The number of indexes in the phone book.
    private boolean mNeedPhonebookSize = false;

    // The number of missed calls that have not been checked on the PSE at the
    // point of the request. Only apply to "mch" case.
    private boolean mNeedNewMissedCallsNum = false;

    private boolean mVcardSelector = false;

    // record current path the client are browsing
    private String mCurrentPath = "";

    private Handler mCallback = null;

    private Context mContext;

    private BluetoothPbapVcardManager mVcardManager;

    BluetoothPbapSimVcardManager mVcardSimManager;

    private int mOrderBy = ORDER_BY_INDEXED;

    private static final int CALLLOG_NUM_LIMIT = 50;

    public static final int ORDER_BY_INDEXED = 0;

    public static final int ORDER_BY_ALPHABETICAL = 1;

    public static boolean sIsAborted = false;

    private long mDatabaseIdentifierLow = INVALID_VALUE_PARAMETER;

    private long mDatabaseIdentifierHigh = INVALID_VALUE_PARAMETER;

    private long mFolderVersionCounterbitMask = 0x0008;

    private long mDatabaseIdentifierBitMask = 0x0004;

    private AppParamValue mConnAppParamValue;

    private PbapStateMachine mStateMachine;

    private BluetoothMethodProxy mPbapMethodProxy;

    private enum ContactsType {
        TYPE_PHONEBOOK,
        TYPE_SIM;
    }

    public static class ContentType {
        public static final int PHONEBOOK = 1;

        public static final int INCOMING_CALL_HISTORY = 2;

        public static final int OUTGOING_CALL_HISTORY = 3;

        public static final int MISSED_CALL_HISTORY = 4;

        public static final int COMBINED_CALL_HISTORY = 5;

        public static final int FAVORITES = 6;

        public static final int SIM_PHONEBOOK = 7;
    }

    public BluetoothPbapObexServer(
            Handler callback, Context context, PbapStateMachine stateMachine) {
        super();
        mCallback = callback;
        mContext = context;
        mVcardManager = new BluetoothPbapVcardManager(mContext);
        mVcardSimManager = new BluetoothPbapSimVcardManager(mContext);
        mStateMachine = stateMachine;
        mPbapMethodProxy = BluetoothMethodProxy.getInstance();
    }

    @Override
    public int onConnect(final HeaderSet request, HeaderSet reply) {
        logHeader(request);
        notifyUpdateWakeLock();
        try {
            byte[] uuid = (byte[]) mPbapMethodProxy.getHeader(request, HeaderSet.TARGET);
            if (uuid == null) {
                return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
            }
            Log.d(TAG, "onConnect(): uuid=" + Arrays.toString(uuid));

            if (uuid.length != UUID_LENGTH) {
                Log.w(TAG, "Wrong UUID length");
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                        0);
                return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
            }
            for (int i = 0; i < UUID_LENGTH; i++) {
                if (uuid[i] != PBAP_TARGET[i]) {
                    Log.w(TAG, "Wrong UUID");
                    ContentProfileErrorReportUtils.report(
                            BluetoothProfile.PBAP,
                            BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                            BluetoothStatsLog
                                    .BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                            1);
                    return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
                }
            }
            reply.setHeader(HeaderSet.WHO, uuid);
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    2);
            Log.e(TAG, e.toString());
            return ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
        }

        try {
            byte[] remote = (byte[]) mPbapMethodProxy.getHeader(request, HeaderSet.WHO);
            if (remote != null) {
                Log.d(TAG, "onConnect(): remote=" + Arrays.toString(remote));
                reply.setHeader(HeaderSet.TARGET, remote);
            }
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    3);
            Log.e(TAG, e.toString());
            return ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
        }

        try {
            mConnAppParamValue = new AppParamValue();
            byte[] appParam =
                    (byte[]) mPbapMethodProxy.getHeader(request, HeaderSet.APPLICATION_PARAMETER);
            if ((appParam != null) && !parseApplicationParameter(appParam, mConnAppParamValue)) {
                return ResponseCodes.OBEX_HTTP_BAD_REQUEST;
            }
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    4);
            Log.e(TAG, e.toString());
            return ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
        }

        Log.v(TAG, "onConnect(): uuid is ok, will send out " + "MSG_SESSION_ESTABLISHED msg.");

        return ResponseCodes.OBEX_HTTP_OK;
    }

    @Override
    public void onDisconnect(final HeaderSet req, final HeaderSet resp) {
        Log.d(TAG, "onDisconnect(): enter");
        logHeader(req);
        notifyUpdateWakeLock();
        resp.responseCode = ResponseCodes.OBEX_HTTP_OK;
    }

    @Override
    public int onAbort(HeaderSet request, HeaderSet reply) {
        Log.d(TAG, "onAbort(): enter.");
        notifyUpdateWakeLock();
        sIsAborted = true;
        return ResponseCodes.OBEX_HTTP_OK;
    }

    @Override
    public int onPut(final Operation op) {
        Log.d(TAG, "onPut(): not support PUT request.");
        notifyUpdateWakeLock();
        return ResponseCodes.OBEX_HTTP_BAD_REQUEST;
    }

    @Override
    public int onDelete(final HeaderSet request, final HeaderSet reply) {
        Log.d(TAG, "onDelete(): not support PUT request.");
        notifyUpdateWakeLock();
        return ResponseCodes.OBEX_HTTP_BAD_REQUEST;
    }

    @Override
    public int onSetPath(
            final HeaderSet request,
            final HeaderSet reply,
            final boolean backup,
            final boolean create) {
        logHeader(request);
        Log.d(TAG, "before setPath, mCurrentPath ==  " + mCurrentPath);
        notifyUpdateWakeLock();
        String currentPathTmp = mCurrentPath;
        String tmpPath = null;
        try {
            tmpPath = (String) mPbapMethodProxy.getHeader(request, HeaderSet.NAME);
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    5);
            Log.e(TAG, "Get name header fail");
            return ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
        }
        Log.d(TAG, "backup=" + backup + " create=" + create + " name=" + tmpPath);

        if (backup) {
            if (currentPathTmp.length() != 0) {
                currentPathTmp = currentPathTmp.substring(0, currentPathTmp.lastIndexOf("/"));
            }
        } else {
            if (tmpPath == null) {
                currentPathTmp = "";
            } else {
                if (tmpPath.startsWith("/")) {
                    currentPathTmp = currentPathTmp + tmpPath;
                } else {
                    currentPathTmp = currentPathTmp + "/" + tmpPath;
                }
            }
        }

        if ((currentPathTmp.length() != 0) && (!isLegalPath(currentPathTmp))) {
            if (create) {
                Log.w(TAG, "path create is forbidden!");
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                        6);
                return ResponseCodes.OBEX_HTTP_FORBIDDEN;
            } else {
                Log.w(TAG, "path is not legal");
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                        7);
                return ResponseCodes.OBEX_HTTP_NOT_FOUND;
            }
        }
        mCurrentPath = currentPathTmp;
        Log.v(TAG, "after setPath, mCurrentPath ==  " + mCurrentPath);

        return ResponseCodes.OBEX_HTTP_OK;
    }

    @Override
    public void onClose() {
        mStateMachine.sendMessage(PbapStateMachine.DISCONNECT);
    }

    @Override
    public int onGet(Operation op) {
        notifyUpdateWakeLock();
        sIsAborted = false;
        HeaderSet request = null;
        HeaderSet reply = new HeaderSet();
        String type = "";
        String name = "";
        byte[] appParam = null;
        AppParamValue appParamValue = new AppParamValue();
        try {
            request = op.getReceivedHeader();
            type = (String) mPbapMethodProxy.getHeader(request, HeaderSet.TYPE);
            name = (String) mPbapMethodProxy.getHeader(request, HeaderSet.NAME);
            appParam =
                    (byte[]) mPbapMethodProxy.getHeader(request, HeaderSet.APPLICATION_PARAMETER);
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    8);
            Log.e(TAG, "request headers error");
            return ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
        }

        /* TODO: block Get request if contacts are not completely loaded locally */

        logHeader(request);
        Log.d(TAG, "OnGet type is " + type + "; name is " + name);

        if (type == null) {
            return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
        }

        if (!mPbapMethodProxy.getSystemService(mContext, UserManager.class).isUserUnlocked()) {
            Log.e(TAG, "Storage locked, " + type + " failed");
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_ERROR,
                    9);
            return ResponseCodes.OBEX_HTTP_UNAVAILABLE;
        }

        // Accroding to specification,the name header could be omitted such as
        // sony erriccsonHBH-DS980

        // For "x-bt/phonebook" and "x-bt/vcard-listing":
        // if name == null, guess what carkit actually want from current path
        // For "x-bt/vcard":
        // We decide which kind of content client would like per current path

        boolean validName = true;
        if (TextUtils.isEmpty(name)) {
            validName = false;
        }

        boolean isSimEnabled = BluetoothPbapService.isSimEnabled();

        if (!validName || (validName && type.equals(TYPE_VCARD))) {
            Log.d(TAG, "Guess what carkit actually want from current path (" + mCurrentPath + ")");

            if (mCurrentPath.equals(PB_PATH)) {
                appParamValue.needTag = ContentType.PHONEBOOK;
            } else if (mCurrentPath.equals(FAV_PATH)) {
                appParamValue.needTag = ContentType.FAVORITES;
            } else if (mCurrentPath.equals(ICH_PATH)
                    || (isSimEnabled && mCurrentPath.equals(SIM_ICH_PATH))) {
                appParamValue.needTag = ContentType.INCOMING_CALL_HISTORY;
            } else if (mCurrentPath.equals(OCH_PATH)
                    || (isSimEnabled && mCurrentPath.equals(SIM_OCH_PATH))) {
                appParamValue.needTag = ContentType.OUTGOING_CALL_HISTORY;
            } else if (mCurrentPath.equals(MCH_PATH)
                    || (isSimEnabled && mCurrentPath.equals(SIM_MCH_PATH))) {
                appParamValue.needTag = ContentType.MISSED_CALL_HISTORY;
                mNeedNewMissedCallsNum = true;
            } else if (mCurrentPath.equals(CCH_PATH)
                    || (isSimEnabled && mCurrentPath.equals(SIM_CCH_PATH))) {
                appParamValue.needTag = ContentType.COMBINED_CALL_HISTORY;
            } else if (mCurrentPath.equals(TELECOM_PATH)
                    || (isSimEnabled && mCurrentPath.equals(SIM_PATH))) {
                /* PBAP 1.1.1 change */
                if (!validName && type.equals(TYPE_LISTING)) {
                    Log.e(TAG, "invalid vcard listing request in default folder");
                    ContentProfileErrorReportUtils.report(
                            BluetoothProfile.PBAP,
                            BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                            BluetoothStatsLog
                                    .BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_ERROR,
                            10);
                    return ResponseCodes.OBEX_HTTP_NOT_FOUND;
                }
            } else if (isSimEnabled && mCurrentPath.equals(SIM_PB_PATH)) {
                appParamValue.needTag = ContentType.SIM_PHONEBOOK;
            } else {
                Log.w(TAG, "mCurrentpath is not valid path!!!");
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                        11);
                return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
            }
            Log.v(TAG, "onGet(): appParamValue.needTag=" + appParamValue.needTag);
        } else {
            // we have weak name checking here to provide better
            // compatibility with other devices,although unique name such as
            // "pb.vcf" is required by SIG spec.
            if (mVcardSimManager.isSimPhoneBook(
                    name, type, PB, SIM1, TYPE_PB, TYPE_LISTING, mCurrentPath)) {
                appParamValue.needTag = ContentType.SIM_PHONEBOOK;
                Log.d(TAG, "download SIM phonebook request");
                if (!isSimEnabled) {
                    // Not support SIM card currently
                    Log.w(TAG, "Not support access SIM card info!");
                    ContentProfileErrorReportUtils.report(
                            BluetoothProfile.PBAP,
                            BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                            BluetoothStatsLog
                                    .BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                            12);
                    return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
                }
            } else if (isNameMatchTarget(name, PB)) {
                appParamValue.needTag = ContentType.PHONEBOOK;
                Log.v(TAG, "download phonebook request");
            } else if (isNameMatchTarget(name, FAV)) {
                appParamValue.needTag = ContentType.FAVORITES;
                Log.v(TAG, "download favorites request");
            } else if (isNameMatchTarget(name, ICH)) {
                appParamValue.needTag = ContentType.INCOMING_CALL_HISTORY;
                appParamValue.callHistoryVersionCounter =
                        mVcardManager.getCallHistoryPrimaryFolderVersion(
                                ContentType.INCOMING_CALL_HISTORY);
                Log.v(TAG, "download incoming calls request");
            } else if (isNameMatchTarget(name, OCH)) {
                appParamValue.needTag = ContentType.OUTGOING_CALL_HISTORY;
                appParamValue.callHistoryVersionCounter =
                        mVcardManager.getCallHistoryPrimaryFolderVersion(
                                ContentType.OUTGOING_CALL_HISTORY);
                Log.v(TAG, "download outgoing calls request");
            } else if (isNameMatchTarget(name, MCH)) {
                appParamValue.needTag = ContentType.MISSED_CALL_HISTORY;
                appParamValue.callHistoryVersionCounter =
                        mVcardManager.getCallHistoryPrimaryFolderVersion(
                                ContentType.MISSED_CALL_HISTORY);
                mNeedNewMissedCallsNum = true;
                Log.v(TAG, "download missed calls request");
            } else if (isNameMatchTarget(name, CCH)) {
                appParamValue.needTag = ContentType.COMBINED_CALL_HISTORY;
                appParamValue.callHistoryVersionCounter =
                        mVcardManager.getCallHistoryPrimaryFolderVersion(
                                ContentType.COMBINED_CALL_HISTORY);
                Log.v(TAG, "download combined calls request");
            } else {
                Log.w(TAG, "Input name doesn't contain valid info!!!");
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                        13);
                return ResponseCodes.OBEX_HTTP_NOT_FOUND;
            }
        }

        if ((appParam != null) && !parseApplicationParameter(appParam, appParamValue)) {
            return ResponseCodes.OBEX_HTTP_BAD_REQUEST;
        }

        // listing request
        if (type.equals(TYPE_LISTING)) {
            return pullVcardListing(appParamValue, reply, op, name);
        } else if (type.equals(TYPE_VCARD)) {
            // pull vcard entry request
            return pullVcardEntry(appParamValue, op, reply, name);
        } else if (type.equals(TYPE_PB)) {
            // down load phone book request
            return pullPhonebook(appParamValue, reply, op, name);
        } else {
            Log.w(TAG, "unknown type request!!!");
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                    14);
            return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
        }
    }

    private boolean isNameMatchTarget(String name, String target) {
        if (name == null) {
            return false;
        }
        String contentTypeName = name;
        if (contentTypeName.endsWith(".vcf")) {
            contentTypeName =
                    contentTypeName.substring(0, contentTypeName.length() - ".vcf".length());
        }
        // There is a test case: Client will send a wrong name "/telecom/pbpb".
        // So we must use the String between '/' and '/' as a indivisible part
        // for comparing.
        String[] nameList = contentTypeName.split("/");
        for (String subName : nameList) {
            if (subName.equals(target)) {
                return true;
            }
        }
        return false;
    }

    /** check whether path is legal */
    private boolean isLegalPath(final String str) {
        if (str.length() == 0) {
            return true;
        }
        String[] legal_paths =
                BluetoothPbapService.isSimEnabled() ? LEGAL_PATH_WITH_SIM : LEGAL_PATH;

        for (int i = 0; i < legal_paths.length; i++) {
            if (str.equals(legal_paths[i])) {
                return true;
            }
        }
        return false;
    }

    @VisibleForTesting
    public static class AppParamValue {
        public int maxListCount;

        public int listStartOffset;

        public String searchValue;

        // Indicate which vCard parameter the search operation shall be carried
        // out on. Can be "Name | Number | Sound", default value is "Name".
        public String searchAttr;

        // Indicate which sorting order shall be used for the
        // <x-bt/vcard-listing> listing object.
        // Can be "Alphabetical | Indexed | Phonetical", default value is
        // "Indexed".
        public String order;

        public int needTag;

        public boolean vcard21;

        public byte[] propertySelector;

        public byte[] supportedFeature;

        public boolean ignorefilter;

        public byte[] vCardSelector;

        public String vCardSelectorOperator;

        public byte[] callHistoryVersionCounter;

        public AppParamValue() {
            maxListCount = 0xFFFF;
            listStartOffset = 0;
            searchValue = "";
            searchAttr = "";
            order = "";
            needTag = 0x00;
            vcard21 = true;
            // Filter is not set by default
            ignorefilter = true;
            vCardSelectorOperator = "0";
            propertySelector = new byte[] {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            vCardSelector = new byte[] {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            supportedFeature = new byte[] {0x00, 0x00, 0x00, 0x00};
        }

        @Override
        public String toString() {
            return "AppParamValue<maxListCount="
                    + maxListCount
                    + " listStartOffset="
                    + listStartOffset
                    + " searchValue="
                    + searchValue
                    + " searchAttr="
                    + searchAttr
                    + " needTag="
                    + needTag
                    + " vcard21="
                    + vcard21
                    + " order="
                    + order
                    + "vcardselector="
                    + Arrays.toString(vCardSelector)
                    + "vcardselop="
                    + vCardSelectorOperator
                    + ">";
        }
    }

    /** To parse obex application parameter */
    @VisibleForTesting
    boolean parseApplicationParameter(final byte[] appParam, AppParamValue appParamValue) {
        int i = 0;
        boolean parseOk = true;
        while ((i < appParam.length) && (parseOk)) {
            switch (appParam[i]) {
                case ApplicationParameter.TRIPLET_TAGID.PROPERTY_SELECTOR_TAGID:
                    i += 2; // length and tag field in triplet
                    for (int index = 0;
                            index < ApplicationParameter.TRIPLET_LENGTH.PROPERTY_SELECTOR_LENGTH;
                            index++) {
                        if (appParam[i + index] != 0) {
                            appParamValue.ignorefilter = false;
                            appParamValue.propertySelector[index] = appParam[i + index];
                        }
                    }
                    i += ApplicationParameter.TRIPLET_LENGTH.PROPERTY_SELECTOR_LENGTH;
                    break;
                case ApplicationParameter.TRIPLET_TAGID.SUPPORTEDFEATURE_TAGID:
                    i += 2; // length and tag field in triplet
                    for (int index = 0;
                            index < ApplicationParameter.TRIPLET_LENGTH.SUPPORTEDFEATURE_LENGTH;
                            index++) {
                        if (appParam[i + index] != 0) {
                            appParamValue.supportedFeature[index] = appParam[i + index];
                        }
                    }

                    i += ApplicationParameter.TRIPLET_LENGTH.SUPPORTEDFEATURE_LENGTH;
                    break;

                case ApplicationParameter.TRIPLET_TAGID.ORDER_TAGID:
                    i += 2; // length and tag field in triplet
                    appParamValue.order = Byte.toString(appParam[i]);
                    i += ApplicationParameter.TRIPLET_LENGTH.ORDER_LENGTH;
                    break;
                case ApplicationParameter.TRIPLET_TAGID.SEARCH_VALUE_TAGID:
                    i += 1; // length field in triplet
                    // length of search value is variable
                    int length = appParam[i];
                    if (length == 0) {
                        parseOk = false;
                        break;
                    }
                    if (appParam[i + length] == 0x0) {
                        appParamValue.searchValue = new String(appParam, i + 1, length - 1);
                    } else {
                        appParamValue.searchValue = new String(appParam, i + 1, length);
                    }
                    i += length;
                    i += 1;
                    break;
                case ApplicationParameter.TRIPLET_TAGID.SEARCH_ATTRIBUTE_TAGID:
                    i += 2;
                    appParamValue.searchAttr = Byte.toString(appParam[i]);
                    i += ApplicationParameter.TRIPLET_LENGTH.SEARCH_ATTRIBUTE_LENGTH;
                    break;
                case ApplicationParameter.TRIPLET_TAGID.MAXLISTCOUNT_TAGID:
                    i += 2;
                    if (appParam[i] == 0 && appParam[i + 1] == 0) {
                        mNeedPhonebookSize = true;
                    } else {
                        int highValue = appParam[i] & 0xff;
                        int lowValue = appParam[i + 1] & 0xff;
                        appParamValue.maxListCount = highValue * 256 + lowValue;
                    }
                    i += ApplicationParameter.TRIPLET_LENGTH.MAXLISTCOUNT_LENGTH;
                    break;
                case ApplicationParameter.TRIPLET_TAGID.LISTSTARTOFFSET_TAGID:
                    i += 2;
                    int highValue = appParam[i] & 0xff;
                    int lowValue = appParam[i + 1] & 0xff;
                    appParamValue.listStartOffset = highValue * 256 + lowValue;
                    i += ApplicationParameter.TRIPLET_LENGTH.LISTSTARTOFFSET_LENGTH;
                    break;
                case ApplicationParameter.TRIPLET_TAGID.FORMAT_TAGID:
                    i += 2; // length field in triplet
                    if (appParam[i] != 0) {
                        appParamValue.vcard21 = false;
                    }
                    i += ApplicationParameter.TRIPLET_LENGTH.FORMAT_LENGTH;
                    break;

                case ApplicationParameter.TRIPLET_TAGID.VCARDSELECTOR_TAGID:
                    i += 2;
                    for (int index = 0;
                            index < ApplicationParameter.TRIPLET_LENGTH.VCARDSELECTOR_LENGTH;
                            index++) {
                        if (appParam[i + index] != 0) {
                            mVcardSelector = true;
                            appParamValue.vCardSelector[index] = appParam[i + index];
                        }
                    }
                    i += ApplicationParameter.TRIPLET_LENGTH.VCARDSELECTOR_LENGTH;
                    break;
                case ApplicationParameter.TRIPLET_TAGID.VCARDSELECTOROPERATOR_TAGID:
                    i += 2;
                    appParamValue.vCardSelectorOperator = Byte.toString(appParam[i]);
                    i += ApplicationParameter.TRIPLET_LENGTH.VCARDSELECTOROPERATOR_LENGTH;
                    break;
                default:
                    parseOk = false;
                    Log.e(TAG, "Parse Application Parameter error");
                    ContentProfileErrorReportUtils.report(
                            BluetoothProfile.PBAP,
                            BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                            BluetoothStatsLog
                                    .BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_ERROR,
                            15);
                    break;
            }
        }

        Log.d(TAG, "ParseApplicationParameter: params=" + appParamValue);
        return parseOk;
    }

    /** Form and Send an XML format String to client for Phone book listing */
    private int sendVcardListingXml(
            AppParamValue appParamValue, Operation op, int needSendBody, int size) {
        StringBuilder result = new StringBuilder();
        int itemsFound = 0;
        result.append("<?xml version=\"1.0\"?>");
        result.append("<!DOCTYPE vcard-listing SYSTEM \"vcard-listing.dtd\">");
        result.append("<vCard-listing version=\"1.0\">");
        String type = "";
        // Phonebook listing request
        if ((appParamValue.needTag == ContentType.PHONEBOOK)
                || (appParamValue.needTag == ContentType.FAVORITES)) {
            if (appParamValue.searchAttr.equals("0")) {
                type = "name";
            } else if (appParamValue.searchAttr.equals("1")) {
                type = "number";
            }
            if (type.length() > 0) {
                itemsFound =
                        createList(
                                appParamValue,
                                needSendBody,
                                size,
                                result,
                                type,
                                ContactsType.TYPE_PHONEBOOK);
            } else {
                return ResponseCodes.OBEX_HTTP_PRECON_FAILED;
            }
            // SIM Phonebook listing Request
        } else if (appParamValue.needTag == ContentType.SIM_PHONEBOOK) {
            type = mVcardSimManager.getType(appParamValue.searchAttr);
            if (type.length() > 0) {
                itemsFound =
                        createList(
                                appParamValue,
                                needSendBody,
                                size,
                                result,
                                type,
                                ContactsType.TYPE_SIM);
            } else {
                return ResponseCodes.OBEX_HTTP_PRECON_FAILED;
            }
            // Call history listing request
        } else {
            ArrayList<String> nameList = mVcardManager.loadCallHistoryList(appParamValue.needTag);
            int requestSize =
                    nameList.size() >= appParamValue.maxListCount
                            ? appParamValue.maxListCount
                            : nameList.size();
            int startPoint = appParamValue.listStartOffset;
            int endPoint = startPoint + requestSize;
            if (endPoint > nameList.size()) {
                endPoint = nameList.size();
            }
            Log.d(
                    TAG,
                    "call log list, size="
                            + requestSize
                            + " offset="
                            + appParamValue.listStartOffset);

            for (int j = startPoint; j < endPoint; j++) {
                writeVCardEntry(j + 1, nameList.get(j), result);
            }
        }
        result.append("</vCard-listing>");

        Log.d(TAG, "itemsFound =" + itemsFound);

        return pushBytes(op, result.toString());
    }

    private int createList(
            AppParamValue appParamValue,
            int needSendBody,
            int size,
            StringBuilder result,
            String type,
            ContactsType contactType) {
        int itemsFound = 0;

        ArrayList<String> nameList = null;
        if (mVcardSelector) {
            if (contactType == ContactsType.TYPE_PHONEBOOK) {
                nameList =
                        mVcardManager.getSelectedPhonebookNameList(
                                mOrderBy,
                                appParamValue.vcard21,
                                needSendBody,
                                size,
                                appParamValue.vCardSelector,
                                appParamValue.vCardSelectorOperator);
            } else if (contactType == ContactsType.TYPE_SIM) {
                nameList = mVcardSimManager.getSIMPhonebookNameList(mOrderBy);
            }
        } else {
            if (contactType == ContactsType.TYPE_PHONEBOOK) {
                nameList = mVcardManager.getPhonebookNameList(mOrderBy);
            } else if (contactType == ContactsType.TYPE_SIM) {
                nameList = mVcardSimManager.getSIMPhonebookNameList(mOrderBy);
            }
        }

        final int requestSize =
                nameList.size() >= appParamValue.maxListCount
                        ? appParamValue.maxListCount
                        : nameList.size();
        final int listSize = nameList.size();
        String compareValue = "", currentValue;

        Log.d(
                TAG,
                "search by "
                        + type
                        + ", requestSize="
                        + requestSize
                        + " offset="
                        + appParamValue.listStartOffset
                        + " searchValue="
                        + appParamValue.searchValue);

        if (type.equals("number")) {
            ArrayList<Integer> savedPosList = new ArrayList<>();
            ArrayList<String> selectedNameList = new ArrayList<String>();
            // query the number, to get the names
            ArrayList<String> names = new ArrayList<>();
            if (contactType == ContactsType.TYPE_PHONEBOOK) {
                names = mVcardManager.getContactNamesByNumber(appParamValue.searchValue);
            } else if (contactType == ContactsType.TYPE_SIM) {
                names = mVcardSimManager.getSIMContactNamesByNumber(appParamValue.searchValue);
            }
            if (mOrderBy == ORDER_BY_ALPHABETICAL) Collections.sort(names);
            for (int i = 0; i < names.size(); i++) {
                compareValue = names.get(i).trim();
                Log.d(TAG, "compareValue=" + compareValue);
                for (int pos = 0; pos < listSize; pos++) {
                    currentValue = nameList.get(pos);
                    Log.v(TAG, "currentValue=" + currentValue);
                    if (currentValue.equals(compareValue)) {
                        if (currentValue.contains(",")) {
                            currentValue = currentValue.substring(0, currentValue.lastIndexOf(','));
                        }
                        selectedNameList.add(currentValue);
                        savedPosList.add(pos);
                    }
                }
            }

            for (int j = appParamValue.listStartOffset;
                    j < selectedNameList.size() && itemsFound < requestSize;
                    j++) {
                itemsFound++;
                writeVCardEntry(savedPosList.get(j), selectedNameList.get(j), result);
            }

        } else {
            ArrayList<Integer> savedPosList = new ArrayList<>();
            ArrayList<String> selectedNameList = new ArrayList<String>();
            if (appParamValue.searchValue != null) {
                compareValue = appParamValue.searchValue.trim().toLowerCase();
            }

            for (int pos = 0; pos < listSize; pos++) {
                currentValue = nameList.get(pos);

                if (currentValue.contains(",")) {
                    currentValue = currentValue.substring(0, currentValue.lastIndexOf(','));
                }

                if (appParamValue.searchValue != null) {
                    if (appParamValue.searchValue.isEmpty()
                            || ((currentValue.toLowerCase())
                                    .startsWith(compareValue.toLowerCase()))) {
                        selectedNameList.add(currentValue);
                        savedPosList.add(pos);
                    }
                }
            }

            for (int i = appParamValue.listStartOffset;
                    i < selectedNameList.size() && itemsFound < requestSize;
                    i++) {
                itemsFound++;
                writeVCardEntry(savedPosList.get(i), selectedNameList.get(i), result);
            }
        }
        return itemsFound;
    }

    /** Function to send obex header back to client such as get phonebook size request */
    @VisibleForTesting
    static int pushHeader(final Operation op, final HeaderSet reply) {
        OutputStream outputStream = null;

        Log.d(TAG, "Push Header");
        Log.d(TAG, reply.toString());

        int pushResult = ResponseCodes.OBEX_HTTP_OK;
        try {
            op.sendHeaders(reply);
            outputStream = op.openOutputStream();
            outputStream.flush();
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    16);
            Log.e(TAG, e.toString());
            pushResult = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
        } finally {
            if (!closeStream(outputStream, op)) {
                pushResult = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            }
        }
        return pushResult;
    }

    /** Function to send vcard data to client */
    private int pushBytes(Operation op, final String vcardString) {
        if (vcardString == null) {
            Log.w(TAG, "vcardString is null!");
            return ResponseCodes.OBEX_HTTP_OK;
        }

        OutputStream outputStream = null;
        int pushResult = ResponseCodes.OBEX_HTTP_OK;
        try {
            outputStream = op.openOutputStream();
            outputStream.write(vcardString.getBytes());
            Log.v(TAG, "Send Data complete!");
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    17);
            Log.e(TAG, "open/write outputstrem failed" + e.toString());
            pushResult = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
        }

        if (!closeStream(outputStream, op)) {
            pushResult = ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
        }

        return pushResult;
    }

    private int handleAppParaForResponse(
            AppParamValue appParamValue, int size, HeaderSet reply, Operation op, String name) {
        byte[] misnum = new byte[1];
        ApplicationParameter ap = new ApplicationParameter();
        boolean needSendCallHistoryVersionCounters = false;
        if (isNameMatchTarget(name, MCH)
                || isNameMatchTarget(name, ICH)
                || isNameMatchTarget(name, OCH)
                || isNameMatchTarget(name, CCH)) {
            needSendCallHistoryVersionCounters =
                    checkPbapFeatureSupport(mFolderVersionCounterbitMask);
        }
        boolean needSendPhonebookVersionCounters = false;
        if (isNameMatchTarget(name, PB) || isNameMatchTarget(name, FAV)) {
            needSendPhonebookVersionCounters =
                    checkPbapFeatureSupport(mFolderVersionCounterbitMask);
        }

        // In such case, PCE only want the number of index.
        // So response not contain any Body header.
        if (mNeedPhonebookSize) {
            Log.d(TAG, "Need Phonebook size in response header.");
            mNeedPhonebookSize = false;

            byte[] pbsize = new byte[2];

            pbsize[0] = (byte) ((size / 256) & 0xff); // HIGH VALUE
            pbsize[1] = (byte) ((size % 256) & 0xff); // LOW VALUE
            ap.addTriplet(
                    ApplicationParameter.TRIPLET_TAGID.PHONEBOOKSIZE_TAGID,
                    ApplicationParameter.TRIPLET_LENGTH.PHONEBOOKSIZE_LENGTH,
                    pbsize);

            if (mNeedNewMissedCallsNum) {
                mNeedNewMissedCallsNum = false;
                int nmnum = 0;
                ContentResolver contentResolver;
                contentResolver = mContext.getContentResolver();

                Cursor c =
                        contentResolver.query(
                                Calls.CONTENT_URI,
                                null,
                                Calls.TYPE
                                        + " = "
                                        + Calls.MISSED_TYPE
                                        + " AND "
                                        + android.provider.CallLog.Calls.NEW
                                        + " = 1",
                                null,
                                Calls.DEFAULT_SORT_ORDER);

                if (c != null) {
                    nmnum = c.getCount();
                    c.close();
                }

                nmnum = nmnum > 0 ? nmnum : 0;
                misnum[0] = (byte) nmnum;
                ap.addTriplet(
                        ApplicationParameter.TRIPLET_TAGID.NEWMISSEDCALLS_TAGID,
                        ApplicationParameter.TRIPLET_LENGTH.NEWMISSEDCALLS_LENGTH,
                        misnum);
                Log.d(
                        TAG,
                        "handleAppParaForResponse(): mNeedNewMissedCallsNum=true,  num= " + nmnum);
            }

            if (checkPbapFeatureSupport(mDatabaseIdentifierBitMask)) {
                setDbCounters(ap);
            }
            if (needSendPhonebookVersionCounters) {
                setFolderVersionCounters(ap);
            }
            if (needSendCallHistoryVersionCounters) {
                setCallversionCounters(ap, appParamValue);
            }
            reply.setHeader(HeaderSet.APPLICATION_PARAMETER, ap.getHeader());

            Log.d(TAG, "Send back Phonebook size only, without body info! Size= " + size);

            return pushHeader(op, reply);
        }

        // Only apply to "mch" download/listing.
        // NewMissedCalls is used only in the response, together with Body
        // header.
        if (mNeedNewMissedCallsNum) {
            Log.d(TAG, "Need new missed call num in response header.");
            mNeedNewMissedCallsNum = false;
            int nmnum = 0;
            ContentResolver contentResolver;
            contentResolver = mContext.getContentResolver();

            Cursor c =
                    contentResolver.query(
                            Calls.CONTENT_URI,
                            null,
                            Calls.TYPE
                                    + " = "
                                    + Calls.MISSED_TYPE
                                    + " AND "
                                    + android.provider.CallLog.Calls.NEW
                                    + " = 1",
                            null,
                            Calls.DEFAULT_SORT_ORDER);

            if (c != null) {
                nmnum = c.getCount();
                c.close();
            }

            nmnum = nmnum > 0 ? nmnum : 0;
            misnum[0] = (byte) nmnum;
            Log.d(TAG, "handleAppParaForResponse(): mNeedNewMissedCallsNum=true,  num= " + nmnum);

            ap.addTriplet(
                    ApplicationParameter.TRIPLET_TAGID.NEWMISSEDCALLS_TAGID,
                    ApplicationParameter.TRIPLET_LENGTH.NEWMISSEDCALLS_LENGTH,
                    misnum);
            reply.setHeader(HeaderSet.APPLICATION_PARAMETER, ap.getHeader());
            Log.d(TAG, "handleAppParaForResponse(): mNeedNewMissedCallsNum=true,  num= " + nmnum);

            // Only Specifies the headers, not write for now, will write to PCE
            // together with Body
            try {
                op.sendHeaders(reply);
            } catch (IOException e) {
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                        18);
                Log.e(TAG, e.toString());
                return ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            }
        }

        if (checkPbapFeatureSupport(mDatabaseIdentifierBitMask)) {
            setDbCounters(ap);
            reply.setHeader(HeaderSet.APPLICATION_PARAMETER, ap.getHeader());
            try {
                op.sendHeaders(reply);
            } catch (IOException e) {
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                        19);
                Log.e(TAG, e.toString());
                return ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            }
        }

        if (needSendPhonebookVersionCounters) {
            setFolderVersionCounters(ap);
            reply.setHeader(HeaderSet.APPLICATION_PARAMETER, ap.getHeader());
            try {
                op.sendHeaders(reply);
            } catch (IOException e) {
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                        20);
                Log.e(TAG, e.toString());
                return ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            }
        }

        if (needSendCallHistoryVersionCounters) {
            setCallversionCounters(ap, appParamValue);
            reply.setHeader(HeaderSet.APPLICATION_PARAMETER, ap.getHeader());
            try {
                op.sendHeaders(reply);
            } catch (IOException e) {
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                        21);
                Log.e(TAG, e.toString());
                return ResponseCodes.OBEX_HTTP_INTERNAL_ERROR;
            }
        }

        return NEED_SEND_BODY;
    }

    private int pullVcardListing(
            AppParamValue appParamValue, HeaderSet reply, Operation op, String name) {
        String searchAttr = appParamValue.searchAttr.trim();

        if (searchAttr == null || searchAttr.length() == 0) {
            // If searchAttr is not set by PCE, set default value per spec.
            appParamValue.searchAttr = "0";
            Log.d(TAG, "searchAttr is not set by PCE, assume search by name by default");
        } else if (!searchAttr.equals("0") && !searchAttr.equals("1")) {
            Log.w(TAG, "search attr not supported");
            if (searchAttr.equals("2")) {
                // search by sound is not supported currently
                Log.w(TAG, "do not support search by sound");
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                        22);
                return ResponseCodes.OBEX_HTTP_NOT_IMPLEMENTED;
            }
            return ResponseCodes.OBEX_HTTP_PRECON_FAILED;
        } else {
            Log.i(TAG, "searchAttr is valid: " + searchAttr);
        }

        int size = mVcardManager.getPhonebookSize(appParamValue.needTag, mVcardSimManager);
        int needSendBody = handleAppParaForResponse(appParamValue, size, reply, op, name);
        if (needSendBody != NEED_SEND_BODY) {
            op.noBodyHeader();
            return needSendBody;
        }

        if (size == 0) {
            Log.d(TAG, "PhonebookSize is 0, return.");
            return ResponseCodes.OBEX_HTTP_OK;
        }

        String orderPara = appParamValue.order.trim();
        if (TextUtils.isEmpty(orderPara)) {
            // If order parameter is not set by PCE, set default value per spec.
            orderPara = "0";
            Log.d(
                    TAG,
                    "Order parameter is not set by PCE. " + "Assume order by 'Indexed' by default");
        } else if (!orderPara.equals("0") && !orderPara.equals("1")) {
            Log.d(TAG, "Order parameter is not supported: " + appParamValue.order);
            if (orderPara.equals("2")) {
                // Order by sound is not supported currently
                Log.w(TAG, "Do not support order by sound");
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                        23);
                return ResponseCodes.OBEX_HTTP_NOT_IMPLEMENTED;
            }
            return ResponseCodes.OBEX_HTTP_PRECON_FAILED;
        } else {
            Log.i(TAG, "Order parameter is valid: " + orderPara);
        }

        if (orderPara.equals("0")) {
            mOrderBy = ORDER_BY_INDEXED;
        } else if (orderPara.equals("1")) {
            mOrderBy = ORDER_BY_ALPHABETICAL;
        }

        return sendVcardListingXml(appParamValue, op, needSendBody, size);
    }

    private int pullVcardEntry(
            AppParamValue appParamValue, Operation op, HeaderSet reply, final String name) {
        if (name == null || name.length() < VCARD_NAME_SUFFIX_LENGTH) {
            Log.d(TAG, "Name is Null, or the length of name < 5 !");
            return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
        }
        String strIndex = name.substring(0, name.length() - VCARD_NAME_SUFFIX_LENGTH + 1);
        int intIndex = 0;
        if (strIndex.trim().length() != 0) {
            try {
                intIndex = Integer.parseInt(strIndex);
            } catch (NumberFormatException e) {
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                        24);
                Log.e(TAG, "catch number format exception " + e.toString());
                return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
            }
        }

        int size = mVcardManager.getPhonebookSize(appParamValue.needTag, mVcardSimManager);
        int needSendBody = handleAppParaForResponse(appParamValue, size, reply, op, name);
        if (size == 0) {
            Log.d(TAG, "PhonebookSize is 0, return.");
            return ResponseCodes.OBEX_HTTP_NOT_FOUND;
        }

        boolean vcard21 = appParamValue.vcard21;
        if (appParamValue.needTag == 0) {
            Log.w(TAG, "wrong path!");
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                    25);
            return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
        } else if ((appParamValue.needTag == ContentType.PHONEBOOK)
                || (appParamValue.needTag == ContentType.FAVORITES)) {
            if (intIndex < 0 || intIndex >= size) {
                Log.w(TAG, "The requested vcard is not acceptable! name= " + name);
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                        26);
                return ResponseCodes.OBEX_HTTP_NOT_FOUND;
            } else if ((intIndex == 0) && (appParamValue.needTag == ContentType.PHONEBOOK)) {
                // For PB_PATH, 0.vcf is the phone number of this phone.
                String ownerVcard =
                        mVcardManager.getOwnerPhoneNumberVcard(
                                vcard21,
                                appParamValue.ignorefilter ? null : appParamValue.propertySelector);
                return pushBytes(op, ownerVcard);
            } else {
                return mVcardManager.composeAndSendPhonebookOneVcard(
                        op,
                        intIndex,
                        vcard21,
                        null,
                        mOrderBy,
                        appParamValue.ignorefilter,
                        appParamValue.propertySelector);
            }
        } else if (appParamValue.needTag == ContentType.SIM_PHONEBOOK) {
            if (intIndex < 0 || intIndex >= size) {
                Log.w(TAG, "The requested vcard is not acceptable! name= " + name);
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                        27);
                return ResponseCodes.OBEX_HTTP_NOT_FOUND;
            } else if (intIndex == 0) {
                // For PB_PATH, 0.vcf is the phone number of this phone.
                String ownerVcard =
                        mVcardManager.getOwnerPhoneNumberVcard(
                                vcard21,
                                appParamValue.ignorefilter ? null : appParamValue.propertySelector);
                return pushBytes(op, ownerVcard);
            } else {
                return BluetoothPbapSimVcardManager.composeAndSendSIMPhonebookOneVcard(
                        mContext, op, intIndex, vcard21, null, mOrderBy);
            }
        } else {
            if (intIndex <= 0 || intIndex > size) {
                Log.w(TAG, "The requested vcard is not acceptable! name= " + name);
                ContentProfileErrorReportUtils.report(
                        BluetoothProfile.PBAP,
                        BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                        BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                        28);
                return ResponseCodes.OBEX_HTTP_NOT_FOUND;
            }
            // For others (ich/och/cch/mch), 0.vcf is meaningless, and must
            // begin from 1.vcf
            if (intIndex >= 1) {
                return mVcardManager.composeAndSendSelectedCallLogVcards(
                        appParamValue.needTag,
                        op,
                        intIndex,
                        intIndex,
                        vcard21,
                        needSendBody,
                        size,
                        appParamValue.ignorefilter,
                        appParamValue.propertySelector,
                        appParamValue.vCardSelector,
                        appParamValue.vCardSelectorOperator,
                        mVcardSelector);
            }
        }
        return ResponseCodes.OBEX_HTTP_OK;
    }

    private int pullPhonebook(
            AppParamValue appParamValue, HeaderSet reply, Operation op, final String name) {
        // code start for passing PTS3.2 TC_PSE_PBD_BI_01_C
        if (name != null) {
            int dotIndex = name.indexOf(".");
            String vcf = "vcf";
            if (dotIndex >= 0 && dotIndex <= name.length()) {
                if (!name.regionMatches(dotIndex + 1, vcf, 0, vcf.length())) {
                    Log.w(TAG, "name is not .vcf");
                    ContentProfileErrorReportUtils.report(
                            BluetoothProfile.PBAP,
                            BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                            BluetoothStatsLog
                                    .BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                            29);
                    return ResponseCodes.OBEX_HTTP_NOT_ACCEPTABLE;
                }
            }
        } // code end for passing PTS3.2 TC_PSE_PBD_BI_01_C

        int pbSize = mVcardManager.getPhonebookSize(appParamValue.needTag, mVcardSimManager);
        int needSendBody = handleAppParaForResponse(appParamValue, pbSize, reply, op, name);
        if (needSendBody != NEED_SEND_BODY) {
            op.noBodyHeader();
            return needSendBody;
        }

        if (pbSize == 0) {
            Log.d(TAG, "PhonebookSize is 0, return.");
            return ResponseCodes.OBEX_HTTP_OK;
        }

        int requestSize =
                pbSize >= appParamValue.maxListCount ? appParamValue.maxListCount : pbSize;
        // Limit the number of call log to CALLLOG_NUM_LIMIT
        if ((appParamValue.needTag != BluetoothPbapObexServer.ContentType.PHONEBOOK)
                && (appParamValue.needTag != BluetoothPbapObexServer.ContentType.FAVORITES)
                && (appParamValue.needTag != BluetoothPbapObexServer.ContentType.SIM_PHONEBOOK)) {
            if (requestSize > CALLLOG_NUM_LIMIT) {
                requestSize = CALLLOG_NUM_LIMIT;
            }
        }

        /**
         * startIndex (resp., lastIndex) corresponds to the index of the first (resp., last) vcard
         * entry in the phonebook object. PBAP v1.2.3: only pb starts indexing at 0.vcf (owner
         * card), the other phonebook objects (e.g., fav) start at 1.vcf. Additionally, the owner
         * card is included in pb's pbSize. This means pbSize corresponds to the index of the last
         * vcf in the fav phonebook object, but does not for the pb phonebook object.
         */
        int startIndex = 1;
        int lastIndex = pbSize;
        if (appParamValue.needTag == BluetoothPbapObexServer.ContentType.PHONEBOOK) {
            startIndex = 0;
            lastIndex = pbSize - 1;
        }
        // [startPoint, endPoint] denote the range of vcf indices to send, inclusive.
        int startPoint = startIndex + appParamValue.listStartOffset;
        int endPoint = startPoint + requestSize - 1;
        if (appParamValue.listStartOffset < 0 || startPoint > lastIndex) {
            Log.w(TAG, "listStartOffset is not correct! " + startPoint);
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__LOG_WARN,
                    30);
            return ResponseCodes.OBEX_HTTP_OK;
        }
        if (endPoint > lastIndex) {
            endPoint = lastIndex;
        }

        Log.d(
                TAG,
                "pullPhonebook(): requestSize="
                        + requestSize
                        + " startPoint="
                        + startPoint
                        + " endPoint="
                        + endPoint);

        boolean vcard21 = appParamValue.vcard21;
        boolean favorites =
                (appParamValue.needTag == BluetoothPbapObexServer.ContentType.FAVORITES);
        if ((appParamValue.needTag == BluetoothPbapObexServer.ContentType.PHONEBOOK) || favorites) {
            if (startPoint == 0) {
                String ownerVcard =
                        mVcardManager.getOwnerPhoneNumberVcard(
                                vcard21,
                                appParamValue.ignorefilter ? null : appParamValue.propertySelector);
                if (endPoint == 0) {
                    return pushBytes(op, ownerVcard);
                } else {
                    return mVcardManager.composeAndSendPhonebookVcards(
                            op,
                            1,
                            endPoint,
                            vcard21,
                            ownerVcard,
                            needSendBody,
                            pbSize,
                            appParamValue.ignorefilter,
                            appParamValue.propertySelector,
                            appParamValue.vCardSelector,
                            appParamValue.vCardSelectorOperator,
                            mVcardSelector,
                            favorites);
                }
            } else {
                return mVcardManager.composeAndSendPhonebookVcards(
                        op,
                        startPoint,
                        endPoint,
                        vcard21,
                        null,
                        needSendBody,
                        pbSize,
                        appParamValue.ignorefilter,
                        appParamValue.propertySelector,
                        appParamValue.vCardSelector,
                        appParamValue.vCardSelectorOperator,
                        mVcardSelector,
                        favorites);
            }
        } else if (appParamValue.needTag == BluetoothPbapObexServer.ContentType.SIM_PHONEBOOK) {
            if (startPoint == 0) {
                String ownerVcard =
                        mVcardManager.getOwnerPhoneNumberVcard(
                                vcard21, appParamValue.propertySelector);
                if (endPoint == 0) {
                    return pushBytes(op, ownerVcard);
                } else {
                    return BluetoothPbapSimVcardManager.composeAndSendSIMPhonebookVcards(
                            mContext, op, 1, endPoint, vcard21, ownerVcard);
                }
            } else {
                return BluetoothPbapSimVcardManager.composeAndSendSIMPhonebookVcards(
                        mContext, op, startPoint, endPoint, vcard21, null);
            }
        } else {
            return mVcardManager.composeAndSendSelectedCallLogVcards(
                    appParamValue.needTag,
                    op,
                    startPoint,
                    endPoint,
                    vcard21,
                    needSendBody,
                    pbSize,
                    appParamValue.ignorefilter,
                    appParamValue.propertySelector,
                    appParamValue.vCardSelector,
                    appParamValue.vCardSelectorOperator,
                    mVcardSelector);
        }
    }

    public static boolean closeStream(final OutputStream out, final Operation op) {
        boolean returnvalue = true;
        try {
            if (out != null) {
                out.close();
            }
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    31);
            Log.e(TAG, "outputStream close failed" + e.toString());
            returnvalue = false;
        }
        try {
            if (op != null) {
                op.close();
            }
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    32);
            Log.e(TAG, "operation close failed" + e.toString());
            returnvalue = false;
        }
        return returnvalue;
    }

    // Reserved for future use. In case PSE challenge PCE and PCE input wrong
    // session key.
    @Override
    public final void onAuthenticationFailure(final byte[] userName) {}

    public static final String createSelectionPara(final int type) {
        String selection = null;
        switch (type) {
            case ContentType.INCOMING_CALL_HISTORY:
                selection =
                        "("
                                + Calls.TYPE
                                + "="
                                + CallLog.Calls.INCOMING_TYPE
                                + " OR "
                                + Calls.TYPE
                                + "="
                                + CallLog.Calls.REJECTED_TYPE
                                + ")";
                break;
            case ContentType.OUTGOING_CALL_HISTORY:
                selection = Calls.TYPE + "=" + CallLog.Calls.OUTGOING_TYPE;
                break;
            case ContentType.MISSED_CALL_HISTORY:
                selection = Calls.TYPE + "=" + CallLog.Calls.MISSED_TYPE;
                break;
            default:
                break;
        }
        Log.v(TAG, "Call log selection: " + selection);
        return selection;
    }

    /** XML encode special characters in the name field */
    private static void xmlEncode(String name, StringBuilder result) {
        if (name == null) {
            return;
        }

        final StringCharacterIterator iterator = new StringCharacterIterator(name);
        char character = iterator.current();
        while (character != CharacterIterator.DONE) {
            if (character == '<') {
                result.append("&lt;");
            } else if (character == '>') {
                result.append("&gt;");
            } else if (character == '\"') {
                result.append("&quot;");
            } else if (character == '\'') {
                result.append("&#039;");
            } else if (character == '&') {
                result.append("&amp;");
            } else {
                // The char is not a special one, add it to the result as is
                result.append(character);
            }
            character = iterator.next();
        }
    }

    @VisibleForTesting
    static void writeVCardEntry(int vcfIndex, String name, StringBuilder result) {
        result.append("<card handle=\"");
        result.append(vcfIndex);
        result.append(".vcf\" name=\"");
        xmlEncode(name, result);
        result.append("\"/>");
    }

    private void notifyUpdateWakeLock() {
        Message msg = Message.obtain(mCallback);
        msg.what = BluetoothPbapService.MSG_ACQUIRE_WAKE_LOCK;
        msg.sendToTarget();
    }

    public static final void logHeader(HeaderSet hs) {
        Log.v(TAG, "Dumping HeaderSet " + hs.toString());
        try {

            Log.v(TAG, "COUNT : " + hs.getHeader(HeaderSet.COUNT));
            Log.v(TAG, "NAME : " + hs.getHeader(HeaderSet.NAME));
            Log.v(TAG, "TYPE : " + hs.getHeader(HeaderSet.TYPE));
            Log.v(TAG, "LENGTH : " + hs.getHeader(HeaderSet.LENGTH));
            Log.v(TAG, "TIME_ISO_8601 : " + hs.getHeader(HeaderSet.TIME_ISO_8601));
            Log.v(TAG, "TIME_4_BYTE : " + hs.getHeader(HeaderSet.TIME_4_BYTE));
            Log.v(TAG, "DESCRIPTION : " + hs.getHeader(HeaderSet.DESCRIPTION));
            Log.v(TAG, "TARGET : " + hs.getHeader(HeaderSet.TARGET));
            Log.v(TAG, "HTTP : " + hs.getHeader(HeaderSet.HTTP));
            Log.v(TAG, "WHO : " + hs.getHeader(HeaderSet.WHO));
            Log.v(TAG, "OBJECT_CLASS : " + hs.getHeader(HeaderSet.OBJECT_CLASS));
            Log.v(TAG, "APPLICATION_PARAMETER : " + hs.getHeader(HeaderSet.APPLICATION_PARAMETER));
        } catch (IOException e) {
            ContentProfileErrorReportUtils.report(
                    BluetoothProfile.PBAP,
                    BluetoothProtoEnums.BLUETOOTH_PBAP_OBEX_SERVER,
                    BluetoothStatsLog.BLUETOOTH_CONTENT_PROFILE_ERROR_REPORTED__TYPE__EXCEPTION,
                    33);
            Log.e(TAG, "dump HeaderSet error " + e);
        }
    }

    @VisibleForTesting
    void setDbCounters(ApplicationParameter ap) {
        ap.addTriplet(
                ApplicationParameter.TRIPLET_TAGID.DATABASEIDENTIFIER_TAGID,
                ApplicationParameter.TRIPLET_LENGTH.DATABASEIDENTIFIER_LENGTH,
                getDatabaseIdentifier());
    }

    @VisibleForTesting
    static void setFolderVersionCounters(ApplicationParameter ap) {
        ap.addTriplet(
                ApplicationParameter.TRIPLET_TAGID.PRIMARYVERSIONCOUNTER_TAGID,
                ApplicationParameter.TRIPLET_LENGTH.PRIMARYVERSIONCOUNTER_LENGTH,
                getPBPrimaryFolderVersion());
        ap.addTriplet(
                ApplicationParameter.TRIPLET_TAGID.SECONDARYVERSIONCOUNTER_TAGID,
                ApplicationParameter.TRIPLET_LENGTH.SECONDARYVERSIONCOUNTER_LENGTH,
                getPBSecondaryFolderVersion());
    }

    @VisibleForTesting
    static void setCallversionCounters(ApplicationParameter ap, AppParamValue appParamValue) {
        ap.addTriplet(
                ApplicationParameter.TRIPLET_TAGID.PRIMARYVERSIONCOUNTER_TAGID,
                ApplicationParameter.TRIPLET_LENGTH.PRIMARYVERSIONCOUNTER_LENGTH,
                appParamValue.callHistoryVersionCounter);

        ap.addTriplet(
                ApplicationParameter.TRIPLET_TAGID.SECONDARYVERSIONCOUNTER_TAGID,
                ApplicationParameter.TRIPLET_LENGTH.SECONDARYVERSIONCOUNTER_LENGTH,
                appParamValue.callHistoryVersionCounter);
    }

    @VisibleForTesting
    byte[] getDatabaseIdentifier() {
        mDatabaseIdentifierHigh = 0;
        mDatabaseIdentifierLow = BluetoothPbapUtils.sDbIdentifier.get();
        if (mDatabaseIdentifierLow != INVALID_VALUE_PARAMETER
                && mDatabaseIdentifierHigh != INVALID_VALUE_PARAMETER) {
            ByteBuffer ret = ByteBuffer.allocate(16);
            ret.putLong(mDatabaseIdentifierHigh);
            ret.putLong(mDatabaseIdentifierLow);
            return ret.array();
        } else {
            return null;
        }
    }

    @VisibleForTesting
    static byte[] getPBPrimaryFolderVersion() {
        long primaryVcMsb = 0;
        ByteBuffer pvc = ByteBuffer.allocate(16);
        pvc.putLong(primaryVcMsb);

        Log.d(TAG, "primaryVersionCounter is " + BluetoothPbapUtils.sPrimaryVersionCounter);
        pvc.putLong(BluetoothPbapUtils.sPrimaryVersionCounter);
        return pvc.array();
    }

    @VisibleForTesting
    static byte[] getPBSecondaryFolderVersion() {
        long secondaryVcMsb = 0;
        ByteBuffer svc = ByteBuffer.allocate(16);
        svc.putLong(secondaryVcMsb);

        Log.d(TAG, "secondaryVersionCounter is " + BluetoothPbapUtils.sSecondaryVersionCounter);
        svc.putLong(BluetoothPbapUtils.sSecondaryVersionCounter);
        return svc.array();
    }

    private boolean checkPbapFeatureSupport(long featureBitMask) {
        Log.d(TAG, "checkPbapFeatureSupport featureBitMask is " + featureBitMask);
        return ((ByteBuffer.wrap(mConnAppParamValue.supportedFeature).getInt() & featureBitMask)
                != 0);
    }

    @VisibleForTesting
    public void setCurrentPath(String path) {
        mCurrentPath = path != null ? path : "";
    }

    @VisibleForTesting
    public void setConnAppParamValue(AppParamValue connAppParamValue) {
        mConnAppParamValue = connAppParamValue;
    }
}
