package edu.cmu.cs.cloudlet.android.util;

import java.io.IOException;
import android.content.Context;
import android.hardware.Camera;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

class Preview extends SurfaceView implements SurfaceHolder.Callback {

	SurfaceHolder mHolder;
	Camera mCamera = null;

	public void close() {
		if (mCamera != null) {
			mCamera.stopPreview();
			mCamera.release();
			mCamera = null;

		}
	}

	public Preview(Context context, AttributeSet attrs) {
		super(context, attrs);
		if (mCamera == null) {
			mCamera = Camera.open();
		}

		mHolder = getHolder();
		mHolder.addCallback(this);
		mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		// TODO Auto-generated method stub
		super.onMeasure(widthMeasureSpec, heightMeasureSpec);
	}

	public void surfaceCreated(SurfaceHolder holder) {

		if (mCamera == null) {
			mCamera = Camera.open();
		}
		if (mCamera != null) {
			try {
				mCamera.setPreviewDisplay(holder);
				Camera.Parameters parameters = mCamera.getParameters();
				parameters.setJpegQuality(60);
				mCamera.setParameters(parameters);
				mCamera.startPreview();
			} catch (IOException exception) {
				Log.e("Error", "exception:surfaceCreated Camera Open ");
				mCamera.release();
				mCamera = null;
				// TODO: add more exception handling logic here
			}
		}
	}

	public void surfaceDestroyed(SurfaceHolder holder) {
		if (mCamera != null) {
			mCamera.stopPreview();
			mCamera.release();
			mCamera = null;
		}
	}

	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
		Camera.Parameters parameters = mCamera.getParameters();
		parameters.setPreviewSize(w, h);
		mCamera.setParameters(parameters);
		mCamera.startPreview();
	}

}
