package edu.cmu.cs.cloudlet.android.application.graphics;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL10;

public class Graphics {

	private FloatBuffer tmpVertexBuffer, tmpBuf;
	private ByteBuffer tmpByteColorBuffer;
	private byte[] byteBuf, colorBytes;
	
	private int particleNumber = 0;
	
	public void draw(GL10 gl) {
		if(this.particleNumber <= 0 || this.tmpVertexBuffer == null)		//check if data received
			return;
		gl.glPointSize(6);													//set point size
		gl.glVertexPointer(2, GL10.GL_FLOAT, 0, tmpVertexBuffer);			//assign vertex buffer	
		gl.glColorPointer(4, GL10.GL_UNSIGNED_BYTE, 0, tmpByteColorBuffer);	//assign color buffer
		gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);						//enable arrays
		gl.glEnableClientState(GL10.GL_COLOR_ARRAY);
		gl.glDrawArrays(GL10.GL_POINTS, 0, this.particleNumber); 			//draw points
		gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);						//disable arrays
		gl.glDisableClientState(GL10.GL_COLOR_ARRAY);
	}
	
	public void updatePosition(ByteBuffer buffer) {
		int numCoords = 2;													//X,Y
		
		//Each particle consists of 2 floats of size 4 + one int with color info
		int numReceived = buffer.limit() / (4 * numCoords + 4);		
		
		//Offset to color info, which is received after all vertex information
		int color_offset = numReceived * (4 * numCoords);

		//Initialize
		if ((this.tmpVertexBuffer == null && this.tmpByteColorBuffer == null) 
				|| this.particleNumber < numReceived) {
			this.particleNumber = numReceived;
			
			// Init Vertex Buffer
			int buffer_size = this.particleNumber * 4 * 2;					//in bytes
			ByteBuffer tempBuf = ByteBuffer.allocateDirect(buffer_size);				
			tempBuf.order(ByteOrder.nativeOrder());
			tmpVertexBuffer = tempBuf.asFloatBuffer();
			tmpVertexBuffer.position(0);
			
			buffer_size = this.particleNumber * 4;
			tmpByteColorBuffer = ByteBuffer.allocateDirect(buffer_size);
			tmpByteColorBuffer.order(ByteOrder.nativeOrder());
		}
		
		//Extract vertex data
		buffer.position(0);
		tmpBuf = buffer.asFloatBuffer();
		float[] floats = new float[this.particleNumber * 2];
		tmpBuf.get(floats, 0, this.particleNumber * 2);
		tmpVertexBuffer.put(floats);
		tmpVertexBuffer.position(0);
		buffer.position(0);
		
		//Extract color data
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
		
		try {
			Thread.sleep(1);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}
	
}
