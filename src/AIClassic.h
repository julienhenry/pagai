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
		AIClassic(char &_ID) : AISimple(_ID) {init();}
		AIClassic (): AISimple(ID) {init();}

		void init()
			{
				aman = new AbstractManClassic();
				passID = SIMPLE;
				Passes[SIMPLE] = passID;	
			}

		~AIClassic () {
			}

		const char *getPassName() const;
};

#endif
