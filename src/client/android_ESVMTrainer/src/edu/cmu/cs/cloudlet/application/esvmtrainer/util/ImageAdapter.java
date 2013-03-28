package edu.cmu.cs.cloudlet.application.esvmtrainer.util;

import java.io.File;
import java.util.LinkedList;

import edu.cmu.cs.cloudlet.application.esvmtrainer.AnnotationActivity;
import edu.cmu.cs.cloudlet.application.esvmtrainer.R;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.drawable.BitmapDrawable;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;

public class ImageAdapter extends BaseAdapter {

	private final int GRID_IMAGE_WIDTH = 100;
	private final int GRID_IMAGE_HEIGHT = 100;

	private Context mContext;
	private LinkedList<AnnotatedImageDS> cropImageList;

	// Constructor
	public ImageAdapter(Context c, LinkedList<AnnotatedImageDS> cropImageList) {
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
		AnnotatedImageDS cropImage = cropImageList.get(position);
		Bitmap bitmap = null;
		MarkableImageView imageView = new MarkableImageView(mContext);
		if (cropImage.isCrop()) {
			// show cropped region overlapped with icon
			File imageFile = cropImage.getCropFile();
			bitmap = BitmapFactory.decodeFile(imageFile.getAbsolutePath());
			imageView.setChecked(true);
		} else {
			File imageFile = cropImage.getOriginalFile();
			bitmap = decodeSampledBitmapFromResource(
					imageFile.getAbsolutePath(), GRID_IMAGE_WIDTH,
					GRID_IMAGE_HEIGHT);
		}
		imageView.setImageBitmap(bitmap);
		imageView.setScaleType(ImageView.ScaleType.FIT_XY);
		imageView.setLayoutParams(new GridView.LayoutParams(GRID_IMAGE_WIDTH,
				GRID_IMAGE_HEIGHT));
		return imageView;
	}

	public static Bitmap decodeSampledBitmapFromResource(String pathName,
			int reqWidth, int reqHeight) {

		// First decode with inJustDecodeBounds=true to check dimensions
		final BitmapFactory.Options options = new BitmapFactory.Options();
		options.inJustDecodeBounds = true;
		BitmapFactory.decodeFile(pathName, options);

		// Calculate inSampleSize
		options.inSampleSize = calculateInSampleSize(options, reqWidth,
				reqHeight);

		// Decode bitmap with inSampleSize set
		options.inJustDecodeBounds = false;
		return BitmapFactory.decodeFile(pathName, options);
	}

	public static int calculateInSampleSize(BitmapFactory.Options options,
			int reqWidth, int reqHeight) {
		// Raw height and width of image
		final int height = options.outHeight;
		final int width = options.outWidth;
		int inSampleSize = 1;

		if (height > reqHeight || width > reqWidth) {
			// Calculate ratios of height and width to requested height and
			// width
			final int heightRatio = Math.round((float) height
					/ (float) reqHeight);
			final int widthRatio = Math.round((float) width / (float) reqWidth);

			inSampleSize = heightRatio < widthRatio ? heightRatio : widthRatio;
		}

		return inSampleSize;
	}

	private Bitmap overlayBitmap(Bitmap bmp1, Bitmap bmp2) {
		Bitmap bmOverlay = Bitmap.createBitmap(bmp1.getWidth(),
				bmp1.getHeight(), bmp1.getConfig());
		Canvas canvas = new Canvas(bmOverlay);
		canvas.drawBitmap(bmp1, new Matrix(), null);
		canvas.drawBitmap(bmp2, new Matrix(), null);
		return bmOverlay;
	}
}

class MarkableImageView extends ImageView {
	private boolean checked = false;

	public MarkableImageView(Context context) {
		super(context);
	}

	public MarkableImageView(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	public MarkableImageView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
	}

	public void setChecked(boolean checked) {
		this.checked = checked;
		invalidate();
	}

	public boolean isChecked() {
		return checked;
	}

	@Override
	protected void onDraw(Canvas canvas) {
		super.onDraw(canvas);
		if (checked) {
			Bitmap b = BitmapFactory.decodeResource(getResources(),
					R.drawable.checkmark_done);
			int cwidth = canvas.getWidth() / 8;
			int cheight = canvas.getHeight() / 8;
			Bitmap check = Bitmap.createScaledBitmap(b, (int) (cwidth / 2),
					(int) (cheight / 2), false);
			canvas.drawBitmap(check, 0, 0, new Paint());
		}
	}
}