package com.sprd.security.tools.mem;

import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.drawable.Drawable;

public class TaskInfo {  
    Context context ;  
    PackageManager pm ;  
    public TaskInfo(Context context) {  
        this.context = context;  
        pm = context.getPackageManager();  
    } 
    

    public boolean checkAppType(String packName){
    	boolean flag = true;
    	ApplicationInfo info;
		try {
			info = pm.getApplicationInfo(packName, 0);
			if((info.flags & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0){
		       	 flag = false;
	        }else if ((info.flags & ApplicationInfo.FLAG_SYSTEM) == 0) { 
	       	 	flag = false;
	        }
		} catch (NameNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}   
		return flag;
    }
    
    public Drawable getAppIcon(String packname){  
      try {  
             ApplicationInfo info = pm.getApplicationInfo(packname, 0);   
             return info.loadIcon(pm);  
        } catch (NameNotFoundException e) {  
            // TODO Auto-generated catch block   
            e.printStackTrace();  
            return null;  
        }  
    }  
      
    public Drawable getAppIcon2(String packName){
    	Intent mainIntent = new Intent(Intent.ACTION_MAIN, null);
		mainIntent.addCategory(Intent.CATEGORY_LAUNCHER);
		List<ResolveInfo> resolveInfos = pm.queryIntentActivities(mainIntent, 0);
		for (ResolveInfo reInfo : resolveInfos) {
			if(reInfo.activityInfo.packageName == packName){
				return reInfo.loadIcon(pm);
			}
		}
		return null;
    }
    

    public String getAppVersion(String packname){  
          
          try {  
              PackageInfo packinfo =    pm.getPackageInfo(packname, 0);
              if(null == packinfo.versionName){
            	  return "";
              }else{
            	  return packinfo.versionName;  
              }
            } catch (NameNotFoundException e) {  
                e.printStackTrace();  
                return "";  
            }  
    }  
      
  

    public String getAppName(String packname){  
          try {  
                 ApplicationInfo info = pm.getApplicationInfo(packname, 0);  
                 if(null == info.loadLabel(pm).toString()){
                	 return packname;
                 }else{
                	 return info.loadLabel(pm).toString();  
                 }
            } catch (NameNotFoundException e) {  
                // TODO Auto-generated catch block   
                e.printStackTrace();  
                return packname;  
            }  
    }  

    public String[] getAppPremission(String packname){  
          try {  
              PackageInfo packinfo =    pm.getPackageInfo(packname, PackageManager.GET_PERMISSIONS);      
              return packinfo.requestedPermissions;  
  
            } catch (NameNotFoundException e) {  
                e.printStackTrace();  
                return null;  
            }  
    }  
      
      
    /* 
     * get apk sign  
     */  
    public String getAppSignature(String packname){  
          try {  
              PackageInfo packinfo =    pm.getPackageInfo(packname, PackageManager.GET_SIGNATURES);  
              //get all permission    
              return packinfo.signatures[0].toCharsString();  
  
            } catch (NameNotFoundException e) {  
                e.printStackTrace();  
                return null;  
            }  
    }  
}  
