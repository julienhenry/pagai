/**
 * \file recoverName.cc
 * \brief Implementation of the recoverName class
 * \author Rahul Nanda, Julien Henry
 */
#include<algorithm>
#include<fstream>

#include "recoverName.h"
#include "Debug.h"
#include "SMTpass.h"
#include "llvm/Analysis/Dominators.h"

#define MAX 0xFFFFFFFF

// map M1 for pass1
std::multimap<const Value*,Info> M1;

//Basic Block Mapping: 
//Block_line maps a basic block to starting line no. in original source code
//Block_column maps a basic block to column no. in original code.
std::map<BasicBlock*,int> Block_line,Block_column; 

std::map<Value*,Info> computed_mappings;

void recoverName::fill_info_set(BasicBlock * b, std::set<Info> * infos, Value * val) {
	for (BasicBlock::iterator I = b->begin(), E = b->end(); I != E; ++I) {
		if (const DbgValueInst *DVI=dyn_cast<DbgValueInst>(I)) {
			if (DVI->getValue() == val) {
				MDNode * MD = DVI->getVariable(); 
				Info varInfo = resolveMetDescriptor(MD);
				infos->insert(varInfo);
			}
		}
	}
	BasicBlock * pred = b->getUniquePredecessor();
	if (pred != NULL) fill_info_set(pred,infos,val);
	
	llvm::DomTreeNodeBase<llvm::BasicBlock>*dominator;
	if (pred_begin(b) != pred_end(b)) {
		dominator = DT->getNode(b)->getIDom();
	} else {
		dominator = NULL;
	}
}

Info recoverName::getMDInfos_rec(Value* v,std::set<Value*> & seen) {
	*Out << "getMD rec " << *v << "\n";
	if (computed_mappings.count(v)) return computed_mappings[v];
	///////
	PHINode * phi = dyn_cast<PHINode>(v);
	assert(phi != NULL);

	std::set<Info> res_infos;
	for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
		Value * val = phi->getIncomingValue(i);
		BasicBlock * b = phi->getIncomingBlock(i);
		std::set<Info> infos;
		fill_info_set(b,&infos,val);
		//if (infos.size() == 0 && !seen.count(val) && isa<PHINode>(val)) {
		if (infos.size() == 0 && !seen.count(val)) {
			std::set<Value*> s;
			s.insert(seen.begin(),seen.end());
			s.insert(val);
			infos.insert(getMDInfos_rec(val,s));
		}
		if (res_infos.empty()) {
			res_infos.insert(infos.begin(),infos.end());
		} else if (infos.size() > 0) {
			std::set<Info> new_res_infos;
			std::set<Info>::iterator it = res_infos.begin(), et = res_infos.end();
			for (; it != et; it++) {
				if (infos.count(*it)) new_res_infos.insert(*it);
			}
			res_infos.clear();
			res_infos.insert(new_res_infos.begin(),new_res_infos.end());
		}
	}
	computed_mappings[v] = *res_infos.begin();
	return *res_infos.begin();
}

Info recoverName::getMDInfos(const Value* V) {
	Value * v = const_cast<Value*>(V);
	if (computed_mappings.count(v)) return computed_mappings[v];
	
	///////
	if (PHINode * phi = dyn_cast<PHINode>(v)) {
		*Out << "getMD " << *phi << "\n";
		std::set<Value*> seen;
		seen.insert(phi);
		return getMDInfos_rec(v,seen);
	}
	///////
	
	std::pair<std::multimap<const Value*,Info>::iterator,std::multimap<const Value*,Info>::iterator> ret1;
	ret1=M1.equal_range(V);
	std::multimap<const Value*,Info>::iterator it;

	if(ret1.first!=ret1.second) {
		it=ret1.first;
		computed_mappings[v] = (it)->second;
		return (it)->second;
	}
	std::set<const Value*> seen;
	seen.insert(V);
	std::set<Info,compare_Info> possible_mappings = getPossibleMappings(V,&seen);

	if (possible_mappings.begin() == possible_mappings.end()) {
		DEBUG(
		*Out << "no possible mappings for " << *V << "\n...";
		);
		return Info(SMTpass::getVarName(v),-1,"unknown");
	}
	computed_mappings[v] = Info(*possible_mappings.begin());
	return Info(*possible_mappings.begin());
}

// for debugging purpose...
void recoverName::print_set(std::set<Info,compare_Info> * s) {
	std::set<Info>::iterator it = s->begin(), et = s->end();
	for (; it != et; it++) {
		(*it).display();
	}
	*Out << "\n";
}

