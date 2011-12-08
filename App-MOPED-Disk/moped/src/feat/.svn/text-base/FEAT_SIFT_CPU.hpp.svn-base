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

#include <siftfast.h>

extern int DoubleImSize;

namespace MopedNS {

	class FEAT_SIFT_CPU:public MopedAlg {
		
		string ScaleOrigin;
		
	public:
		
		FEAT_SIFT_CPU( string ScaleOrigin ) 
		: ScaleOrigin(ScaleOrigin) {
		}
		
		void getConfig( map<string,string> &config ) const {

			GET_CONFIG( ScaleOrigin );
		}
			
		void setConfig( map<string,string> &config ) {
			
			SET_CONFIG( ScaleOrigin );
			if( ScaleOrigin=="-1" ) 
				DoubleImSize=1;
			else 
				DoubleImSize=0;
		}

		void process( FrameData &frameData ) {
		
			for( int i=0; i<(int)frameData.images.size(); i++) {
				
				Image *img = frameData.images[i].get();
				
				vector<FrameData::DetectedFeature> &detectedFeatures = frameData.detectedFeatures[_stepName];

				// Convert to a floating point image with pixels in range [0,1].
				SFImage image = CreateImage(img->height, img->width);
				for (int y = 0; y < img->height; y++) 
					for (int x = 0; x < img->width; x++) 
						image->pixels[y*image->stride+x] = ((float) img->data[img->width*y+x]) * 1./255.;

				Keypoint keypts = GetKeypoints(image);
				Keypoint key = keypts;
				while (key) {
					
					detectedFeatures.resize(detectedFeatures.size()+1);

					detectedFeatures.back().imageIdx = i;
					
					detectedFeatures.back().descriptor.resize(128);
					for (int x=0; x<128; x++) detectedFeatures.back().descriptor[x] = key->descrip[x];

					detectedFeatures.back().coord2D[0] =  key->col;
					detectedFeatures.back().coord2D[1] =  key->row;

					key = key->next;
				}

				FreeKeypoints(keypts);
				DestroyAllImages();   // we can't destroy just one!
			}
		}
	};
};
