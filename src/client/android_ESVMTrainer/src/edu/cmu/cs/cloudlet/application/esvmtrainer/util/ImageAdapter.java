package edu.cmu.cs.cloudlet.application.esvmtrainer.util;

import java.io.File;
import java.util.LinkedList;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;

public class ImageAdapter extends BaseAdapter{
	private Context mContext;
	private LinkedList<CropImageDS> cropImageList;
 
    // Constructor
    public ImageAdapter(Context c, LinkedList<CropImageDS> cropImageList){
        this.mContext = c;
        this.cropImageList = cropImageList;
    }
 
    @Override
    public int getCount() {
        return cropImageList.size();
    }
 
    @Override
    public Object getItem(int position) {
        return cropImageList.get(position);
    }
 
    @Override
    public long getItemId(int position) {
        return 0;
    }
 
    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
    	CropImageDS cropImage = cropImageList.get(position);
    	File imageFile = null;
    	if (cropImage.isCrop()){
    		imageFile = cropImage.getCropFile();
    	}else{
    		imageFile = cropImage.getOriginalFile();
    	}
    	
        ImageView imageView = new ImageView(mContext);
        Bitmap bitmap = BitmapFactory.decodeFile(imageFile.getAbsolutePath());
        imageView.setImageBitmap(bitmap);
        imageView.setScaleType(ImageView.ScaleType.FIT_CENTER);
        imageView.setLayoutParams(new GridView.LayoutParams(200, 200));
        return imageView;
    }
}
