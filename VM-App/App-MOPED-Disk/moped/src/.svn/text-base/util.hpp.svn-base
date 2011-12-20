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

#include <omp.h>
#include <deque>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#define foreach( i, c ) for( typeof((c).begin()) i##_hid=(c).begin(), *i##_hid2=((typeof((c).begin())*)1); i##_hid2 && i##_hid!=(c).end(); ++i##_hid) for( typeof( *(c).begin() ) &i=*i##_hid, *i##_hid3=(typeof( *(c).begin() )*)(i##_hid2=NULL); !i##_hid3 ; ++i##_hid3, ++i##_hid2) 
#define eforeach( i, it, c ) for( typeof((c).begin()) it=(c).begin(), i##_hid = (c).begin(), *i##_hid2=((typeof((c).begin())*)1); i##_hid2 && it!=(c).end(); (it==i##_hid)?++it,++i##_hid:i##_hid=it) for( typeof(*(c).begin()) &i=*it, *i##_hid3=(typeof( *(c).begin() )*)(i##_hid2=NULL); !i##_hid3 ; ++i##_hid3, ++i##_hid2) 

#define GET_CONFIG( varName ) config[ MopedNS::toString( _stepName ) + ":" + MopedNS::toString( _alg ) +":" + string(__FI##LE__).substr( ( string(__FI##LE__).find_last_of("/\\") + 1 + string(__FI##LE__).size() ) % string(__FI##LE__).size(), string(__FI##LE__).size() - ( string(__FI##LE__).find_last_of("/\\") + 1 + string(__FI##LE__).size() ) % string(__FI##LE__).size() -4  ) + "/" #varName ] = MopedNS::toString( varName )
#define SET_CONFIG( varName ) configUpdated = MopedNS::fromString( varName, config[ MopedNS::toString( _stepName ) + ":" + MopedNS::toString( _alg ) +":" + string(__FI##LE__).substr( ( string(__FI##LE__).find_last_of("/\\") + 1 + string(__FI##LE__).size() ) % string(__FI##LE__).size(), ( string(__FI##LE__).find_last_of("/\\") + 1 + string(__FI##LE__).size() ) % string(__FI##LE__).size() -4 ) +"/" #varName ] ) || configUpdated

namespace MopedNS {


	struct FrameData {

		struct DetectedFeature {
				
			int imageIdx;			
			
			// Image coordinate where the feature was detected
			Pt<2> coord2D;

			// Feature detected in the image
			vector<float> descriptor;	
		};
		
		struct Match {
			
			int imageIdx;			
			
			Pt<2> coord2D;
			
			Pt<3> coord3D;
		};
		
		typedef list<int> Cluster;
		
		vector<SP_Image> images;
		
		map< string, vector< DetectedFeature > > detectedFeatures;
		
		vector< vector< Match > > matches;
		
		vector< vector< Cluster> > clusters;
		
		list<SP_Object> *objects;
		
		
		
		
		
		int correctMatches, incorrectMatches;
		vector< vector< Cluster> > oldClusters;
		list<SP_Object> oldObjects;
		map<string, Float> times;
	};

	
	class MopedAlg {
	public:


		vector<SP_Model> *models;
		
		bool capable;
		bool configUpdated;
	public:

		string _stepName;
		int _alg;
	
		MopedAlg() { capable = true; configUpdated=true; }
	
		bool isCapable() const { return capable; }

		void setStepNameAndAlg( string &stepName, int alg ) {
			
			_stepName = stepName;
			_alg = alg;
		}
		
		virtual void modelsUpdated( vector<SP_Model> &_models) {

			models = &_models;
			configUpdated=true;
		}
		
		
	
		virtual void getConfig( map<string,string> &config ) const {};
		virtual void setConfig( map<string,string> &config ) {};
		
		virtual void process( FrameData &frameData ) = 0;
	};


	struct MopedStep : public vector< shared_ptr<MopedAlg> > {
	
		MopedAlg *getAlg() { 
			foreach( alg, *this ) 
				if( alg->isCapable() ) 
					return alg.get();
			return NULL; 
		}
	};
	
	struct MopedPipeline : public vector< MopedStep > {
		
		
		map< string, int > fromStepNameToIndex;
		
		
		void addAlg( string stepName, MopedAlg *mopedAlg ) { 
			
			int step;
			if( fromStepNameToIndex.find( stepName ) == fromStepNameToIndex.end() ) {

				step = fromStepNameToIndex.size();
				fromStepNameToIndex[ stepName ] = step;
			} else {
				
				step = fromStepNameToIndex[ stepName ];
			}
				
			if( step >= (int)this->size() ) this->resize( step+1 );
			
			mopedAlg->setStepNameAndAlg( stepName, (*this)[step].size() );
			
			(*this)[step].push_back( shared_ptr<MopedAlg>( mopedAlg ) );
		}
		
		
		list<MopedAlg *> getAlgs( bool onlyActive = false ) {
			
			list<MopedAlg *> algs;
			
			foreach( mopedStep, *this )
				if( !onlyActive )
					foreach( alg, mopedStep )
						algs.push_back( alg.get() );
				else
					if( mopedStep.getAlg() )
						algs.push_back( mopedStep.getAlg() );
						
			return algs;
		}
	};
}