std::set<Info,compare_Info> recoverName::getPossibleMappings(const Value * V, std::set<const Value *> * seen) {
	std::set<Info,compare_Info> res;
	std::pair<std::multimap<const Value*,Info>::iterator,std::multimap<const Value*,Info>::iterator> in_Map;
	in_Map=M1.equal_range(V);
	std::multimap<const Value*,Info>::iterator it;

	bool empty = true;
	for (it = in_Map.first; it!=in_Map.second; it++) {
		empty = false;
		res.insert(it->second);
	}
	if (empty) {
		// V should be a PHINode
		if (! isa<PHINode>(V)) return res; // can occur for instance when there is an undef as PHINode argument
		const PHINode * phi = dyn_cast<PHINode>(V);
		seen->insert(V);
		for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
			Value * v = phi->getIncomingValue(i);
			if (seen->count(v) > 0) continue;
			std::set<Info,compare_Info> s = getPossibleMappings(v,seen);
			if (res.empty()) {
				res.swap(s);
			} else {
				// computing the intersection
				std::set<Info,compare_Info>::iterator it1 = res.begin();
				std::set<Info,compare_Info>::iterator it2 = s.begin();
				while ( (it1 != res.end()) && (it2 != s.end()) ) {
				    if (*it1 < *it2) {
				    	res.erase(it1++);
				    } else if (*it2 < *it1) {
				    	++it2;
				    } else { // **it1 == **it2
				            ++it1;
				            ++it2;
				    }
				}
				// Anything left in set_1 from here on did not appear in set_2,
				// so we remove it.
				res.erase(it1, res.end());	
			}

		}
		seen->erase(V);
	}
	return res;
}

//process function involves calling two functions 'pass1' and then 'pass2', passing the argument Function*
//pass1 and pass2 create maps M1 and M2 for all const Value* present in Function* passed to process function.
int recoverName::process(Function *F) { 
	pass1(F);	
	std::multimap<const Value*,Info>::iterator itt;
	DEBUG(
		*Out<<"MAPPING OF VARIABLES ...\nMap1\n";
		for ( itt=M1.begin() ; itt != M1.end(); itt++ ) {
			*Out<< *(*itt).first << " => ";
			(*itt).second.display();
			*Out<<"\n";
		}
	);
	return 1;
}

int recoverName::getBasicBlockLineNo(BasicBlock* BB) {
#if 0
	Instruction * I = getFirstMetadata(BB);
	if (I == NULL) return -1;
	MDNode * MD = I->getMetadata(0);
	MDNode * MDNode_lexical_block = get_DW_TAG_lexical_block(MD);
	if(const ConstantInt *LineNo = dyn_cast<ConstantInt>(MDNode_lexical_block->getOperand(2)))
		return LineNo->getZExtValue();
	return -1;
#endif

	std::map<BasicBlock*,int>::iterator it;
	it=Block_line.find(BB);
	if(it!=Block_line.end())
	{
		return it->second;
	}
	else
		return -1;
}

int recoverName::getBasicBlockColumnNo(BasicBlock* BB) {
#if 0
	Instruction * I = getFirstMetadata(BB);
	if (I == NULL) return -1;
	MDNode * MD = I->getMetadata(0);
	MDNode * MDNode_lexical_block = get_DW_TAG_lexical_block(MD);
	if(const ConstantInt *LineNo = dyn_cast<ConstantInt>(MDNode_lexical_block->getOperand(3)))
		return LineNo->getZExtValue();
	return -1;
#endif

	std::map<BasicBlock*,int>::iterator it;
	it=Block_column.find(BB);
	if(it!=Block_column.end())
	{
		return it->second;
	}
	else
		return -1;
}

//this functions collects required information about a variable from a MDNode*, creates the corresponding object 
//of type Info and returns it.
Info recoverName::resolveMetDescriptor(MDNode* md) {
	std::string name,type;
	int lineNo,tag;
	int varNameLoc,lineNoLoc,typeLoc;

	bool assigned=false;
	if(const ConstantInt *Tag = dyn_cast<ConstantInt>(md->getOperand(0)))
	{
		tag=Tag->getZExtValue()-LLVM_DEBUG_VERSION; 	
		switch(tag)
		{
			case 256: //DW_TAG_AUTO_VARIABLE
			case 257: //DW_TAG_arg_variable
			case 258: //DW_TAG_return_variable
						varNameLoc=2;lineNoLoc=4;typeLoc=5;assigned=true;
						break;
			case 52: //DW_TAG_variable
						varNameLoc=3;lineNoLoc=7;typeLoc=8;assigned=true;
						break;
		}	
	}	
	if(assigned)
	{
		if (const MDString * MDS1 = dyn_cast<MDString>(md->getOperand(varNameLoc))) 
		{
			name=MDS1->getString().str();
    	} 		
		if(const ConstantInt *LineNo = dyn_cast<ConstantInt>(md->getOperand(lineNoLoc)))
		{
			lineNo=LineNo->getZExtValue();	
		}

		if(const MDNode *Type=dyn_cast<MDNode>(md->getOperand(typeLoc))) 
		{
			if(Type->getOperand(2)!=NULL){
				if(const MDString *MDS2 = dyn_cast<MDString>(Type->getOperand(2))) 
				{
					type=MDS2->getString().str();
    			}}
		//using the tag value to find type name..
			else if(const ConstantInt *Tag = dyn_cast<ConstantInt>(Type->getOperand(0)))
			{
				tag=Tag->getZExtValue()-LLVM_DEBUG_VERSION;
				switch(tag)
				{
					case 36: //DW_TAG_base_type
							type="base_type";
							break;
					case 16: //DW_TAG_reference_type 
							type="reference_type";
							break;
					case 15: //DW_TAG_pointer_type 
							type="pointer_type";
							break;
					case 5: //DW_TAG_formal_parameter
							type="formal_parameter";
							break;
					case 13: //DW_TAG_member
							type="member";
							break;
					case 22: //DW_TAG_typedef
							type="typedef";
							break;
					case 38: //DW_TAG_const_type
							type="const_type";
							break;
					case 53: //DW_TAG_volatile_type 
							type="volatile_type";
							break;
					case 55: //DW_TAG_restrict_type
							type="restrict_type";
							break;
					default: type="Unknown_type!!";
				}
			}
		}
		Info res(name,lineNo,type);
		return res;
	}
	Info res("",-1,"");
	return res;
}

