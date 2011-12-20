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


	class MATCH_HASH_CPU:public MopedAlg {

		static inline void norm( vector<float> &d ) {
			float norm=0; for (int x=0; x<(int)d.size(); x++) norm += d[x]*d[x]; norm = 1/sqrtf(norm);
			for (int x=0; x<(int)d.size(); x++) d[x] *=norm;
		}

		
		template< typename T >
		class HashNN {

			int DESC_SIZE;
			int NUM_HASH;
			int HASH_SIZE;
			int NUM_DIMS;
			
			int THRESHOLD;
			
			int L1( vector<char> &v1, vector<char> &v2)
				{ int r=0; for(int x=0; x<DESC_SIZE; x++) r+=abs(v1[x]-v2[x]); return r; }

			vector< pair< vector<char>, T> > data;
			vector< vector< int > *> hashTables;
			vector< vector< int > > indexDims;

		public:
		
			HashNN() : DESC_SIZE(0), NUM_HASH(0), NUM_DIMS(0) {};
			
			~HashNN() {
				for( vector< vector< int > * >::iterator it=hashTables.begin(); it!=hashTables.end(); it++)
					if( *it ) delete *it;
			}
			
			void setUp( vector< pair< vector<char>, T > > &_data, int descSize, int numHash, int numDims, char threshold, int SizeLimit  ) {
				
				DESC_SIZE=descSize;
				NUM_HASH=numHash; 
				NUM_DIMS=numDims;
				HASH_SIZE=1<<NUM_DIMS;
				THRESHOLD=threshold;
				
				data=_data;
				
				indexDims.resize(NUM_HASH);
				
				hashTables.resize(NUM_HASH*HASH_SIZE, NULL) ;

				#pragma omp parallel for
				for(int i=0; i<NUM_HASH; i++) {
					indexDims[i].resize(NUM_DIMS);
					for( int j=0; j<NUM_DIMS; j++)
						indexDims[i][j]=rand()%DESC_SIZE;
					
					for( int k=0; k<(int)data.size(); k++) {
							
						int hash = 0;
						for(int j=0; j<NUM_DIMS; j++)
							hash = (hash << 1) + (data[k].first[indexDims[i][j]]>THRESHOLD?1:0);

						if( !hashTables[i*HASH_SIZE+hash]) hashTables[i*HASH_SIZE+hash] = new vector< int >;
						hashTables[i*HASH_SIZE+hash]->push_back( k );
					}
					
					for( int j=0; j<HASH_SIZE; j++)
						if( hashTables[i*HASH_SIZE+j] && (int)hashTables[i*HASH_SIZE+j]->size() > SizeLimit ) delete hashTables[i*HASH_SIZE+j];
				}
			}
			
			int search( vector<char> &v, pair<float, T *> &result) {

				int bestDist = 10E8 ;
				int secondBestDist = 10E8 ;
				float bestRatio = 1.;
				T *bestMatch = NULL;			
				
				for(int i=0; i<NUM_HASH; i++) {

					int hash=0;
					for(int j=0; j<NUM_DIMS; j++) 
						hash = (hash << 1)+(v[indexDims[i][j]]>THRESHOLD?1:0);
					
					if( !hashTables[i*HASH_SIZE+hash] ) continue;

					for( typeof( hashTables[i*HASH_SIZE+hash]->begin()) it = hashTables[i*HASH_SIZE+hash]->begin(); it != hashTables[i*HASH_SIZE+hash]->end(); it++ ) { 
						
						clog << *it << " " << data.size() << endl;
						
						if( bestMatch != NULL && data[*it].second == *bestMatch ) continue;
						
						int dist = L1( v, data[*it].first );
						if( dist < bestDist ) {
							
							bestRatio = (float)dist/(float)bestDist;
							bestDist = dist;
							bestMatch = &data[*it].second;
						} else if( dist>bestDist && dist<secondBestDist ) {
							
							bestRatio = (float)bestDist/(float)dist;
							secondBestDist = dist;
						}
					}
				}
				result.first = bestRatio;
				result.second = bestMatch;
				return bestDist < 10E8;
			}
		};
		
		int DescriptorSize;
		string DescriptorType;
		int NHashTables;
		int NDims;
		float Ratio;
		char Threshold;
		float SizeLimit;

		HashNN<pair<int,Pt<3> > > hashNN;
		
		bool skipCalculation;

		void Update() {
			
			skipCalculation = true;
			
			vector< pair< vector<char>, pair<int, Pt<3> > > > data;

			for( int nModel = 0; nModel < (int)models->size(); nModel++ ) {
				
				vector<Model::IP> &IPs = (*models)[nModel]->IPs[DescriptorType];
				
				for( int nFeat = 0; nFeat < (int)IPs.size(); nFeat++ ) {
					
					vector<char> desc(IPs[nFeat].descriptor.size());
					for(int x=0; x<(int)IPs[nFeat].descriptor.size(); x++) desc[x]=min(max(IPs[nFeat].descriptor[x]*256+0.5, 0.), 127.);
					data.push_back( make_pair( desc, pair<int, Pt<3> >( nModel, IPs[nFeat].coord3D ) ) );
				}
			}
			
			if( data.size() > 1 ) { 
				skipCalculation = false;
				hashNN.setUp( data, DescriptorSize, NHashTables, NDims, Threshold, SizeLimit );
			}
			configUpdated = false;
		}
				
	public:

		
		MATCH_HASH_CPU( int DescriptorSize, string DescriptorType, int NHashTables, int NDims, float Ratio, char Threshold, float SizeLimit )
		: DescriptorSize(DescriptorSize), DescriptorType(DescriptorType), NHashTables(NHashTables), NDims(NDims), Ratio(Ratio), Threshold(Threshold), SizeLimit(SizeLimit) {			

			skipCalculation=true;
		}

		void getConfig( map<string,string> &config ) const {
			
			GET_CONFIG(SizeLimit);
			GET_CONFIG(Threshold);
			GET_CONFIG(Ratio);
			GET_CONFIG(DescriptorType);
			GET_CONFIG(DescriptorSize);
			GET_CONFIG(NHashTables);
			GET_CONFIG(NDims);
		};
		
		void setConfig( map<string,string> &config ) {
			
			SET_CONFIG(SizeLimit);
			SET_CONFIG(Threshold);
			SET_CONFIG(Ratio);
			SET_CONFIG(DescriptorType);
			SET_CONFIG(DescriptorSize);
			SET_CONFIG(NHashTables);
			SET_CONFIG(NDims);
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

				vector<char> desc(DescriptorSize);
				for(int x=0; x<DescriptorSize; x++) desc[x]=min(max(corresp[i].descriptor[x]*256+0.5, 0.), 127.);
				
				pair<float, pair<int, Pt<3> > *> result;
				norm( corresp[i].descriptor );
				if( !hashNN.search( desc, result ) ) 
					continue;
				
				if( result.first > Ratio )
					continue;
					
				int nModel = result.second->first;
				
				matches[nModel].resize( matches[nModel].size() +1 );
				
				FrameData::Match &match = matches[nModel].back();

				match.imageIdx = corresp[i].imageIdx;
				match.coord3D = result.second->second;
				match.coord2D = corresp[i].coord2D;
			}
			
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
