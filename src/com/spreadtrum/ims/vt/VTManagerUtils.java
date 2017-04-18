package com.spreadtrum.ims.vt;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnKeyListener;
import android.content.Intent;
import android.telecom.VideoProfile;
import android.telecom.TelecomManager;
import android.net.Uri;
import android.view.View;
import android.view.KeyEvent;
import android.view.WindowManager;
import android.view.LayoutInflater;
import android.widget.Toast;
import android.widget.TextView;
import android.widget.ListView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;

import com.spreadtrum.ims.ImsRIL;
import com.spreadtrum.ims.R;

import static com.android.internal.telephony.PhoneConstants.SUBSCRIPTION_KEY;
import com.android.internal.telephony.GsmCdmaPhone;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.CommandsInterface;
import android.os.Message;

import android.telephony.TelephonyManagerEx;

public class VTManagerUtils {
    private static final String TAG = VTManagerUtils.class.getSimpleName();

    public static final int VIDEO_CALL_NORLMAL_CLEAR = 16;
    public static final int VIDEO_CALL_NORLMAL_DISCONNECT = 31;
    public static final int VIDEO_CALL_NO_SERVICE = 50;
    public static final int VIDEO_CALL_CAPABILITY_NOT_AUTHORIZED = 57;
    public static final int VIDEO_CALL_NORLMAL_UNSPECIFIELD = 255;
    public static final int VODEO_CALL_FDN_BLOCKED = 241;

    public static void showVideoCallFailToast(Context context,int disconnectCause){
        log("showVideoCallFailToast-> disconnectCause:"+disconnectCause);
        if(disconnectCause == VIDEO_CALL_NORLMAL_CLEAR){
            return;
        } else if(disconnectCause == VIDEO_CALL_NORLMAL_DISCONNECT){
            Toast.makeText(context,context.getString(R.string.net_connection_disconnect),
                    Toast.LENGTH_LONG).show();
        } else if(disconnectCause == VIDEO_CALL_NORLMAL_UNSPECIFIELD){
            Toast.makeText(context,context.getString(R.string.videophone_failcause_3),
                    Toast.LENGTH_LONG).show();
        } else if(disconnectCause == VODEO_CALL_FDN_BLOCKED){
            Toast.makeText(context,context.getString(R.string.callFailed_fdn_only),
                    Toast.LENGTH_LONG).show();
        } else {
            Toast.makeText(context,getVideoCallFailReason(context,disconnectCause),
                    Toast.LENGTH_LONG).show();
        }
    }

