package com.sprd.security.tools.mem;

import java.lang.ref.SoftReference;
import java.util.HashMap;
import java.util.List;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;


public class AppMemoryListAdapter extends BaseAdapter{
	private HashMap<String, SoftReference<Drawable>> imageCache;
	private Context context;
	private List<AppMemory> appList;
	private TaskInfo taskInfo;
	
	public AppMemoryListAdapter(Context con, List<AppMemory> apps){
		this.context = con;
		this.appList = apps;
		this.imageCache = new HashMap<String, SoftReference<Drawable>>();
		this.taskInfo = new TaskInfo(con);
	}
	
	private Drawable getImageDrawable(AppMemory app){
		String id = String.valueOf(app.getId());
		String pName = app.getPackageName();
		Drawable drawable = null;
		if(imageCache.containsKey(id)){
			SoftReference<Drawable> softRef = imageCache.get(id);
			drawable = softRef.get();
			if(drawable == null){
				imageCache.remove(id);
				drawable = taskInfo.getAppIcon(pName);
				if(drawable != null){   
			        imageCache.put(id, new SoftReference<Drawable>(drawable));   
			    }
			}
		}else{
			drawable = taskInfo.getAppIcon(pName);
		    if(drawable != null){   
		        imageCache.put(id, new SoftReference<Drawable>(drawable));   
		    }
		}
		System.out.println("appname:" + app.getName() + " pachagename: " + pName);
		System.out .println("image" + drawable);
		return drawable;
	}
	
	public void refreshData(List<AppMemory> apps){
		this.appList = apps;
	}
	
	@Override
	public int getCount() {
		// TODO Auto-generated method stub
		return appList.size();
	}

	@Override
	public Object getItem(int position) {
		// TODO Auto-generated method stub
		return appList.get(position);
	}

	@Override
	public long getItemId(int position) {
		// TODO Auto-generated method stub
		return position;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		// TODO Auto-generated method stub
		HolderView holder;
		if (null == convertView) {
			holder = new HolderView();
			convertView = LayoutInflater.from(context).inflate(R.layout.layout_app_memory_item, null);
			holder.ivAppIcon = (ImageView) convertView.findViewById(R.id.iv_app_icon);
			holder.tvAppName = (TextView) convertView.findViewById(R.id.tv_app_name);
			holder.tvAppType = (TextView) convertView.findViewById(R.id.tv_app_type);
			holder.tvAppDirty = (TextView) convertView.findViewById(R.id.tv_app_dirty);
			convertView.setTag(holder);
		} else {
			holder = (HolderView) convertView.getTag();
		}
		AppMemory app = appList.get(position);
		holder.ivAppIcon.setImageDrawable(this.getImageDrawable(app));
		holder.tvAppName.setText(app.getName());
		float privateDirty = 0.0f;
		privateDirty = Math.round((app.getMemoryInfo().getTotalPrivateDirty()/1024f) * 100)/100f;
		holder.tvAppDirty.setText(privateDirty + "MB");
		System.out.println(app.getMemoryInfo().getTotalPrivateDirty());
		if(app.isSystemApp()){
			holder.tvAppType.setText(context.getString(R.string.sysprocess));
			holder.tvAppName.setTextColor(context.getResources().getColor(R.color.red));
			holder.tvAppType.setTextColor(context.getResources().getColor(R.color.red));
			holder.tvAppDirty.setTextColor(context.getResources().getColor(R.color.red));
		}else{
			holder.tvAppType.setText(context.getString(R.string.appprocess));
			holder.tvAppName.setTextColor(context.getResources().getColor(R.color.green));
			holder.tvAppType.setTextColor(context.getResources().getColor(R.color.green));
			holder.tvAppDirty.setTextColor(context.getResources().getColor(R.color.red));
		}
		return convertView;
	}
	class HolderView {
		ImageView ivAppIcon;
		TextView tvAppName;
		TextView tvAppType;
		TextView tvAppDirty;
	}
}
