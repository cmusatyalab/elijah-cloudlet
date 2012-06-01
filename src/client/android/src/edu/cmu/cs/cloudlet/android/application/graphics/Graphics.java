package edu.cmu.cs.cloudlet.android.application.graphics;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.Random;

import javax.microedition.khronos.opengles.GL10;

import android.util.Log;

public class Graphics {

	protected FloatBuffer vertexBuffer;
	protected FloatBuffer colorBuffer;
	protected int particleNumber = 0;
	private String lock = "lock";

	
	public Graphics() {
		/**
		 * initialize
		 */
		// /**
		// * byte array into bytbe buffer test
		// */
		// byte[] barr = toByta(vertices);
		// ByteBuffer byteBuf = ByteBuffer.wrap(barr);
		// byteBuf.order(ByteOrder.nativeOrder());
		// vertexBuffer = byteBuf.asFloatBuffer();
		// vertexBuffer.position(0);

		ByteBuffer byteBuf = ByteBuffer.allocateDirect(1000);
		byteBuf.order(ByteOrder.nativeOrder());
		vertexBuffer = byteBuf.asFloatBuffer();
		vertexBuffer.position(0);

	}

	public void draw(GL10 gl) {
		if(this.particleNumber <= 0)
			return;

		synchronized(lock){
			gl.glFrontFace(GL10.GL_CW);
			gl.glPointSize(10);
			gl.glVertexPointer(2, GL10.GL_FLOAT, 0, vertexBuffer);
			gl.glColorPointer(4, GL10.GL_FLOAT, 0, colorBuffer);
			gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
			gl.glEnableClientState(GL10.GL_COLOR_ARRAY);
			gl.glDrawArrays(GL10.GL_POINTS, 0, this.particleNumber); // draw points
			gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
			gl.glDisableClientState(GL10.GL_COLOR_ARRAY);
		}
	}

	public static byte[] toByta(float data) {
		return toByta(Float.floatToRawIntBits(data));
	}

	public static byte[] toByta(float[] data) {
		if (data == null)
			return null;
		// ----------
		byte[] byts = new byte[data.length * 4];
		for (int i = 0; i < data.length; i++)
			System.arraycopy(toByta(data[i]), 0, byts, i * 4, 4);
		return byts;
	}

	public void updatePosition(ByteBuffer buffer) {
		synchronized (lock) {
			int particleSize = buffer.limit() / (8 * 2 + 4);
			if (this.vertexBuffer == null || this.particleNumber < particleSize) {
				this.particleNumber = particleSize;
				
				// Inint Vertext Buffer
				int buffer_size = this.particleNumber*(Float.SIZE/8)*2;
				ByteBuffer tempBuf = ByteBuffer.allocateDirect(this.particleNumber*(Float.SIZE/8)*2);				
				tempBuf.order(ByteOrder.nativeOrder());
				vertexBuffer = tempBuf.asFloatBuffer();
				vertexBuffer.position(0);
				
				// Inint Vertext Color Buffer
				tempBuf = ByteBuffer.allocateDirect(this.particleNumber*(Float.SIZE/8)*4);
				tempBuf.order(ByteOrder.nativeOrder());
				colorBuffer = tempBuf.asFloatBuffer();
				colorBuffer.position(0);
			}

			final int color_offset = particleSize * (8 * 2);
			final double w_scale = (1.0 * VisualizationStaticInfo.containerWidth);
			final double h_scale = (1.0 * VisualizationStaticInfo.containerHeight);

			for (int i = 0; i < this.particleNumber; i++) {
				double x = (buffer.getDouble(i * 16) / (VisualizationStaticInfo.containerWidth/2)) -1;
				double y = (buffer.getDouble(i * 16 + 8) / (VisualizationStaticInfo.containerHeight/2)) -1;
				int color_int = buffer.getInt(i * 4 + color_offset);
				byte[] bytes = ByteBuffer.allocate(4).putInt(color_int).array();
				vertexBuffer.put(i*2, (float)x);
				vertexBuffer.put(i*2+1, (float)y);
				colorBuffer.put(i*4,(float)(0x00ff & bytes[0])/255.0f);
				colorBuffer.put(i*4+1,(float)(0x00ff & bytes[1])/255.0f);
				colorBuffer.put(i*4+2,(float)(0x00ff & bytes[2])/255.0f);
				colorBuffer.put(i*4+3,(float)(0x00ff & bytes[3])/255.0f);
			}

			double x1 = buffer.getDouble(0) * w_scale;
			double y1 = buffer.getDouble(8) * h_scale;
			int color = buffer.getInt(color_offset);
			byte[] bytes = ByteBuffer.allocate(4).putInt(color).array();
//			Log.d("krha", "#: " + particleSize + ", position : (" + x1 + ", " + y1 + "), red="
//					+ (float)(0x00ff & bytes[0])/255.0f + " g=" + (float)(0x00ff & bytes[1])/255.0f + " b=" + (float)(0x00ff & bytes[2])/255.0f
//					+ " alpah=" + (float)(0x00ff & bytes[3])/255.0f);
		}

		try {
			Thread.sleep(1);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}
	

	float[] colorRamp(float t) {
		final int ncolors = 6;
		float c[][] = { { 1.0f, 0.0f, 0.0f, }, { 1.0f, 0.5f, 0.0f, }, { 1.0f, 1.0f, 0.0f, }, { 0.0f, 1.0f, 0.0f, },
				{ 0.0f, 1.0f, 1.0f, }, { 0.0f, 0.0f, 1.0f, }, };
		t = t * (ncolors - 1);
		int i = (int) t;
		float u = (float) (t - Math.floor(t));
		float[] rgba = { 1.f, 1.f, 1.f, 1.f };
		/*
		 * rgba[0] = lerp(c[i][0], c[i+1][0], u); rgba[1] = lerp(c[i][1],
		 * c[i+1][1], u); rgba[2] = lerp(c[i][2], c[i+1][2], u); rgba[3] = 1.f;
		 */
		return rgba;
	}
}
