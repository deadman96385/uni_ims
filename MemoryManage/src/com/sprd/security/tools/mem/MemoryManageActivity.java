package com.sprd.security.tools.mem;

import java.util.ArrayList;
import java.util.List;

import android.app.ActivityManager;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.app.AlertDialog;
import android.app.Service;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;
import android.os.Debug.MemoryInfo;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.view.View;

public class MemoryManageActivity extends BaseAmsActivity implements View.OnClickListener{
	/**
	 * listView
	 */
	private ListView mListView;
	/**
	 * listView
	 */
	private AppMemoryListAdapter appAdapter;
	/**
	 * top
	 */
	private View title;
	private ImageButton titleBack, titleRefresh;
	private TextView titleName;
	/**
	 * bottom
	 */
	private TextView freeMemory;
	private ActivityManager activityManager;
	/**
	 * running process info
	 */
	private List<RunningAppProcessInfo> listProcess;
	/**
	 * running process info name
	 */
	private List<String> listName;
	/**
	 * pids
	 */
	private int[] pIds;
	/**
	 * memoinfo of each process
	 */
	private MemoryInfo[] pMemoryInfos;
	private List<AppMemory> appList;
	/**
	 * get help info
	 */
	private TaskInfo taskInfo;
	private MemoryHandler mHandler;
	private MemoryThread mThread;
	private Context context;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_memory_manage);
		context=this;
		taskInfo = new TaskInfo(this);
		mHandler = new MemoryHandler();
		mThread = new MemoryThread();
		
		initView();
		initListener();
		mThread.start();
	}
	private void initData() {
		// TODO Auto-generated method stub
		if(activityManager == null){
			activityManager = (ActivityManager) getSystemService(Service.ACTIVITY_SERVICE);
		}
		if(listProcess != null){
			listProcess.clear();
			listProcess = null;
		}
		listProcess = activityManager.getRunningAppProcesses();
		if(pIds != null){
			pIds = null;;
		}
		pIds = getAllProcessId(listProcess);
		if(listName != null){
			listName.clear();
			listName = null;
		}
		listName = getAllProcessName(listProcess);
		if(pMemoryInfos != null){
			pMemoryInfos = null;
		}
		pMemoryInfos = activityManager.getProcessMemoryInfo(pIds);
		if(appList != null){
			appList.clear();
			appList = null;
		}
		appList = new ArrayList<AppMemory>();
		AppMemory app = null;
		for(int i = 0; i < pIds.length; i++){
			app = new AppMemory(pIds[i], listName.get(i), listProcess.get(i).processName,
					taskInfo.checkAppType(listProcess.get(i).processName), pMemoryInfos[i]);
			appList.add(app);
		}
		
		
	}
	private void initView(){
		title = (View) findViewById(R.id.title_memory);
		titleBack = (ImageButton) title.findViewById(R.id.title_left_bt);
		titleRefresh = (ImageButton) title.findViewById(R.id.title_right_bt);
		titleName = (TextView) title.findViewById(R.id.title_text);
		titleName.setText("title");
		
		mListView = (ListView)findViewById(R.id.lv_memory_manager_app);
		
		freeMemory = (TextView)findViewById(R.id.free_memory);
		
	}
	private void initListener(){
		titleBack.setOnClickListener(this);
		titleRefresh.setOnClickListener(this);
		mListView.setOnItemClickListener(new OnItemClickListener() {

			public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
				// TODO Auto-generated method stub
				final AppMemory appMemory = appList.get(position);
				AlertDialog.Builder builder =new AlertDialog.Builder(MemoryManageActivity.this) ;
				if(appMemory.isSystemApp()){
					builder.setTitle(appMemory.getName() + "is system app,close it will cause unstable?") ;
					builder.setPositiveButton(getResources().getString(R.string.diag_close), new DialogInterface.OnClickListener() {

						@Override
						public void onClick(DialogInterface dialog, int which) {
							// TODO Auto-generated method stub
							ActivityManager am = (ActivityManager) MemoryManageActivity.this.getSystemService(Service.ACTIVITY_SERVICE);
							am.killBackgroundProcesses(appMemory.getPackageName());
							mThread = new MemoryThread();
							mThread.start();
						}
						
					}).setNegativeButton(getResources().getString(R.string.diag_cancel), new OnClickListener() {
						
						@Override
						public void onClick(DialogInterface dialog, int which) {
							// TODO Auto-generated method stub
							dialog.cancel();
						}

					});
				}else{
					builder.setTitle(String.format(getResources().getString(R.string.dia_title), appMemory.getName())) ;
					builder.setPositiveButton(getResources().getString(R.string.diag_close), new DialogInterface.OnClickListener() {

						public void onClick(DialogInterface dialog, int which) {
							// TODO Auto-generated method stub
							ActivityManager am = (ActivityManager) MemoryManageActivity.this.getSystemService(Service.ACTIVITY_SERVICE);
							am.killBackgroundProcesses(appMemory.getPackageName());
							mThread = new MemoryThread();
							mThread.start();
						}
						
					}).setNegativeButton(getResources().getString(R.string.diag_cancel), new OnClickListener() {
						
						public void onClick(DialogInterface dialog, int which) {
							// TODO Auto-generated method stub
							dialog.cancel() ;  
						}
					});
				}
				builder.create().show() ;
			}
		});
	}
	private void refreshView(){
		if(appAdapter == null){
			appAdapter = new AppMemoryListAdapter(context, appList);
		}
		appAdapter.refreshData(appList);
		mListView.setAdapter(appAdapter);
		appAdapter.notifyDataSetChanged();
		
		displayBriefMemory();
	}
	
	private int[] getAllProcessId(List<RunningAppProcessInfo> processes){
		int[] ids = new int[processes.size()];
		for(int i = 0; i < processes.size(); i++){
			ids[i] = processes.get(i).pid;
		}
		return ids;
	}
	
	/**
	 * get all apps names
	 * @param listProcess
	 * @return
	 */
	private List<String> getAllProcessName(List<RunningAppProcessInfo> listProcess){
		List<String> listName = new ArrayList<String>();
		for(int i = 0; i < listProcess.size(); i++){
			String name = taskInfo.getAppName(listProcess.get(i).processName) + taskInfo.getAppVersion(listProcess.get(i).processName);
			listName.add(name);
		}
		
		return listName;
	}
	
	private void displayBriefMemory() { 
		activityManager = (ActivityManager) getSystemService(ACTIVITY_SERVICE); 
		ActivityManager.MemoryInfo info = new ActivityManager.MemoryInfo(); 
		activityManager.getMemoryInfo(info); 
		freeMemory.setText(Math.round((info.availMem/1024/1024f) * 100)/100f + "MB");
		} 
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		this.getMenuInflater().inflate(R.menu.memory_manage, menu);
		return true;
	}

	class MemoryThread extends Thread{

		@Override
		public void run() {
			// TODO Auto-generated method stub
			super.run();
			initData();
			Message msg = mHandler.obtainMessage();
			msg.what = 1;
			mHandler.sendMessage(msg);
		}
		
	}
	class MemoryHandler extends Handler{

		@Override
		public void handleMessage(Message msg) {
			// TODO Auto-generated method stub
			super.handleMessage(msg);
			if(msg.what == 1){
				refreshView();
			}else{
				Toast.makeText(context, "get process info fail", Toast.LENGTH_SHORT).show();
			}
		}
		
	}
	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		switch(v.getId()){
		case R.id.title_left_bt:{
			ActivityManager am = (ActivityManager) context.getSystemService(Service.ACTIVITY_SERVICE);
			for(int i = 0; i < appList.size(); i++){
				am.killBackgroundProcesses(appList.get(i).getPackageName());
			}
			mThread = new MemoryThread();
			mThread.start();
			break;
		}
		case R.id.title_right_bt:{
			mThread = new MemoryThread();
			mThread.start();
			break;
		}
		}
	}
	
}
