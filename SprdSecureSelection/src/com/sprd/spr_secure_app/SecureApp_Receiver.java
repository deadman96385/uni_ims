package com.sprd.spr_secure_app;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Debug;
import android.util.Log;



public class SecureApp_Receiver extends BroadcastReceiver{

	private static final String TAG = "SecureApp_Receiver";

	@Override
	public void onReceive(Context context, Intent intent) {
		// TODO Auto-generated method stub
		Log.d(TAG, "onReceive actino = " + intent.getAction());
		String action = intent.getAction();
		if(action.equals("ACTION_SPR_SECURE_CHOOSE")){
			Intent intent1 = new Intent("com.sprd.spr_secure_app.SecureChooseActivity");
			context.sendBroadcast(intent1);
		}
	}
}
