#ifndef _EXECUTE_H
#define _EXECUTE_H

#include <string>

#include "llvm/Support/FormattedStream.h"

using namespace llvm;

class execute {
public :
	void exec(std::string InputFilename, std::string OutputFilename);
};

#endif
