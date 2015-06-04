package com.sprd.security.tools.mem;

import android.os.Bundle;

/**
 * activity base ui
 * @author gary.gong
 */
public class BaseAmsActivity extends ActionBarActivity {
	
	protected void onCreate(Bundle bundle) {
//		setTheme(R.style.AppTheme);
		boolean isThemeChange = getSharedPreferences("system_config", MODE_PRIVATE)
				.getBoolean("theme_value", true);
		if (isThemeChange) {
			setTheme(R.style.Theme_RealWorld);
		} else {
			setTheme(R.style.AppTheme);
		}
		
		super.onCreate(bundle);
	}

}