    public static View getVideoCallFallBackView(final Context context,final String number,
            int disconnectCause, final GsmCdmaPhone phone) {
        LayoutInflater inflater = (LayoutInflater)context.getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        final CharSequence[] items = context.getResources().getTextArray(R.array.videophone_fallback_menu);

        View view = inflater.inflate(R.xml.vt_fallback_dialog_ex,null);
        TextView causeView = (TextView) view.findViewById(R.id.FallBackCause);
        causeView.setText(getVideoCallFailReason(context,disconnectCause));

        ListView fallBackList = (ListView) view.findViewById(R.id.FallBackList);
        fallBackList.setAdapter(new ArrayAdapter<CharSequence>(context,
                android.R.layout.simple_list_item_1, items));
        fallBackList.setItemsCanFocus(false);
        fallBackList.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        fallBackList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                log("onFallBackListItemClick-> position:"+position);
                switch (position) {
                case 0:
                    placeCall(context, true, number, phone.getSubId());
                    break;
                case 1:
                    placeCall(context, false, number, phone.getSubId());
                    break;
                case 2:
                    break;
                default:
                    break;
                }
                VTManagerProxy.getInstance().dismissFallBackDialog();
            }
        });
        return view;

    }

    public static void initVideoCallFallBackDialog(AlertDialog dialog, Context context,final String number,
            int disconnectCause, final GsmCdmaPhone phone){

        if (disconnectCause == VIDEO_CALL_NO_SERVICE || disconnectCause == VIDEO_CALL_CAPABILITY_NOT_AUTHORIZED) {
            dialog.setTitle(context.getString(R.string.no_vt_service));
        } else {
            dialog.setTitle(context.getString(R.string.videophone_fallback_title));
        }
        dialog.setCanceledOnTouchOutside(false);
        dialog.setOnKeyListener(new OnKeyListener() {
            public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    switch (keyCode) {
                    case KeyEvent.KEYCODE_BACK:
                        dialog.dismiss();
                        return true;
                    case KeyEvent.KEYCODE_SEARCH:
                        log("KEYCODE_SEARCH");
                        return true;
                    }
                }
                return false;
            }
        });
        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_DIALOG);
        dialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
    }

    public static void placeCall(Context context,boolean isVideo, String number, int subId){
        String url = "tel:" + number;
        int videoState = isVideo ? VideoProfile.STATE_BIDIRECTIONAL : VideoProfile.STATE_AUDIO_ONLY;

        Intent intent = new Intent(Intent.ACTION_CALL, Uri.parse(url));
        intent.putExtra(TelecomManager.EXTRA_START_CALL_WITH_VIDEO_STATE, videoState);
        intent.putExtra(SUBSCRIPTION_KEY, subId);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
    }

    /**
     * Get Video Call Fail Reason from disconnect cause value.
     */
    public static String getVideoCallFailReason(Context context,int disconnectCause) {
        log("getVideoCallFailReason-> disconnectCause:"+disconnectCause);

        switch (disconnectCause) {
        case 1000:
            return context.getString(R.string.videophone_failcause_1000);
        case 1:
        case 22:
        case 28:
            return context.getString(R.string.videophone_failcause_1);
        case 3:
        case 6:
        case 18:
        case 21:
        case 29:
        case 38:
        case 41:
        case 43:
        case 49:
        case 81:
            return context.getString(R.string.videophone_failcause_3);
        case 8:
        case 55:
            return context.getString(R.string.videophone_failcause_8);
        case 17:
            return context.getString(R.string.videophone_failcause_17);
        case 19:
            return context.getString(R.string.videophone_failcause_19);
        case 27:
            return context.getString(R.string.videophone_failcause_27);
        case 34:
        case 42:
        case 44:
            return context.getString(R.string.videophone_failcause_34);
        case 63:
            return context.getString(R.string.videophone_failcause_63_modify);
        case 79:
            return context.getString(R.string.videophone_failcause_79);
        case 47:
            return context.getString(R.string.videophone_failcause_47);
        case 57:
            return context.getString(R.string.videophone_failcause_57_remote);
        case 58:
            return context.getString(R.string.videophone_failcause_58);
        case 88:
            return context.getString(R.string.videophone_failcause_88);
        case 50:
            return context.getString(R.string.videophone_failcause_50);
        case 157:
            return context.getString(R.string.videophone_failcause_157);
        case -1:
            return context.getString(R.string.videophone_failcause_minus_1);
        default:
            return context.getString(R.string.videophone_failcause_default);

        }

    }

    private static void log(String string){
        android.util.Log.i(TAG, string);
    }

    public static AlertDialog showVolteCallMediaUpdateAlert(Context context, final ImsRIL mCi, Message response,ImsVideoCallProvider vtprovide) {
        /* SPRD: add for bug545171 @{ */
        final Message reponseMsg = new Message();
        if(response != null){
            reponseMsg.arg1 = response.arg1;
        }
        /* @} */
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(context.getString(R.string.remote_request_change));
        /* SPRD: Add feature of low battery for Reliance@{ */
        builder.setMessage(TelephonyManagerEx.isBatteryLow()? context.getString(R.string.low_battery_warning_media_alert_message):context.getString(R.string.choose_accept_reject));
        /* @} */
        builder.setPositiveButton(context.getString(R.string.remote_request_change_accept), new android.content.DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                mCi.responseVolteCallMediaChange(true,reponseMsg);
                vtprovide.receiveSessionModifyResponse(android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_INVALID,
                        null,null);//SPRD:add for bug610607
                if(dialog != null){
                    dialog.dismiss();
                }
            }
        });
        builder.setNegativeButton(context.getString(R.string.remote_request_change_reject), new android.content.DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                mCi.responseVolteCallMediaChange(false,reponseMsg);
                vtprovide.receiveSessionModifyResponse(android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_INVALID,
                        null,null);//SPRD:add for bug610607
                if(dialog != null){
                    dialog.dismiss();
                }
            }
        });
        builder.setCancelable(false);
        AlertDialog dialog = builder.create();
        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_KEYGUARD_DIALOG);
        dialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
        return dialog;
    }

    /* SPRD: Add feature of low battery for Reliance @{ */
    public static AlertDialog showLowBatteryMediaChangeAlert(final Context context, final ImsRIL ril) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(context.getString(R.string.low_battery_warning_title));
        builder.setMessage(context.getString(R.string.low_battery_warning_message));
        builder.setPositiveButton(
                context.getString(R.string.low_battery_continue_video_call),
                new android.content.DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (dialog != null) {
                            dialog.dismiss();
                        }
                    }
                });
        builder.setNegativeButton(
                context.getString(R.string.low_battery_convert_to_voice_call),
                new android.content.DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (ril != null) {
                            ril.requestVolteCallMediaChange(true, null);
                            log("Battery is low,user choose to downgrade to voice call.");
                        }
                        if (dialog != null) {
                            dialog.dismiss();
                        }
                    }
                });
        builder.setCancelable(false);
        AlertDialog dialog = builder.create();
        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_DIALOG);
        dialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
        return dialog;
    }
    /* @} */

}
