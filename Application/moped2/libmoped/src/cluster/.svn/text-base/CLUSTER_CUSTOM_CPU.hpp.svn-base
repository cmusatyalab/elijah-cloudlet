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

	class CLUSTER_CUSTOM_CPU :public MopedAlg {
		
		template< typename T, int N >
		struct Canopy {
			
			Pt<N> center;
			Pt<N> touchPtsAggregate;
			
			list<T> boundPoints;
			int boundPointsSize;
			
			int canopyId;
			Canopy<T,N> *merges;
			
			Canopy<T,N>( Pt<N> &p, T &t, int id ) {
				
				center = p;
				
				boundPoints.push_back( t );
				boundPointsSize = 1;

				canopyId = id;
			} 
		};
		
		template< typename T, int N >
		void CustomClustering( vector< list<T> > &clusters, vector< pair<Pt<N>,T> > &points, int MinPts, int MaxIterations ) {
			
			float SqRadius = 1.*1.;
			float SqMerge  = 0.2*0.2;
			
			vector< Canopy<T,N> > can;
			can.reserve( points.size() );
			
			list< int > canopiesRemaining;
			
			for(int i=0; i<(int)points.size(); i++) {
				can.push_back( Canopy<T,N>( points[i].first, points[i].second, i ) );
				can.back().merges = &can.back();
				canopiesRemaining.push_back( i );
			}
			
			for( int nIter = 0; canopiesRemaining.size() && nIter < MaxIterations; nIter++ ) { // shift canopies to their centroids

				bool somethingHasChanged = false;
				
				foreach ( cId, canopiesRemaining ) {

					can[cId].touchPtsAggregate = can[cId].center * can[cId].boundPointsSize;
					
					int touchPtsN = can[cId].boundPointsSize;
				
					foreach ( othercId, canopiesRemaining ) {
						
						if( cId == othercId ) continue;
						
						float dist = can[cId].center.sqEuclDist( can[othercId].center );
						if (dist < SqRadius) {
							
							touchPtsN += can[othercId].boundPointsSize;
							can[cId].touchPtsAggregate += can[othercId].center * can[othercId].boundPointsSize;
						}
					}
					can[cId].touchPtsAggregate /= touchPtsN;
				}

				foreach ( cId, canopiesRemaining ) {
					foreach ( othercId, canopiesRemaining ) {
						if( cId==othercId ) break;
						
						float dist = can[cId].touchPtsAggregate.sqEuclDist( can[othercId].touchPtsAggregate );
						if (dist < SqMerge) {
							can[othercId].merges->merges = &can[cId];
							can[othercId].merges = &can[cId];
						}
					}
				}
				
				eforeach ( cId, cId_it, canopiesRemaining ) {

					if( can[cId].merges != &can[cId] ) {

						can[cId].merges->center = can[cId].merges->center * can[cId].merges->boundPoints.size() + can[cId].center * can[cId].boundPointsSize;

						can[cId].merges->boundPoints.splice( can[cId].merges->boundPoints.end(), can[cId].boundPoints );
						can[cId].merges->boundPointsSize += can[cId].boundPointsSize;

						can[cId].merges->center /= can[cId].merges->boundPointsSize;
						
						cId_it = canopiesRemaining.erase( cId_it );
						somethingHasChanged = true;
					}
				}

				eforeach ( cId, cId_it, canopiesRemaining ) {
					
					if( can[cId].boundPointsSize < MinPts*2 )	continue;
					
					clusters.resize( clusters.size() + 1 );
					clusters.back().splice( clusters.back().end(), can[cId].boundPoints );

					cId_it = canopiesRemaining.erase( cId_it );
					somethingHasChanged = true;
				}
				
				if( !somethingHasChanged ) {
					
					SqRadius *= 1.25*1.25;
					SqMerge  *= 1.25*1.25;
				}
			}
			foreach ( cId, canopiesRemaining ) {
				
				if( can[cId].boundPointsSize < MinPts )	continue;
				
				clusters.resize( clusters.size() + 1 );
				clusters.back().splice( clusters.back().end(), can[cId].boundPoints );
			}
		}

		int MinPts;
		int MaxIterations;
		
	public:

		CLUSTER_CUSTOM_CPU( unsigned int MinPts, unsigned int MaxIterations ) 
		: MinPts(MinPts), MaxIterations(MaxIterations)  {
		}

		void getConfig( map<string,string> &config ) const {
		
			GET_CONFIG( MinPts );
			GET_CONFIG( MaxIterations );		
		}
		
		void setConfig( map<string,string> &config ) {
			
			SET_CONFIG( MinPts );
			SET_CONFIG( MaxIterations );
		}
		
		void process( FrameData &frameData ) {

			frameData.clusters.resize( models->size() );
		
			//#pragma omp parallel for
			for( int model=0; model<(int)frameData.matches.size(); model++) {
				
				vector< vector< pair<Pt<2>, int> > > pointsPerImage( frameData.images.size() ) ;
				
				for( int match=0; match<(int)frameData.matches[model].size(); match++)
					pointsPerImage[ frameData.matches[model][match].imageIdx ].push_back( make_pair( frameData.matches[model][match].coord2D, match ) );

				for( int i=0; i<(int)frameData.images.size(); i++ )
					CustomClustering( frameData.clusters[model], pointsPerImage[i], MinPts, MaxIterations );
			}
		}
	};
};
