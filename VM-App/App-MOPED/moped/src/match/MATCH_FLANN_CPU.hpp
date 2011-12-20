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

	class MATCH_FLANN_CPU:public MopedAlg {

		static inline void norm( vector<float> &d ) {
			float norm=0; for (int x=0; x<(int)d.size(); x++) norm += d[x]*d[x]; norm = 1/sqrtf(norm);
			for (int x=0; x<(int)d.size(); x++) d[x] *=norm;
		}
		
		int DescriptorSize;
		string DescriptorType;
		float Precision;
		float Ratio;

		bool skipCalculation;


		vector< pair<int, Pt<3> *> > modelPointData;

	
		shared_ptr<cv::flann::Index> index_id[MAX_THREADS];
		cv::Mat mat;
	
		void Update() {
			
			skipCalculation = true;
			
			if( models==NULL ) return;
			
			modelPointData.clear();
			vector<float> dataset;

			for( int nModel = 0; nModel < (int)models->size(); nModel++ ) {
				
				vector<Model::IP> &IPs = (*models)[nModel]->IPs[DescriptorType];
				
				for( int nFeat = 0; nFeat < (int)IPs.size(); nFeat++ ) {
					
					norm( IPs[nFeat].descriptor );
					for( int i = 0; i < (int)IPs[nFeat].descriptor.size(); i++ )
						dataset.push_back( IPs[nFeat].descriptor[i] );
				
					modelPointData.push_back( make_pair( nModel, &IPs[nFeat].coord3D ) );
				}
			}
			mat = cv::Mat( dataset.size()/DescriptorSize, DescriptorSize, CV_32F);
			
			if( modelPointData.size() > 1 ) { 
				skipCalculation = false;

				for(int x=0; x<(int)dataset.size(); x++) mat.at<float>(x/DescriptorSize,x%DescriptorSize)=dataset[x];
				
				#pragma omp parallel for
				for(int x=0; x<MAX_THREADS; x++) 
					index_id[x]=shared_ptr<cv::flann::Index>( new cv::flann::Index( mat, cv::flann::KDTreeIndexParams(4) ) );
			}
			configUpdated = false;
		}

				
	public:

		
		MATCH_FLANN_CPU( int DescriptorSize, string DescriptorType, float Precision, float Ratio )
		: DescriptorSize(DescriptorSize), DescriptorType(DescriptorType), Precision(Precision), Ratio(Ratio) {			

			skipCalculation=false;
		}

		void getConfig( map<string,string> &config ) const {
			
			GET_CONFIG(Ratio);
			GET_CONFIG(Precision);
			GET_CONFIG(DescriptorType);
			GET_CONFIG(DescriptorSize);
		};
		
		void setConfig( map<string,string> &config ) {
			
			SET_CONFIG(Ratio);
			SET_CONFIG(Precision);
			SET_CONFIG(DescriptorType);
			SET_CONFIG(DescriptorSize);
		};

		void process( FrameData &frameData ) {

			if( configUpdated ) Update();
			
			if( skipCalculation ) return;
				
			vector< FrameData::DetectedFeature > &corresp = frameData.detectedFeatures[DescriptorType];
			if( corresp.empty() ) return;
			
			vector< vector< FrameData::Match > > &matches = frameData.matches;
			
			vector< vector< vector< FrameData::Match > > > threadedMatches( MAX_THREADS, vector< vector< FrameData::Match > >( models->size() ) );
			
			#pragma omp parallel for
			for( int i=0; i<(int)corresp.size(); i++)  {
				
				int threadNum = omp_get_thread_num();
				vector< vector< FrameData::Match > > &matches = threadedMatches[ threadNum ];
			
				vector<float> desc(DescriptorSize);
				vector<int> nx(2);
				vector<float> dx(2);

				norm( corresp[i].descriptor );
				for(int x=0; x<DescriptorSize; x++ ) desc[x]=corresp[i].descriptor[x];
				
				index_id[threadNum]->knnSearch(desc, nx, dx, 2, cv::flann::SearchParams(32) );


				if(  dx[0]/dx[1] < Ratio ) {				

					int nModel = modelPointData[nx[0]].first;
					
					matches[nModel].resize( matches[nModel].size() +1 );
					
					FrameData::Match &match = matches[nModel].back();

					match.imageIdx = corresp[i].imageIdx;
					match.coord3D = *modelPointData[nx[0]].second;
					match.coord2D = corresp[i].coord2D;
				}
			}
			
			matches.resize( models->size() );
			for(int nModel = 0; nModel < (int)models->size(); nModel++) {

				int sz = 0;
				for(int t=0; t<MAX_THREADS; t++)
					sz += threadedMatches[t][nModel].size();

				matches[nModel].resize(sz);

				sz = 0;
				for(int t=0; t<MAX_THREADS; t++) {
					memcpy( &matches[nModel][sz], &threadedMatches[t][nModel][0], sizeof(FrameData::Match) * threadedMatches[t][nModel].size() );
					sz += threadedMatches[t][nModel].size();
				}
			}
		}
	};
};
