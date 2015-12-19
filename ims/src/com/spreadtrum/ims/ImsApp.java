package com.spreadtrum.ims;

import android.app.Application;
import android.util.Log;
import android.content.ComponentName;
import android.content.Intent;


public class ImsApp extends Application {
    private static final String TAG = ImsApp.class.getSimpleName();

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "ImsApp Boot Successfully!");
        if(!ImsConfigImpl.isVolteEnabledBySystemProperties()){
            Log.w(TAG, "Could Not Start Ims Service because volte disabled by system properties!");
            return;
        }
        ComponentName comp = new ComponentName(this.getPackageName(),
                ImsService.class.getName());
        ComponentName service = this.startService(new Intent().setComponent(comp));
        if (service == null) {
            Log.w(TAG, "Could Not Start Service " + comp.toString());
        } else {
            Log.i(TAG, "ImsService Boot Successfully!");
        }
    }

}
