package edu.cmu.cs.cloudlet.android.application.graphics;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL10;

import android.util.Log;

public class Graphics {

	protected FloatBuffer tmpVertexBuffer, vertexBuffer;
	protected FloatBuffer tmpColorBuffer, colorBuffer;
	//NEW
	protected ByteBuffer tmpByteColorBuffer, byteColorBuffer;
	private byte[] byteBuf;
	private float[] vertextFloats;
	private byte[] colorBytes;
	private boolean is3D;
	
	protected int particleNumber = 0;
	private String lock = "lock";
	
	public Graphics(boolean is3D) {
		this.is3D = is3D;
	}
	
	public boolean is3D(){
		return this.is3D;
	}
	
	public void draw(GL10 gl) {
		if(this.particleNumber <= 0 || this.vertexBuffer == null)
			return;
		//DATA RECEIVED
		gl.glFrontFace(GL10.GL_CW);											//set face direction
		gl.glPointSize(5);													//set point size
		if(is3D)															//assign vertex buffer
			gl.glVertexPointer(3, GL10.GL_FLOAT, 0, vertexBuffer);	
		else  	
			gl.glVertexPointer(2, GL10.GL_FLOAT, 0, vertexBuffer);	
		gl.glColorPointer(4, GL10.GL_UNSIGNED_BYTE, 0, byteColorBuffer);	//assign color buffer
		gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);						//enable arrays
		gl.glEnableClientState(GL10.GL_COLOR_ARRAY);
		
		/************** BAD HARDCODED BOO **************/
		if(is3D){
//			synchronized(lock){
				gl.glDrawArrays(GL10.GL_POINTS, 0, this.particleNumber * num_z); 		//draw points
//			}
		}
		
		else{
//			synchronized(lock){
				gl.glDrawArrays(GL10.GL_POINTS, 0, this.particleNumber); 		//draw points
//			}
		}
		/**********************************************/
		gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);						//disable arrays
		gl.glDisableClientState(GL10.GL_COLOR_ARRAY);

	}

	/** CONTROL EXECUTION **/
	final int frac = 1;
	final int num_z = 1;
	/***********************/
	
	
	public void updatePosition(ByteBuffer buffer) {
		/**
		 * NOTE: THE FOLLOWING CODE HAS A HARD-CODED THIRD DIMENSION CONSISTING
		 * OF X,Y POINTS EXTENDED BY 100 POINTS IN THE Z-DIRECTION. IT STILL
		 * BASES THE RENDERING ON ONLY (X,Y) POINTS RECEIVED FROM THE SERVER
		 * 
		 * COLORS FOR EACH POINT ARE RECEIVED IN SETS OF 4 CONSECUTIVE BYTES,
		 * EACH REPRESENTING R,G,B,A VALUES RESPECTIVELY. COLOR INFORMATION IS
		 * RECEIVED AT THE END OF VERTEX INFORMATION (NOT MIXED TOGETHER)
		 */
		//int numCoords = is3D ? 3 : 2;
		int numCoords = 2;
	 	//particleSize is the number of particles (size of double * 2 coordinates + 4 byte int for color)
		int particleSize = buffer.limit() / (4 * numCoords + 4);
		
		final int color_offset = particleSize * (4 * numCoords);
		
		
		// Initialize (one time only)
		if ((this.tmpVertexBuffer == null && this.tmpColorBuffer == null) 
				|| this.particleNumber < particleSize) {
			this.particleNumber = particleSize;
			
			// Init Vertex Buffer
			// int buffer_size = this.particleNumber*(Float.SIZE/8)*numCoords;
			int buffer_size = this.particleNumber*(Float.SIZE/8)*2;
			
			/*******************************/
			buffer_size *= num_z;
			/*******************************/
			ByteBuffer tempBuf = ByteBuffer.allocateDirect(buffer_size);				
			tempBuf.order(ByteOrder.nativeOrder());
			tmpVertexBuffer = tempBuf.asFloatBuffer();
			tmpVertexBuffer.position(0);
			
			buffer_size = is3D ? num_z * this.particleNumber * 4 
					: this.particleNumber * 4;
			tmpByteColorBuffer = ByteBuffer.allocateDirect(buffer_size);
			tmpByteColorBuffer.order(ByteOrder.nativeOrder());
		}
		
		// VERTEX DATA
		ByteBuffer tmpByteBuffer = ByteBuffer.wrap(buffer.array(), 0, color_offset);
		tmpByteBuffer.order(ByteOrder.nativeOrder());
		tmpVertexBuffer = tmpByteBuffer.asFloatBuffer();
		tmpVertexBuffer.position(0);
		float val = tmpVertexBuffer.get(0);
		
		// EXTRACT COLOR DATA
		byteBuf = buffer.array(); 
		colorBytes = new byte[buffer.limit() - color_offset];
		System.arraycopy(byteBuf, color_offset, colorBytes, 0, buffer.limit() - color_offset);
		
		//Reverses every 4 consecutive bytes
		for(int i = 0; i < colorBytes.length; i += 4){
			for(int j = 0; j < 2; j++){
				byte tmp = colorBytes[i + j];
				colorBytes[i + j] = colorBytes[i + 3 - j];
				colorBytes[i + 3 - j] = tmp;
			}
		}
		
		tmpByteColorBuffer.put(colorBytes);
		tmpByteColorBuffer.position(0);
		
		this.vertexBuffer = this.tmpVertexBuffer.asReadOnlyBuffer();
		this.byteColorBuffer = this.tmpByteColorBuffer.asReadOnlyBuffer();
			
		try {
			Thread.sleep(1);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}
}
