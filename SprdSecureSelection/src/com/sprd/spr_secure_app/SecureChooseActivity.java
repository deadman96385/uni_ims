package com.example.spr_secure_app;

import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.os.SystemProperties;

public class SecureChooseActivity extends Activity {

	Button BtnOK;
	Button BtnNOK;
	private String SECURE_PROPERTY = "persist.sys.secure.selected";
	private String tag;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		Button BtnOK = (Button)findViewById(R.id.button1);

		Button BtnNOK = (Button)findViewById(R.id.button2);

		BtnOK.setOnClickListener(new BtnOKListener());
		BtnNOK.setOnClickListener(new BtnNOKListener());
	}
    class BtnOKListener implements android.view.View.OnClickListener{

		@Override
		public void onClick(View arg0) {
			// TODO Auto-generated method stub
			Log.d(tag, "click Ok!");
                      SystemProperties.set(SECURE_PROPERTY, "1");
		      finish();
		}
    }

    class BtnNOKListener implements android.view.View.OnClickListener{

		@Override
		public void onClick(View arg0) {
			// TODO Auto-generated method stub
			Log.d(tag, "click not ok!");
                      SystemProperties.set(SECURE_PROPERTY, "0");
		      finish();
		}

    }
}
