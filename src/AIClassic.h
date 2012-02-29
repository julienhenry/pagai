#ifndef _AICLASSIC_H
#define _AICLASSIC_H

#include "AISimple.h"

using namespace llvm;

/// Pass implementing the basic abstract interpretation algorithm
class AIClassic : public AISimple {

	public:
		/// @brief Pass Identifier
		///
		/// It is crucial for LLVM's pass manager that
		/// this ID is different (in address) from a class to another,
		/// hence this cannot be factored in the base class.
		static char ID;	

	public:
		AIClassic(char &_ID, Apron_Manager_Type _man, bool _NewNarrow, bool _Threshold) : AISimple(_ID,_man,_NewNarrow, _Threshold) {
			init();
			passID.D = _man;
			passID.N = _NewNarrow;
			passID.TH = _Threshold;
		}
		
		AIClassic (): AISimple(ID) {
			init();
			passID.D = getApronManager();
			passID.N = useNewNarrowing();
			passID.TH = useThreshold();
		}

		void init()
			{
				aman = new AbstractManClassic();
				passID.T = SIMPLE;
				//Passes[SIMPLE] = passID;	
			}

		~AIClassic () {
			}

		const char *getPassName() const;
};

#endif
