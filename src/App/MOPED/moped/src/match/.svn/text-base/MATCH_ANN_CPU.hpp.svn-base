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

#include <ANN.h>

namespace MopedNS {

	class MATCH_ANN_CPU:public MopedAlg {
		
		static inline void norm( vector<float> &d ) {
			float norm=0; for (int x=0; x<(int)d.size(); x++) norm += d[x]*d[x]; norm = 1./sqrtf(norm);
			for (int x=0; x<(int)d.size(); x++) d[x] *=norm;
		}
		
		int DescriptorSize;
		string DescriptorType;
		Float Quality;
		Float Ratio;

		bool skipCalculation;
		
		vector<int> correspModel;
		vector< Pt<3> * > correspFeat;

		ANNkd_tree *kdtree;


		void Update() {
			
			skipCalculation = true;

			unsigned int modelsNFeats = 0;
			foreach( model, *models )
				modelsNFeats += model->IPs[DescriptorType].size();
			
			correspModel.resize( modelsNFeats );
			correspFeat.resize( modelsNFeats );

			ANNpointArray refPts = annAllocPts( modelsNFeats , DescriptorSize );

			int x=0;
			for( int nModel = 0; nModel < (int)models->size(); nModel++ ) {
				
				vector<Model::IP> &IPs = (*models)[nModel]->IPs[DescriptorType];
				
				for( int nFeat = 0; nFeat < (int)IPs.size(); nFeat++ ) {

					correspModel[x] = nModel;
					correspFeat[x]  = &IPs[nFeat].coord3D;
					norm( IPs[nFeat].descriptor );
					for( int i=0; i<DescriptorSize; i++ )
						refPts[x][i] = IPs[nFeat].descriptor[i];
					
					x++;
				}
			}
			
			if( modelsNFeats > 1 ) { 
				
				skipCalculation = false;
				if( kdtree ) delete kdtree;
				kdtree = new ANNkd_tree(refPts, modelsNFeats, DescriptorSize );
			}
			configUpdated = false;
		}
				
	public:

		MATCH_ANN_CPU( int DescriptorSize, string DescriptorType, Float Quality, Float Ratio )
		: DescriptorSize(DescriptorSize), DescriptorType(DescriptorType), Quality(Quality), Ratio(Ratio)  {			

			kdtree=NULL;
			skipCalculation=true;
		}

		void getConfig( map<string,string> &config ) const {
			
			GET_CONFIG(DescriptorType);
			GET_CONFIG(DescriptorSize);
			GET_CONFIG(Quality);
			GET_CONFIG(Ratio);
		};
		
		void setConfig( map<string,string> &config ) {
			
			SET_CONFIG(DescriptorType);
			SET_CONFIG(DescriptorSize);
			SET_CONFIG(Quality);
			SET_CONFIG(Ratio);
		};

		void process( FrameData &frameData ) {

			if( configUpdated ) Update();
			
			if( skipCalculation ) return;
				
			vector< FrameData::DetectedFeature > &corresp = frameData.detectedFeatures[DescriptorType];
			if( corresp.empty() ) return;
			
			vector< vector< FrameData::Match > > &matches = frameData.matches;
			matches.resize( models->size() );
			
			
			
			ANNpoint pt = annAllocPt(DescriptorSize);
		
			ANNidxArray	nx = new ANNidx[2];
			ANNdistArray ds = new ANNdist[2];

			for( int i=0; i<(int)corresp.size(); i++)  {
				
				norm( corresp[i].descriptor);
				for (int j = 0; j < DescriptorSize; j++) 
					pt[j] = corresp[i].descriptor[j];
			
			        #pragma omp critical(ANN)	
				kdtree->annkSearch(pt, 2, nx, ds, Quality);	
				
						
				if(  ds[0]/ds[1] < Ratio ) {
					
					int nModel1 = correspModel[nx[0]];
					if( matches[nModel1].capacity() < 1000 ) matches[nModel1].reserve(1000);
					matches[nModel1].resize( matches[nModel1].size() +1 );
					
					FrameData::Match &match = matches[nModel1].back();

					match.imageIdx = corresp[i].imageIdx;
					match.coord3D = *correspFeat[nx[0]];
					match.coord2D = corresp[i].coord2D;
				}
			}
		}
	};
};