void recoverName::update_line_column(Instruction * I, unsigned & line, unsigned & column) {
	if(I->hasMetadata() && ! isa<DbgValueInst>(I) && ! isa<DbgDeclareInst>(I)) {
		unsigned l,c;
		if(MDNode *BlockMD=dyn_cast<MDNode>(I->getMetadata(0))) {
			if(const ConstantInt *BBLineNo = dyn_cast<ConstantInt>(BlockMD->getOperand(0))) {
				l=BBLineNo->getZExtValue();
			}
			if(const ConstantInt *BBColumnNo = dyn_cast<ConstantInt>(BlockMD->getOperand(1))) {
				c=BBColumnNo->getZExtValue();
			}
			if(l<line) {
				line=l;
				column=c;
			} else if(l==line && c<column) {
				column=c;
			}
		}
	}
}

//'pass1' reads llvm.dbg.declare and llvm.dbg.value instructions, maps bitcode variables(of type Value*)
//to original source variable(of type Info*) and stores them in multimap M1
void recoverName::pass1(Function *F) {
	MDNode * MD; 
	const Value* val;
	for (Function::iterator bb = F->begin(), e = F->end(); bb != e; ++bb) {
		unsigned bbline=MAX,bbcolumn=MAX;
		for (BasicBlock::iterator I = bb->begin(), E = bb->end(); I != E; ++I) {
			
			update_line_column(I,bbline,bbcolumn);	

			//now check if the instruction is of type llvm.dbg.value or llvm.dbg.declare
			bool dbgInstFlag=false;
			if(const DbgValueInst *DVI=dyn_cast<DbgValueInst>(I)) {
				val=DVI->getValue();
				MD = DVI->getVariable(); 
				dbgInstFlag=true;
			} else if(const DbgDeclareInst *DDI=dyn_cast<DbgDeclareInst>(I)) {	
				val=DDI->getAddress();
				MD = DDI->getVariable(); 
				dbgInstFlag=true;
			}

			if(dbgInstFlag) {
				Info varInfo=resolveMetDescriptor(MD);
				if(!varInfo.empty()) {
					std::pair<std::multimap<const Value*,Info>::iterator,std::multimap<const Value*,Info>::iterator> ret1;
					ret1=M1.equal_range(val);

					bool AlreadyMapped=false;
					//this is to check and avoid duplicate entries in the map M1 
					for(std::multimap<const Value*,Info>::iterator it=ret1.first;it!=ret1.second;it++) {
						if(varInfo == (it->second)) {
							AlreadyMapped=true;
							break;
						} 

					}
					if(!AlreadyMapped) {
						std::pair<const Value*,Info>hi=std::make_pair(val,varInfo);
						M1.insert(hi);
					}
				}
			}
		}
		Block_line.insert(std::make_pair(bb,bbline));

		//Block_column.insert(std::make_pair(bb,bbcolumn));
		
		// bbcolumn is set to 1, since new versions of LLVM always set it to 0
		// instead of the correct column number...
		Block_column.insert(std::make_pair(bb,1));
		//*Out << "(" << bbline << "," << bbcolumn << ")\n";
	}	
}

Instruction * recoverName::getFirstMetadata(Function * F) {
	Function::iterator it = F->begin(), et = F->end();
	for (;it!=et;it++) {
		Instruction * res = getFirstMetadata(it);
		if (res != NULL) return res;
	}
	return NULL;
}

