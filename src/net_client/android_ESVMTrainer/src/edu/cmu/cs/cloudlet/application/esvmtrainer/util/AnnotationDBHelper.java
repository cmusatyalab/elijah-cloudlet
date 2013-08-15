package edu.cmu.cs.cloudlet.application.esvmtrainer.util;

import java.io.File;
import java.util.LinkedList;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.database.sqlite.SQLiteOpenHelper;
import android.graphics.Rect;
import android.util.Log;

public class AnnotationDBHelper extends SQLiteOpenHelper {

	public static final int DB_VERSION = 3;
	public static final String DB_FILENAME = "annotation.sql";
	public static final String TABLE_NAME = "ANNOTATION";
	public static final String COLUMN_ORIGINAL_FILENAME = "original_file";
	public static final String COLUMN_CROP_FILENAME = "cropped_file";
	public static final String COLUMN_ANNOTATION_LEFT = "left";
	public static final String COLUMN_ANNOTATION_RIGHT = "right";
	public static final String COLUMN_ANNOTATION_TOP = "top";
	public static final String COLUMN_ANNOTATION_BOTTOM = "bottom";

	private SQLiteDatabase database = null;
	private Context context;

	// Database creation sql statement
	private static final String DATABASE_CREATE = "create table " + TABLE_NAME + "(" + COLUMN_ORIGINAL_FILENAME
			+ " TEXT primary key, " + COLUMN_CROP_FILENAME + " TEXT, " + COLUMN_ANNOTATION_LEFT + " integer,"
			+ COLUMN_ANNOTATION_RIGHT + " integer, " + COLUMN_ANNOTATION_TOP + " integer, " + COLUMN_ANNOTATION_BOTTOM
			+ " integer" + ");";

	public AnnotationDBHelper(Context context) {
		super(context, DB_FILENAME, null, DB_VERSION);
		this.context = context;
	}

	public AnnotationDBHelper(Context context, String name, CursorFactory factory, int version) {
		super(context, name, factory, version);
	}

	@Override
	public void onCreate(SQLiteDatabase database) {
		// TODO Auto-generated method stub
		database.execSQL(DATABASE_CREATE);
		this.database = database;
	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		// TODO Auto-generated method stub
		Log.w(AnnotationDBHelper.class.getName(), "Upgrading database from version " + oldVersion + " to " + newVersion
				+ ", which will destroy all old data");
		db.execSQL("DROP TABLE IF EXISTS " + TABLE_NAME);
		onCreate(db);
	}

	public void updateImageFromDB(LinkedList<AnnotatedImageDS> imageList) {
		this.database = getReadableDatabase();

		Cursor cursor = this.database.query(AnnotationDBHelper.TABLE_NAME, null,
				AnnotationDBHelper.COLUMN_ANNOTATION_LEFT + " IS NOT NULL", null, null, null, null, null);
		if (cursor != null)
			cursor.moveToFirst();

		while (!cursor.isAfterLast()) {
			String originFile = cursor.getString(cursor.getColumnIndex(AnnotationDBHelper.COLUMN_ORIGINAL_FILENAME));
			for (int i = 0; i < imageList.size(); i++) {
				AnnotatedImageDS imageDS = imageList.get(i);
				if (imageDS.getOriginalFile().getAbsolutePath().equals(originFile)) {
					// matching file
					String cropFile = cursor.getString(cursor.getColumnIndex(AnnotationDBHelper.COLUMN_CROP_FILENAME));
					int left = cursor.getInt(cursor.getColumnIndex(AnnotationDBHelper.COLUMN_ANNOTATION_LEFT));
					int right = cursor.getInt(cursor.getColumnIndex(AnnotationDBHelper.COLUMN_ANNOTATION_RIGHT));
					int bottom = cursor.getInt(cursor.getColumnIndex(AnnotationDBHelper.COLUMN_ANNOTATION_BOTTOM));
					int top = cursor.getInt(cursor.getColumnIndex(AnnotationDBHelper.COLUMN_ANNOTATION_TOP));
					imageDS.setCropRect(new Rect(left, top, right, bottom));
				}
			}
			cursor.moveToNext();
		}
		cursor.close();
		this.database.close();
	}

	public void updateDB(AnnotatedImageDS processedImage) {
		// TODO Auto-generated method stub
		Rect annotRect = processedImage.getCropRect();
		ContentValues values = new ContentValues();
		values.put(AnnotationDBHelper.COLUMN_ORIGINAL_FILENAME, processedImage.getOriginalFile().getAbsolutePath());
		values.put(AnnotationDBHelper.COLUMN_CROP_FILENAME, processedImage.getCropFile().getAbsolutePath());
		values.put(AnnotationDBHelper.COLUMN_ANNOTATION_LEFT, annotRect.left);
		values.put(AnnotationDBHelper.COLUMN_ANNOTATION_RIGHT, annotRect.right);
		values.put(AnnotationDBHelper.COLUMN_ANNOTATION_BOTTOM, annotRect.bottom);
		values.put(AnnotationDBHelper.COLUMN_ANNOTATION_TOP, annotRect.top);

		this.database = getWritableDatabase();
		this.database.insertWithOnConflict(AnnotationDBHelper.TABLE_NAME, null, values, SQLiteDatabase.CONFLICT_REPLACE);
		this.database.close();
	}

	public void close() {
		if (this.database != null) {
			this.database.close();
		}
	}

	public JSONObject getJSONAnnotation(LinkedList<AnnotatedImageDS> imageList) {
		JSONObject retObject = new JSONObject();
		JSONArray annotationArray = new JSONArray();
		for (int i = 0; i < imageList.size(); i++) {
			AnnotatedImageDS imageDS = imageList.get(i);
			File image = imageDS.getOriginalFile();
			Rect annotation = imageDS.getCropRect();
			if (annotation == null){
				return null;
			}
			
			JSONObject newObj = new JSONObject();
			try {
				newObj.put(COLUMN_ORIGINAL_FILENAME, image.getName());
				newObj.put(COLUMN_ANNOTATION_LEFT, annotation.left);
				newObj.put(COLUMN_ANNOTATION_RIGHT, annotation.right);
				newObj.put(COLUMN_ANNOTATION_BOTTOM, annotation.bottom);
				newObj.put(COLUMN_ANNOTATION_TOP, annotation.top);
				annotationArray.put(newObj);
			} catch (JSONException e) {
				return null;
			}
		}
		
		try {
			retObject.put(TABLE_NAME, annotationArray);
			return retObject;
		} catch (JSONException e) {
			return null;
		}
	}
}
