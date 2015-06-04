/*******************************************************************************
 * Copyright (c) 2012 by lasun Corporation all right reserved.
 * 2012-3-30 
 * 
 *******************************************************************************/ 
package com.sprd.security.tools.network.database;


import com.sprd.security.tools.network.R;

import android.app.DatePickerDialog;
import android.content.Context;

public class MyDatePickerDialog extends DatePickerDialog {

        public MyDatePickerDialog(Context context, int theme,
                        OnDateSetListener callBack, int year, int monthOfYear,
                        int dayOfMonth) {
                super(context, theme, callBack, year, monthOfYear, dayOfMonth);
                
        }
         public MyDatePickerDialog(Context context,
                    OnDateSetListener callBack,
                    int year,
                    int monthOfYear,
                    int dayOfMonth) {
                 super(context, callBack, year, monthOfYear, dayOfMonth);
                 setButton(context.getString(R.string.btn_sure) , this);
            }
}