Instruction * recoverName::getFirstMetadata(BasicBlock * b) {
	BasicBlock::iterator it = b->begin(), et = b->end();
	for (;it!=et;it++) {
		if (it->hasMetadata()) return it;
	}
	return NULL;
}

int recoverName::getFunctionLineNo(Function* F) {
	Instruction * I = getFirstMetadata(F);
	MDNode * MD = I->getMetadata(0);
	//MD = dyn_cast<MDNode>(MD->getOperand(2));
	MDNode * MDNode_subprogram = get_DW_TAG_subprogram(MD);
	const ConstantInt *LineNo = dyn_cast<ConstantInt>(MDNode_subprogram->getOperand(19));
	return LineNo->getZExtValue();
}

std::string recoverName::getSourceFileName(Function * F) {
	// we only need the first instruction
	Instruction * I = getFirstMetadata(F);
	if (I == NULL) return "";
	MDNode * MD = I->getMetadata("dbg");
	if (MD == NULL) return "";
	MDNode * MDNode_file_type = get_DW_TAG_file_type(MD);
	MD = dyn_cast<MDNode>(MDNode_file_type->getOperand(1));
	const MDString * Filename = dyn_cast<MDString>(MD->getOperand(0));

	std::string s = Filename->getString().str();
	return s;
}

std::string recoverName::getSourceFileDir(Function * F) {
	// we only need the first instruction
	Instruction * I = getFirstMetadata(F);
	if (I == NULL) return "";
	MDNode * MD = I->getMetadata("dbg");
	if (MD == NULL) return "";
	MDNode * MDNode_file_type = get_DW_TAG_file_type(MD);
	MD = dyn_cast<MDNode>(MDNode_file_type->getOperand(1));
	const MDString * Filename = dyn_cast<MDString>(MD->getOperand(1));

	std::string s = Filename->getString().str();
	return s;
}

bool recoverName::is_readable(Function * F) { 
	std::string dir = getSourceFileDir(F);
	std::string name = getSourceFileName(F);
	if (dir.size() == 0 || name.size() == 0) return false;
	std::string dirname = dir + "/" + name;
    std::ifstream File(dirname.c_str()); 
    return !File.fail(); 
}

bool recoverName::hasMetadata(Module * M) {
	Module::iterator it = M->begin(), et = M->end();
	for (;it!=et;it++) {
		if (hasMetadata(it)) return true;
	}
	return false;
}

bool recoverName::hasMetadata(Function * F) {
	Function::iterator it = F->begin(), et = F->end();
	for (;it!=et;it++) {
		if (hasMetadata(it)) return true;
	}
	return false;
}

bool recoverName::hasMetadata(BasicBlock * b) {
	BasicBlock::iterator it = b->begin(), et = b->end();
	for (;it!=et;it++) {
		if (it->hasMetadata()) return true;
	}
	return false;
}

MDNode * recoverName::get_DW_TAG_lexical_block(MDNode * MD) {
	const ConstantInt *Tag = dyn_cast<ConstantInt>(MD->getOperand(0));
	int tag = Tag->getZExtValue()-LLVM_DEBUG_VERSION;
	MDNode * N;
	switch (tag) {
		case 11: // DW_TAG_lexical_block
			return MD;
		default:
			DEBUG(
				*Out << "MD\n";
				*Out << *MD << "\n";
			);
			N = dyn_cast<MDNode>(MD->getOperand(2));
			break;
	}
	return get_DW_TAG_lexical_block(N);
}

MDNode * recoverName::get_DW_TAG_subprogram(MDNode * MD) {
	const ConstantInt *Tag = dyn_cast<ConstantInt>(MD->getOperand(0));
	int tag = Tag->getZExtValue()-LLVM_DEBUG_VERSION;
	MDNode * N;
	switch (tag) {
		case 11: // DW_TAG_lexical_block
			N = dyn_cast<MDNode>(MD->getOperand(2));
			break;
		case 46: //DW_TAG_subprogram
			return MD;
		default:
			N = dyn_cast<MDNode>(MD->getOperand(2));
			break;
	}
	return get_DW_TAG_subprogram(N);
}

MDNode * recoverName::get_DW_TAG_file_type(MDNode * MD) {
	const ConstantInt *Tag = dyn_cast<ConstantInt>(MD->getOperand(0));
	int tag = Tag->getZExtValue()-LLVM_DEBUG_VERSION;
	MDNode * N;
	switch (tag) {
		case 11: // DW_TAG_lexical_block
			N = dyn_cast<MDNode>(MD->getOperand(2));
			break;
		case 46: //DW_TAG_subprogram
			N = dyn_cast<MDNode>(MD->getOperand(2));
			break;
		case 41: // DW_TAG_file_type
			return MD;
		default:
			N = dyn_cast<MDNode>(MD->getOperand(2));
			break;
	}
	return get_DW_TAG_file_type(N);
}
