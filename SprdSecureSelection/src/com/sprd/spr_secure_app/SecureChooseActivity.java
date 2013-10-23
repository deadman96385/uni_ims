package com.sprd.spr_secure_app;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.app.Activity;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.os.SystemProperties;
import android.widget.TextView;

public class SecureChooseActivity extends Activity {
	Button BtnOK;
	Button BtnNOK;
	private TextView timer;
	private String SECURE_PROPERTY = "persist.sys.secure.selected";
	private String tag = this.getClass().getName();
	public static int waitTime=10;
	private Handler handler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			// TODO Auto-generated method stub
			// super.handleMessage(msg);
			switch (msg.what) {
			case 0:
				updateTimer(msg.obj);
				break;
			case 1:
				// set default value
				String val = SystemProperties.get(SECURE_PROPERTY, "0");
				Log.v(tag, "current "+SECURE_PROPERTY +" value is " + val);
				// over the activity
				Log.d(tag, "close the activity!");
				finish();
				break;
			default:
				break;
			}

		}
	};

	private void updateTimer(Object msg) {
		// TODO Auto-generated method stub
		timer.setText(getResources().getString(R.string.timer) + msg.toString() + getResources().getString(R.string.second));
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.v(tag, "start activity SecurityChooseActivity");
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		Button BtnOK = (Button) findViewById(R.id.button1);
		Button BtnNOK = (Button) findViewById(R.id.button2);
		timer = (TextView) this.findViewById(R.id.textView2);
		timer.setText(getResources().getString(R.string.timer)+waitTime+getResources().getString(R.string.second));
		BtnOK.setOnClickListener(new BtnOKListener());
		BtnNOK.setOnClickListener(new BtnNOKListener());
		Button btnCancel = (Button) findViewById(R.id.cancel);
		btnCancel.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View arg0) {
				// TODO cancel the activity
				Message msg = new Message();
				msg.obj = 0;
				msg.what = 1;
				handler.sendMessage(msg);
			}
		});
		new Thread(new Runnable() {
			@Override
			public void run() {
				// TODO Auto-generated method stub
				int timeCount = waitTime;
				while (waitTime <=timeCount) {
					waitTime--;
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					Message msg = new Message();
					msg.obj = waitTime;
					msg.what = 0;
					handler.sendMessage(msg);
					if(waitTime == 0)
					{
						break;
					}
				}
				Message msg = new Message();
				msg.obj = 0;
				msg.what = 1;
				handler.sendMessage(msg);
			}
		}).start();
	}

	class BtnOKListener implements android.view.View.OnClickListener {

		@Override
		public void onClick(View arg0) {
			// TODO Auto-generated method stub
			Log.d(tag, "click Ok!");
			SystemProperties.set(SECURE_PROPERTY, "1");
			finish();
		}
	}

	class BtnNOKListener implements android.view.View.OnClickListener {

		@Override
		public void onClick(View arg0) {
			// TODO Auto-generated method stub
			Log.d(tag, "click not ok!");
			SystemProperties.set(SECURE_PROPERTY, "0");
			finish();
		}

	}
}
