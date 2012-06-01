package edu.cmu.cs.cloudlet.android.application.graphics;

import java.nio.ByteBuffer;
import java.util.Random;
import java.util.Vector;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import edu.cmu.cs.cloudlet.android.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.OvalShape;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;

public class ParticleDrawView extends SurfaceView implements SurfaceHolder.Callback {
	public static final int PARTICLE_RADIUS = 10;
	public static final int PARTICLE_NUMBER = 2000;
	int frameCounter = 0;

	private DrawThread drawThread;
	private final Object lock = new Object();
	private ShapeDrawable[] particles = null;


	public ParticleDrawView(Context context) {
		super(context);
		getHolder().addCallback(this);        
		drawThread = new DrawThread(getHolder(), this);
	}

    private Animation translate; 
	private void createTranslate(ShapeDrawable mDrawable) {
		translate = new TranslateAnimation(1, 100, 1, 100);
		translate.setRepeatCount(1);
		translate.setDuration(2000);
	}

	protected void onDraw(Canvas canvas) {
		canvas.drawColor(Color.BLACK);

		synchronized(this.lock){
			if(particles == null || particles.length == 0){
				// Did not received any data yet
				return;
			}

			for(int i = 0; i < this.particles.length; i++){
				this.particles[i].draw(canvas);
			}
//			for(int i = 100; i < 200; i++){
//				this.particles[i].draw(canvas);
//			}
		}

		if (++frameCounter % 50 == 0) {
			Log.d("krha", "time : " + System.currentTimeMillis());
		}
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		// TODO Auto-generated method stub
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		this.drawThread.setRunning(true);
		this.drawThread.start();
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		boolean retry = true;
		this.drawThread.setRunning(false);
		while (retry) {
			try {
				this.drawThread.join();
				retry = false;
			} catch (InterruptedException e) {
			}
		}
	}

	class DrawThread extends Thread {
		private SurfaceHolder surfaceHolder;
		private ParticleDrawView drawPanel;

		private boolean isRunning = false;

		public DrawThread(SurfaceHolder surfaceHolder, ParticleDrawView drawPanel) {
			this.surfaceHolder = surfaceHolder;
			this.drawPanel = drawPanel;
		}

		public void setRunning(boolean run) {
			this.isRunning = run;
		}

		@Override
		public void run() {
			Canvas c;
			while (isRunning) {
				try {
					// Don't hog the entire CPU
					Thread.sleep(1);
				} catch (InterruptedException e) {
				}

				c = null;
				try {
					c = surfaceHolder.lockCanvas();
					if (c != null) {
						synchronized (surfaceHolder) {
							this.drawPanel.onDraw(c);
						}
					}
				} finally {
					if (c != null) {
						surfaceHolder.unlockCanvasAndPost(c);
					}
				}
			}
		}
	}

	public void updatePosition(ByteBuffer buffer) {
        synchronized (lock){
	        int particleSize = buffer.limit()/(8*2+4);
	        if(this.particles == null || this.particles.length < particleSize){
	        	this.particles = new ShapeDrawable[particleSize];
	        	for(int i = 0; i < this.particles.length; i ++){
		        	ShapeDrawable newParticle =  new ShapeDrawable(new OvalShape());
	        		newParticle.setBounds(0, 0, PARTICLE_RADIUS, PARTICLE_RADIUS);
		        	this.particles[i] = newParticle;
	        	}
	        }
	
			
	    	final int color_offset = particleSize * (8*2);	    	
			final double w_scale = 1.0*VisualizationStaticInfo.screenWidth/VisualizationStaticInfo.containerWidth;
			final double h_scale = 1.0*VisualizationStaticInfo.screenHeight/VisualizationStaticInfo.containerHeight;
			
			for(int i = 0; i < this.particles.length; i++){
	        	double x = buffer.getDouble(i*16) * w_scale;
	        	double y = buffer.getDouble(i*16 + 8) * h_scale;
	        	int color_int = buffer.getInt(i*4 + color_offset);

				particles[i].setBounds((int)x, (int)y, (int)x + PARTICLE_RADIUS, (int)y + PARTICLE_RADIUS);

	        	byte[] bytes = ByteBuffer.allocate(4).putInt(color_int).array();
				particles[i].getPaint().setARGB((int)(0x00ff&bytes[3]), (int)(0x00ff&bytes[0]), (int)(0x00ff&bytes[1]), (int)(0x00ff&bytes[2]));				
	        }

	    	double x1 = buffer.getDouble(0) * w_scale;
	    	double y1 = buffer.getDouble(8) * h_scale;
        	int color = buffer.getInt(color_offset);
        	byte[] bytes = ByteBuffer.allocate(4).putInt(color).array();
			Log.d("krha", "#: " + particleSize + ", position : (" + x1 + ", " + y1 + "), red=" + (int)(0x00ff&bytes[0]) + " g=" + (int)(0x00ff&bytes[1])+" b="+(int)(0x00ff&bytes[2]) + " alpah="+(int)(0x00ff&bytes[3]));

        }
		
		try {
			Thread.sleep(1);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}
	
	float[] colorRamp(float t) {
		final int ncolors = 6;
		float c[][] = {
			{ 1.0f, 0.0f, 0.0f, },
			{ 1.0f, 0.5f, 0.0f, },
			{ 1.0f, 1.0f, 0.0f, },
			{ 0.0f, 1.0f, 0.0f, },
			{ 0.0f, 1.0f, 1.0f, },
			{ 0.0f, 0.0f, 1.0f, },
		};
		t = t * (ncolors-1);
		int i = (int) t;
		float u = (float) (t - Math.floor(t));
		float[] rgba = {1.f, 1.f, 1.f, 1.f};
		/*
		rgba[0] = lerp(c[i][0], c[i+1][0], u);
		rgba[1] = lerp(c[i][1], c[i+1][1], u);
		rgba[2] = lerp(c[i][2], c[i+1][2], u);
		rgba[3] = 1.f;
		*/
		return rgba;
	}

}
