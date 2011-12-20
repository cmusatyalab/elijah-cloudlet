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

	class FEAT_SURF_GPU:public MopedAlg {
		
		Float Threshold;
		
	public:
		
		FEAT_SURF_GPU( Float Threshold ) 
		: Threshold(Threshold) {
		}
		
		void getConfig( map<string,string> &config ) const {

			GET_CONFIG( Threshold );
		}
			
		void setConfig( map<string,string> &config ) {
			
			SET_CONFIG( Threshold );
		}

		void process( FrameData &frameData ) {
		
			for( int i=0; i<(int)frameData.images.size(); i++) {
				
				Image *image = frameData.images[i].get();
				vector<FrameData::DetectedFeature> &detectedFeatures = frameData.detectedFeatures[_stepName];

				IplImage* gs = cvCreateImage(cvSize(image->width,image->height), IPL_DEPTH_8U, 1);
				for (int y = 0; y < image->height; y++) 
					memcpy( &gs->imageData[y*gs->widthStep], &image->data[y*image->width], image->width );
				
				cv::SURF surf(Threshold);
				
				vector<float> descriptors;
				vector<cv::KeyPoint> keypoints;
				surf( cv::Mat(cv::Ptr<IplImage>(gs)), cv::Mat(), keypoints, descriptors);
								
				int nDesc = detectedFeatures.size();
				detectedFeatures.resize(detectedFeatures.size() + keypoints.size());
				for(int j=0; j<(int)keypoints.size(); j++) {
					
					detectedFeatures[nDesc].imageIdx = i;
					
					detectedFeatures[nDesc].descriptor.resize(64);
					for (int x=0; x<64; x++) detectedFeatures[nDesc].descriptor[x] = descriptors[j*64+x];

					detectedFeatures[nDesc].coord2D[0] =  keypoints[j].pt.x;
					detectedFeatures[nDesc].coord2D[1] =  keypoints[j].pt.y;

					nDesc++;
				}
			}
		}
	};
};
