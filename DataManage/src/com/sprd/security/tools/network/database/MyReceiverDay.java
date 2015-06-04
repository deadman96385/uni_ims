
package com.sprd.security.tools.network.database;

import com.sprd.security.tools.network.Showmain;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.TrafficStats;


public class MyReceiverDay extends BroadcastReceiver {

	/* (non-Javadoc)
	 * @see android.content.BroadcastReceiver#onReceive(android.content.Context, android.content.Intent)
	 */
	@Override
	public void onReceive(Context context, Intent intent) {
		DataSupport minsert = new DataSupport(context);
		long g3_down_total = TrafficStats.getMobileRxBytes();
		long g3_up_total = TrafficStats.getMobileTxBytes(); 
		long mrdown_total = TrafficStats.getTotalRxBytes(); 
		long mtup_total = TrafficStats.getTotalTxBytes(); 
		minsert.insertNow(g3_down_total, Showmain.RXG, Showmain.RX3G, Showmain.NORMAL);
		minsert.insertNow(g3_up_total, Showmain.TXG, Showmain.TX3G, Showmain.NORMAL);
		minsert.insertNow(mrdown_total, Showmain.RX, Showmain.RXT, Showmain.NORMAL);
		minsert.insertNow(mtup_total, Showmain.TX, Showmain.TXT, Showmain.NORMAL);
	}

}
