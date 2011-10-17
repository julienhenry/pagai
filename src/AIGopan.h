#ifndef _AIGOPAN_H
#define _AIGOPAN_H

#include "AISimple.h"

using namespace llvm;

/// @brief Gopan&Reps Implementation.
///
/// This class is almost identical to AIClassic, the only difference
/// being the abstract domain (which uses a main value to decide which
/// paths are to be explored, and a pilot value to actually compute
/// the invariants).
class AIGopan : public AISimple {

	public:
		/// @brief Pass Identifier
		///
		/// It is crucial for LLVM's pass manager that
		/// this ID is different (in address) from a class to another,
		/// hence this cannot be factored in the base class.
		static char ID;	

	public:

		AIGopan(char &_ID, Apron_Manager_Type _man) : AISimple(_ID,_man) {
			init();
			passID.D = _man;
		}
		
		AIGopan (): AISimple(ID) {
			init();
			passID.D = getApronManager();
		}

		void init()
			{
				aman = new AbstractManGopan();
				//aman = new AbstractManClassic();
				passID.T = LOOKAHEAD_WIDENING;
				//Passes[LOOKAHEAD_WIDENING] = passID;	
			}

		~AIGopan () {
			}

		const char *getPassName() const;
};

#endif
