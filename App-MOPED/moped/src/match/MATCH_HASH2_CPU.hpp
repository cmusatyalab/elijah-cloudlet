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

		template< typename F, typename T >
		class HashNN {

			int DESC_SIZE;
			int NUM_HASH;
			int NUM_DIMS;
			
			int L1( F *v1, F *v2) {
				int r=0; for(int x=0; x<DESC_SIZE; x++) r+=abs((int)v1[x]-(int)v2[x]); return r;
			}

			struct P {
				
				typedef unsigned long long U;
				U d;
				shared_ptr< T > data;
				shared_ptr< vector<F> > desc;		
			};		

			vector< int > mean;
			vector< vector< vector< shared_ptr<P> > > > hashTables;
			vector< vector< int > > indexDims;

		public:
		
			HashNN() : DESC_SIZE(0), NUM_HASH(0), NUM_DIMS(0) {};
			
			void setUp( vector< pair< vector<F>,T> > &data, int descSize=128, int numHash=16, int numDims=24  ) {
				
				DESC_SIZE=descSize;
				NUM_HASH=numHash; 
				NUM_DIMS=numDims;
				
				mean=vector<int>(DESC_SIZE, 0);

				foreach( d, data)
					for(int x=0; x<DESC_SIZE; x++)
						mean[x]+=d.first[x];
				for(int x=0; x<DESC_SIZE; x++)
					mean[x]=3; //=data.size();

				vector< P > D; D.reserve( data.size() );
				foreach( d, data) {
					P p;
					p.data = shared_ptr<T>( new T(d.second) );
					p.desc = shared_ptr< vector<F> >( new vector<F>(d.first) );
					D.push_back(p);
				}
					
					foreach( token, D ) {
						for(int j=0; j<DESC_SIZE; j++) 
							if( (*token.desc)[j] > mean[j] ) 
								(*token.desc)[j]=1;
							else 
								(*token.desc)[j]=0;
					}

				indexDims.resize(NUM_HASH);
				hashTables.resize(NUM_HASH);
				for(int i=0; i<NUM_HASH; i++) {
					indexDims[i].resize(NUM_DIMS);
					for( int j=0; j<NUM_DIMS; j++)
						indexDims[i][j]=rand()%DESC_SIZE;
					
					hashTables[i].resize(1<<NUM_DIMS);
					
			if( i ) continue;

					foreach( token, D ) {
							
						P p = token;
						p.d = 0;
						for(int j=0; j<NUM_DIMS; j++) 
							if( (*p.desc)[indexDims[i][j]] ) 
								p.d = (p.d << 1)+1;
							else 
								p.d = p.d << 1;

			p.d=0;
						hashTables[i][p.d].push_back( shared_ptr<P>( new P(p) ) );
					}
					
//					for( int j=0; j<(1<<NUM_DIMS); j++)
//						if( hashTables[i][j].size() > 16 ) hashTables[i][j].clear();
				}
			}
			
			int search( F *_v, pair<float, T *> &result) {

				float bestDist = 128 ;
				float secondBestDist = 128 ;
				float bestRatio = 1.;
				T *bestMatch=NULL;			
				
				F v[DESC_SIZE];
						for(int j=0; j<DESC_SIZE; j++) 
							if( _v[j] > mean[j] ) 
								v[j]=1;
							else 
								v[j]=0;



				P p; 
				for(int i=0; i<NUM_HASH; i++) {

					int hash=0;
					for(int j=0; j<NUM_DIMS; j++) 
						if( v[indexDims[i][j]] ) 
							hash = (hash << 1)+1;
						else 
							hash = hash << 1;
			hash=0;

					for( typeof(hashTables[i][hash].begin()) it = hashTables[i][hash].begin(); it != hashTables[i][hash].end(); it++ ) { 
						
						if( it->get()->data.get() == bestMatch ) continue;
						
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
				}

				result.first = bestRatio;
				result.second = bestMatch;
				return bestDist < 128;
			}
		};
		
		int DescriptorSize;
		string DescriptorType;
		int NHashTables;
		int NDims;
		float Ratio;

		HashNN< unsigned char, pair<int,Pt<3> *> > hashNN;
		
		bool skipCalculation;

		void Update() {
			
			skipCalculation = true;
			
			if( models==NULL ) return;
			
			vector< pair< vector<unsigned char>, pair<int, Pt<3> *> > > data;

			for( int nModel = 0; nModel < (int)models->size(); nModel++ ) {
				
				vector<Model::IP> &IPs = (*models)[nModel]->IPs[DescriptorType];
				
				for( int nFeat = 0; nFeat < (int)IPs.size(); nFeat++ )
					data.push_back( make_pair( IPs[nFeat].descriptor, pair<int, Pt<3> *>( nModel, &IPs[nFeat].coord3D ) ) );
			}
			
			if( data.size() > 10 ) { 
				skipCalculation = false;
				hashNN.setUp( data, DescriptorSize, NHashTables, NDims);
			}
			configUpdated = false;
		}
				
	public:

		
		MATCH_HASH_CPU( int DescriptorSize, string DescriptorType, int NHashTables, int NDims, float Ratio )
		: DescriptorSize(DescriptorSize), DescriptorType(DescriptorType), NHashTables(NHashTables), NDims(NDims), Ratio(Ratio) {			
			
			models=NULL;
			skipCalculation=false;
		}

		void getConfig( map<string,string> &config ) const {
			
			GET_CONFIG(Ratio);
			GET_CONFIG(DescriptorType);
			GET_CONFIG(DescriptorSize);
			GET_CONFIG(NHashTables);
			GET_CONFIG(NDims);
		};
		
		void setConfig( map<string,string> &config ) {
			
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
				
				pair<float, pair<int, Pt<3> *> *> result;
				if( !hashNN.search( &corresp[i].descriptor[0], result ) ) 
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
