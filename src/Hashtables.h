#ifndef _HASHTABLES_H
#define _HASHTABLES_H

#include "llvm/BasicBlock.h"
#include "Node.h"
#include <map>

using namespace llvm;

extern std::map<BasicBlock *,node *> nodes;

#endif
