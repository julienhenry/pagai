#ifndef _HASHTABLES_H
#define _HASHTABLES_H

#include "llvm/BasicBlock.h"
#include "node.h"
#include <map>

using namespace llvm;

extern std::map<BasicBlock *,node *> nodes;

#endif
