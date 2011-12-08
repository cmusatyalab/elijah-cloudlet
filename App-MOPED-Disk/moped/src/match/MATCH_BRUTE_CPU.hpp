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

	class MATCH_BRUTE_CPU:public MopedAlg {

		static inline void norm( vector<float> &d ) {
			float norm=0; for (int x=0; x<(int)d.size(); x++) norm += d[x]*d[x]; norm = 1/sqrtf(norm);
			for (int x=0; x<(int)d.size(); x++) d[x] *=norm;
		}
		
		template< typename T >
		class BRUTE {

			int DESC_SIZE;
			
			int L1( char *v1, char *v2) {
				{ int r=0; for(int x=0; x<DESC_SIZE; x++) r+=abs(v1[x]-v2[x]); return r; }

			struct P {
				
				typedef unsigned long long U;
				U d;
				shared_ptr< T > data;
				shared_ptr< vector<F> > desc;		
			};		

			vector< shared_ptr<P> > dbDesc;

		public:
		
			BRUTE() : DESC_SIZE(0) {};
			
			
			void setUp( vector< pair< vector<F>,T> > &data, int descSize=128 ) {
				
				DESC_SIZE=descSize;
				
				dbDesc.clear(); dbDesc.reserve( data.size() );
				foreach( d, data) {
					P p;
					p.data = shared_ptr<T>( new T(d.second) );
					p.desc = shared_ptr< vector<F> >( new vector<F>(d.first) );
					dbDesc.push_back( shared_ptr<P>( new P(p) )  );
				}
			}
			
			
			
			int search( F *v, pair<float, T *> &result) {

				float bestDist = 10E10 ;
				float secondBestDist = 10E10 ;
				float bestRatio = 1.;
				T *bestMatch=NULL;			
				
				P p; 

				for( typeof(dbDesc.begin()) it = dbDesc.begin(); it != dbDesc.end(); it++ ) { 
					
					float dist = L1( v, &(*it->get()->desc)[0] );
					if( dist < bestDist ) {
						
						bestRatio = (float)dist/(float)bestDist;
						bestDist = dist;
						bestMatch = it->get()->data.get();
					} else if( dist>bestDist && dist<secondBestDist ) {
						
						bestRatio = (float)bestDist/(float)dist;
						secondBestDist = dist;
					}
				}
				result.first = bestRatio;
				result.second = bestMatch;
				return bestDist < 10E9;
			}
		};
		
		int DescriptorSize;
		string DescriptorType;
		float Ratio;

		BRUTE< unsigned char, pair<int,Pt<3> *> > brute;
		
		bool skipCalculation;

		void Update() {
			
			skipCalculation = true;
			
			if( models==NULL ) return;
			
			vector< pair< vector<unsigned char>, pair<int, Pt<3> *> > > data;

			for( int nModel = 0; nModel < (int)models->size(); nModel++ ) {
				
				vector<Model::IP> &IPs = (*models)[nModel]->IPs[DescriptorType];
				norm( IPs[nFeat].descriptor );
				for( int nFeat = 0; nFeat < (int)IPs.size(); nFeat++ )
					data.push_back( make_pair( IPs[nFeat].descriptor, pair<int, Pt<3> *>( nModel, &IPs[nFeat].coord3D ) ) );
			}
			
			if( data.size() > 10 ) { 
				skipCalculation = false;
				brute.setUp( data, DescriptorSize);
			}
			configUpdated = false;
		}
				
	public:

		
		MATCH_BRUTE_CPU( int DescriptorSize, string DescriptorType, float Ratio )
		: DescriptorSize(DescriptorSize), DescriptorType(DescriptorType), Ratio(Ratio) {			
			
			models=NULL;
			skipCalculation=false;
		}

		void getConfig( map<string,string> &config ) const {
			
			GET_CONFIG(Ratio);
			GET_CONFIG(DescriptorType);
			GET_CONFIG(DescriptorSize);
		};
		
		void setConfig( map<string,string> &config ) {
			
			SET_CONFIG(Ratio);
			SET_CONFIG(DescriptorType);
			SET_CONFIG(DescriptorSize);
		};

		void process( FrameData &frameData ) {

			if( configUpdated ) Update();
			
			if( skipCalculation ) return;
				
			vector< FrameData::DetectedFeature > &corresp = frameData.detectedFeatures[DescriptorType];
			if( corresp.empty() ) return;
			
			vector< vector< FrameData::Match > > &matches = frameData.matches;
			matches.resize( models->size() );
			
			vector< vector< vector< FrameData::Match > > > threadedMatches( MAX_THREADS, vector< vector< FrameData::Match > >( models->size() ) );
			
			#pragma omp parallel for
			for( int i=0; i<(int)corresp.size(); i++)  {
				
				int threadNum = omp_get_thread_num();
				
				vector< vector< FrameData::Match > > &matches = threadedMatches[ threadNum ];
				
				pair<float, pair<int, Pt<3> *> *> result;
				norm( corresp[i].descriptor );
				if( !brute.search( &corresp[i].descriptor[0], result ) ) 
					continue;
				
				if( result.first > Ratio )
					continue;
					
				int nModel = result.second->first;
				
				matches[nModel].resize( matches[nModel].size() +1 );
				
				FrameData::Match &match = matches[nModel].back();

				match.imageIdx = corresp[i].imageIdx;
				match.coord3D = *result.second->second;
				match.coord2D = corresp[i].coord2D;
			}
			
			
			map< Pt<2>, pair< int, Pt<3> > > groundTruth;
			for(int nModel = 0; nModel < (int)models->size(); nModel++)
				for(int t=0; t<MAX_THREADS; t++) 
					for(int i=0; i<(int)threadedMatches[t][nModel].size(); i++) 
						groundTruth[ threadedMatches[t][nModel][i].coord2D ] = make_pair( nModel, threadedMatches[t][nModel][i].coord3D );
			
			frameData.correctMatches=0;
			frameData.incorrectMatches=0;
			for(int nModel = 0; nModel < (int)models->size(); nModel++)
				for(int i = 0; i<(int)matches[nModel].size(); i++)
					if( groundTruth[ matches[nModel][i].coord2D ] == make_pair( nModel, matches[nModel][i].coord3D ) )
						frameData.correctMatches++;
					else
						frameData.incorrectMatches++;
			


			cout << _stepName << " " << frameData.correctMatches << " " << frameData.incorrectMatches <<  endl;
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
