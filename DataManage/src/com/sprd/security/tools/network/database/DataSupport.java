
package com.sprd.security.tools.network.database;

import com.sprd.security.tools.network.Showmain;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

/**
 * database
 *
 */
public class DataSupport extends SQLiteOpenHelper {

	/**
	 * @param context
	 * @param name
	 * @param factory
	 * @param version
	 */
	public DataSupport(Context context) {
		// create liuliangdata database
		super(context, "liuliangdata", null, 1);
    if(Showmain.isLog){
		Log.i("liuliang","support>>>>>>>>start");
    }
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * android.database.sqlite.SQLiteOpenHelper#onCreate(android.database.sqlite
	 * .SQLiteDatabase)
	 */
	@Override
	public void onCreate(SQLiteDatabase db) {
		// create table liuliangtable
		String sql = "create table liuliangtable (id integer primary key autoincrement,date datetime not null ,liuliang integer ,type text,typename text,history text)";
		db.execSQL(sql);
		String sqlbiaozhi = "create table biaozhi (id integer primary key autoincrement,date datetime not null ,flagtype text,flagtypename text)";
		db.execSQL(sqlbiaozhi);
	    if(Showmain.isLog){
			Log.i("liuliang","onCreate>>>>>>>>>>>start");
	    };
	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		// TODO Auto-generated method stub
		String sql = "drop table if exits liuliangtable ";
		db.execSQL(sql);
		onCreate(db);
	    if(Showmain.isLog){
			Log.i("liuliang","onupgrade>>>>>>>>>start");
	    }
	}
/**
 * 
 * check shutdown record
 *
 * @param type
 * @param history
 * @return
 */
	public Cursor selectday(String type, String history) {
		SQLiteDatabase db = getReadableDatabase();
	
		String sql="select date,typename,liuliang from liuliangtable where type = ? and history=?";
		Cursor daycursor = db.rawQuery(sql, new String[]{type,history});
	    if(Showmain.isLog){
			Log.i("liuliang","selectday>>>>>>>>>>uppstart");
	    }
		return daycursor;
	}
	/**
	 * 
	 * check record in specific time
	 *
	 * @param type
	 * @param history
	 * @param datestart
	 * @param dateover
	 * @return
	 */
	public Cursor selectbetweenday(String type, String history,String datestart,String dateover) {
		SQLiteDatabase db = getReadableDatabase();
		String sql="select date,typename,liuliang from liuliangtable where type = ? and history=? and date between datetime(?) and datetime(?)";
		Cursor daycursor = db.rawQuery(sql, new String[]{type,history,datestart,dateover});
		if(Showmain.isLog){
			Log.i("liuliang","selectbetweenday>>>>>>>>>>uppstart");
		}
		return daycursor;
	}
    /**
     * 
     * get latest data
     *
     * @param type
     * @return
     */
	public Cursor selectNow(String type) {
		SQLiteDatabase db = getReadableDatabase();
		String sql = "select * from liuliangtable where id in (select max(id) from liuliangtable where type = ?)";
		Cursor nowcursor = db.rawQuery(sql, new String[] { type });
		if(Showmain.isLog){
			Log.i("liuliang","selectNow>>>>>>>>>>uppstart");
	    }
		return nowcursor;
	}
	/**
	 * 
	 * check if first install
	 *
	 * @param flagtype
	 * @return
	 */
	public Cursor selectbiaozhi(String flagtype) {
		SQLiteDatabase db = getReadableDatabase();
		String sql = "select * from biaozhi where flagtype = ?";
		Cursor nowcursor = db.rawQuery(sql, new String[] { flagtype });
		if(Showmain.isLog){
			Log.i("liuliang","selectbiaozhi>>>>>>>>>>uppstart");
		}
		return nowcursor;
	}
	/**
	 * 
	 * get data from two specific time
	 *
	 * @param datestart
	 * @param datestop
	 * @param type
	 * @return
	 */
public Cursor selectBettweenstart(String datestart,String datestop ,String type){
	
	SQLiteDatabase db=getReadableDatabase();
   String sql="select * from liuliangtable where id in (select min(id) from liuliangtable where type=? and date between datetime(?) and datetime(?))";
	Cursor cursor = db.rawQuery(sql, new String[]{type,datestart,datestop});
	if(Showmain.isLog){
		Log.i("liuliang","selectBettweenstart>>>>>>>>>>uppstart");
    }
   return cursor;
	
}
/**
 * 
 * get the largest data from two specific time
 *
 * @param datestart
 * @param datestop
 * @param type
 * @return
 */
public Cursor selectBettweenstop(String datestart,String datestop ,String type){
	
	SQLiteDatabase db=getReadableDatabase();
	String sql="select * from liuliangtable where id in (select max(id)from liuliangtable where type=? and date between datetime(?) and datetime(?))";
	Cursor cursor = db.rawQuery(sql, new String[]{type,datestart,datestop});
	if(Showmain.isLog){
		Log.i("liuliang","selectBettweenstop>>>>>>>>>>uppstart");
    }
	return cursor;
	
}
/**
 * 
 *insert data
 * @param liuliang
 * @param type
 * @param typename
 * @param history
 */
	public void insertNow( long  liuliang, String type, String typename,
			String history) {
		SQLiteDatabase db = getWritableDatabase();
		String insertstr="insert into liuliangtable(date,liuliang,type,typename,history) values(datetime('now'),?,?,?,?) ";
		db.execSQL(insertstr, new Object[]{liuliang,type,typename,history});
		db.close();
		if(Showmain.isLog){
			Log.i("liuliang","insertNow>>>>>>>>>>uppstart");
	    }
	}
	/**
	 * 
	 * insert mark of close system 
	 *
	 * @param flagtype
	 * @param flagtypename
	 */
	public void insertbiaozhi(String flagtype, String flagtypename) {
		SQLiteDatabase db = getWritableDatabase();
		String insertbiaozhi="insert into biaozhi(date,flagtype,flagtypename) values(datetime('now'),?,?) ";
		db.execSQL(insertbiaozhi, new Object[]{flagtype,flagtypename});
		db.close();
		if(Showmain.isLog){
			Log.i("liuliang","insertbiaozhi>>>>>>>>>>uppstart");
		}
	}
}
