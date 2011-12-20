/* 
  MOPED (Multiple Object Pose Estimation and Detection) is a fast and
  scalable object recognition and pose estimation system. If you use this
  code, please reference our work in the following publications:
  
  [1] Collet, A., Berenson, D., Srinivasa, S. S., & Ferguson, D. "Object
  recognition and full pose registration from a single image for robotic
  manipulation." In ICRA 2009.  
  [2] Martinez, M., Collet, A., & Srinivasa, S. S. "MOPED: A Scalable and low
  Latency Object Recognition and Pose Estimation System." In ICRA 2010.
  
  Copyright: Carnegie Mellon University & Intel Corporation
  
  Authors:
   Alvaro Collet (alvaro.collet@gmail.com)
   Manuel Martinez (salutte@gmail.com)
   Siddhartha Srinivasa (siddhartha.srinivasa@intel.com)
  
  The MOPED software is developed at Intel Labs Pittsburgh. For more info,
  visit http://personalrobotics.intel-research.net/pittsburgh
  
  All rights reserved under the BSD license.
  
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products
     derived from this software without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include <SiftGPU.h>
#include <GL/glew.h>
#include <GL/glx.h>

namespace MopedNS {

	class FEAT_SIFT_GPU:public MopedAlg {
		
		string ScaleOrigin;
		string Verbosity;
		string GPUDisplay;
		
		SiftGPU sift;

		Display* m_display;
		GLXDrawable m_drawable;
		GLXContext m_context;

	public:

		FEAT_SIFT_GPU( string ScaleOrigin, string Verbosity, string GPUDisplay ) 
		: ScaleOrigin(ScaleOrigin), Verbosity(Verbosity), GPUDisplay(GPUDisplay) {
			
			if( !getenv("DISPLAY") || strlen(getenv("DISPLAY") )<2 ) {
				capable = false;
				return;
			}		
			
			string oldDisplay = string(getenv("DISPLAY"));
			setenv("DISPLAY",GPUDisplay.c_str(),true);
			
			//processing parameters first
			char *argv[] = { (char *)"-v", (char *)Verbosity.c_str(), (char *)"-loweo", (char *)"-fo", (char *)ScaleOrigin.c_str(), (char *)"-di"};
			sift.ParseParam( 6 , argv);
			
			//create an OpenGL context for computation
			capable = ( sift.CreateContextGL() == SiftGPU::SIFTGPU_FULL_SUPPORTED );

			setenv("DISPLAY",oldDisplay.c_str(),true);
			
			if( capable ) {
				
				m_display=glXGetCurrentDisplay();
				m_drawable=glXGetCurrentDrawable();
				m_context=glXGetCurrentContext();

				glXMakeCurrent(m_display,None,NULL);
			}
		}

		void getConfig( map<string,string> &config ) const {

			GET_CONFIG( ScaleOrigin );
			GET_CONFIG( Verbosity );
			GET_CONFIG( GPUDisplay );
		}
			
		void setConfig( map<string,string> &config ) {
			
			SET_CONFIG( ScaleOrigin );
			SET_CONFIG( Verbosity );
			SET_CONFIG( GPUDisplay );

			char *argv[] = { (char *)"-v", (char *)Verbosity.c_str(), (char *)"-loweo", (char *)"-fo", (char *)ScaleOrigin.c_str(), (char *)"-di"};
			sift.ParseParam( 6 , argv);
		}

		void process( FrameData &frameData ) {

			glXMakeCurrent(m_display,m_drawable,m_context);
		
			for( int i=0; i<(int)frameData.images.size(); i++) {
				
				Image *img = frameData.images[i].get();
				
				vector<FrameData::DetectedFeature> &detectedFeatures = frameData.detectedFeatures[_stepName];

				int Xdiv=(img->width+1599)/1600, Ydiv=(img->height+1599)/1600;
				for(int iX = 0; iX < Xdiv; iX++) {
					for(int iY = 0; iY < Ydiv; iY++) {
						
						int Xbase = iX*1600;
						int Ybase = iY*1600;
						
						int Xsize = min( 1600, img->width  - Xbase );
						int Ysize = min( 1600, img->height - Ybase );
						
						vector<char> dt(Xsize*Ysize);	
						for (int y = 0; y < Ysize; y++) 
							for (int x = 0; x < Xsize; x++) 
								dt[y*Xsize+x] = img->data[(Ybase+y)*img->width+Xbase+x];	
						
						sift.RunSIFT (Xsize, Ysize, &dt[0], GL_LUMINANCE, GL_UNSIGNED_BYTE);
						//Better use GL_LUMINANCE data to save transfer time
						int nFeat = sift.GetFeatureNum();//get feature count
						//allocate memory for readback
						vector<SiftGPU::SiftKeypoint> keys(nFeat);
						//read back keypoints and normalized descritpros
						//specify NULL if you don’t need keypoints or descriptors
						vector<float> imageDescriptors( 128 * nFeat);
						sift.GetFeatureVector(&keys[0], &imageDescriptors[0]);
						

						int nBase = detectedFeatures.size();
						detectedFeatures.resize( nBase + nFeat );
						for(int n=0; n<nFeat; n++) {
							
							detectedFeatures[nBase+n].imageIdx = i;
							
							detectedFeatures[nBase+n].descriptor.resize(128);
							for (int x=0; x<128; x++) detectedFeatures[nBase+n].descriptor[x] = imageDescriptors[(n<<7)+x];

							detectedFeatures[nBase+n].coord2D[0] = keys[n].x + Xbase;
							detectedFeatures[nBase+n].coord2D[1] = keys[n].y + Ybase;
						}
					}
				}
			}
			glXMakeCurrent(m_display,None,NULL);
		}
	};
};
