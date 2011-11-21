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

namespace MopedNS {
	
	class MATCH_DISPLAY:public MopedAlg {

		int display;
	public:

		MATCH_DISPLAY( int display ) 
		: display(display) { }

		void getConfig( map<string,string> &config ) const {

			GET_CONFIG( display );
		}
			
		void setConfig( map<string,string> &config ) {
			
			SET_CONFIG( display );
		}
		
		void process(FrameData &frameData) {

			if( display < 1 ) return;

			vector< vector< FrameData::Match > >   &matches = frameData.matches;
			
			int sz = 0;
			for(int model=0; model<(int)matches.size(); model++ )
				sz += matches[model].size();

			clog << _stepName << ":" << _alg << " : FOUND " << sz << " Matches" << endl;
		
			if( display < 2 ) return;

			vector<CvScalar> objectColors(256);
			for( int i=0; i<256; i++ )  {
				
				int r = i;
				
				r = (((r * 214013L + 2531011L) >> 16) & 32767);
				int a = 192 + r%64;
				r = (((r * 214013L + 2531011L) >> 16) & 32767);
				int b = 64 + r%128;
				r = (((r * 214013L + 2531011L) >> 16) & 32767);
				int c = r%64;
				for(int x=0; x<100; x++) if( (r = (((r * 214013L + 2531011L) >> 16) & 32767)) % 2 ) swap(a,b); else swap(a,c);
				
				objectColors[i] = cvScalar( a,b,c );
			}

			
			for( int i=0; i<(int)frameData.images.size(); i++) {
				
				string windowName = _stepName + " #" + toString(i) + ":" + frameData.images[i]->name;
				
				cvNamedWindow( windowName.c_str(), CV_WINDOW_AUTOSIZE);
				
				IplImage* img = cvCreateImage(cvSize(frameData.images[i]->width,frameData.images[i]->height), IPL_DEPTH_8U, 3);

				for (int y = 0; y < frameData.images[i]->height; y++) {
					for (int x = 0; x < frameData.images[i]->width; x++) { 
						img->imageData[y*img->widthStep+3*x + 0] = frameData.images[i]->data[y*frameData.images[i]->width + x];
						img->imageData[y*img->widthStep+3*x + 1] = frameData.images[i]->data[y*frameData.images[i]->width + x];
						img->imageData[y*img->widthStep+3*x + 2] = frameData.images[i]->data[y*frameData.images[i]->width + x];
					}
				}

				for(int model=0; model<(int)matches.size(); model++ ) {
					
					int objectHash = 0; 
					for(unsigned int x=0; x<(*models)[model]->name.size(); x++) objectHash = objectHash ^ (*models)[model]->name[x];
					CvScalar color = objectColors[objectHash % 256];
					
					foreach( match, matches[model] )
						if( match.imageIdx == i )
							cvCircle(img, cvPoint( match.coord2D[0], match.coord2D[1]), 2, color , CV_FILLED, CV_AA );
				}
				
				cvShowImage( windowName.c_str(), img );
				
				cvReleaseImage(&img);
			}
			
			cvWaitKey( 10 );
		}
	};
};
