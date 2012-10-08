/**
 * \file recoverName.cc
 * \brief Implementation of the recoverName class
 * \author Rahul Nanda, Julien Henry
 */
#include<algorithm>
#include<fstream>

#include "recoverName.h"
#include "Debug.h"

#define MAX 0xFFFFFFFF

// map M1 for pass1 and M2 for pass2
std::multimap<const Value*,Info*> M1,M2;

//Basic Block Mapping: 
//BBM1 maps a basic block to starting line no. in original source code
//BBM2 maps a basic block to column no. in original code.
std::map<BasicBlock*,int> BBM1,BBM2; 

Info* recoverName::getMDInfos(const Value* V) //retrieve appropriate Info* mapped to a const Value* from the maps.
{
	std::pair<std::multimap<const Value*,Info*>::iterator,std::multimap<const Value*,Info*>::iterator> ret1,ret2;
	ret1=M1.equal_range(V);
	ret2=M2.equal_range(V);
	std::multimap<const Value*,Info*>::iterator it;

	//if a Value* is present in Map M2, we return the corresponding Info* mapped to it
	//else we return the mapped Info* from M1. NOTE:In this case, a NULL value can also be returned. 
	if(ret2.first!=ret2.second)
		it=ret2.first;
	else 
		it=ret1.first;
	
	return (it)->second;
}

//process function involves calling two functions 'pass1' and then 'pass2', passing the argument Function*
//pass1 and pass2 create maps M1 and M2 for all const Value* present in Function* passed to process function.
int recoverName::process(Function *F) 
{ 
	//M1.clear();
	//M2.clear();

	pass1(F);	
	pass2(F);

	std::multimap<const Value*,Info*>::iterator itt;

	//simply displaying the multimaps M1 and M2 created for Function *F..
	DEBUG(
	*Out<<"MAPPING OF VARIABLES ...\nMap1\n";
	);
	for ( itt=M1.begin() ; itt != M1.end(); itt++ )
	{
		DEBUG(
		*Out<< *(*itt).first << " => ";
		);
		Info* IN=getMDInfos(itt->first);
		DEBUG(
		(*itt).second->display();
		//IN->display(); 
		*Out<<"\n";
		);
	}
	DEBUG(
	*Out<<"Map2\n";
	);
	for ( itt=M2.begin() ; itt != M2.end(); itt++ )
	{
		DEBUG(
		*Out<< *(*itt).first << " => ";
		);
		Info* IN=getMDInfos(itt->first);
		DEBUG(
		(*itt).second->display();
		//IN->display(); 
		*Out<<"\n";
		);
	}
	return 1;
}

int recoverName::getBasicBlockLineNo(BasicBlock* BB) {
	std::map<BasicBlock*,int>::iterator it;
	it=BBM1.find(BB);
	if(it!=BBM1.end())
	{
		return it->second;
	}
	else
		return -1;
}

int recoverName::getBasicBlockColumnNo(BasicBlock* BB) {
	std::map<BasicBlock*,int>::iterator it;
	it=BBM2.find(BB);
	if(it!=BBM2.end())
	{
		return it->second;
	}
	else
		return -1;
}

//this functions collects required information about a variable from a MDNode*, creates the corresponding object 
//of type Info* and returns it.
Info* resolveMetDescriptor(MDNode* md) {
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
		Info *obj=new Info(name,lineNo,type);
		return obj;
	}
	return NULL;
}

//'pass1' reads llvm.dbg.declare and llvm.dbg.value instructions, maps bitcode variables(of type Value*)
//to original source variable(of type Info*) and stores them in multimap M1
void recoverName::pass1(Function *F) {
	MDNode * md; 
	const Value* BCVar;
	for (Function::iterator bb = F->begin(), e = F->end(); bb != e; ++bb)
	{
		unsigned bblineNo,bbcolumnNo,bblineNoMin=MAX,bbcolumnNoMin=MAX;
		for (BasicBlock::iterator I = bb->begin(), E = bb->end(); I != E; ++I)
		{
				//////////////////
				//SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
				//I->getAllMetadata(MDs);
				//SmallVectorImpl<std::pair<unsigned, MDNode *> >::iterator MI = MDs.begin(), ME = MDs.end();	
			
				//*Out << *I << "\n";
				//for (; MI != ME; MI++) {
				//	*Out << " " << (MI->first) << " " << *(MI->second) << "\n";
				//	MDNode * node = MI->second;
				//	MDNode * MDNode_lexical_block = dyn_cast<MDNode>(node->getOperand(2));
				//	MDNode * MDNode_file_type = dyn_cast<MDNode>(MDNode_lexical_block->getOperand(4));
				//	*Out << * MDNode_file_type  << "\n"; 
				//}
				//*Out << "\n";
				//////////////////////////////
			if(I->hasMetadata() && ! isa<DbgValueInst>(I) && ! isa<DbgDeclareInst>(I))
			{
				SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
				I->getAllMetadata(MDs);

				SmallVectorImpl<std::pair<unsigned, MDNode *> >::iterator MI = MDs.begin();	
			
				if(MDNode *BlockMD=dyn_cast<MDNode>(MI->second))
				{
					if(const ConstantInt *BBLineNo = dyn_cast<ConstantInt>(BlockMD->getOperand(0)))
					{
						bblineNo=BBLineNo->getZExtValue();
					}
					if(const ConstantInt *BBColumnNo = dyn_cast<ConstantInt>(BlockMD->getOperand(1)))
					{
						bbcolumnNo=BBColumnNo->getZExtValue();
					}
					if(bblineNo<bblineNoMin)
					{
						bblineNoMin=bblineNo;
						bbcolumnNoMin=bbcolumnNo;
					}
					else if(bblineNo==bblineNoMin && bbcolumnNo<bbcolumnNoMin)
					{
						bbcolumnNoMin=bbcolumnNo;
					}
				}
			}

			//now to check if the instruction is of type llvm.dbg.value or llvm.dbg.declare
			bool dbgInstFlag=false;
			if(const DbgValueInst *DVI=dyn_cast<DbgValueInst>(I))
			{
				dbgInstFlag=true;
				BCVar=DVI->getValue();
				md = DVI->getVariable(); 
			}

			else if(const DbgDeclareInst *DDI=dyn_cast<DbgDeclareInst>(I))
			{	
				if((md=dyn_cast<MDNode>(DDI->getVariable()))&& (BCVar=dyn_cast<Value>(DDI->getAddress())))
				   dbgInstFlag=true;
			}	

			if(dbgInstFlag)
			{
				Info* varInfo=resolveMetDescriptor(md);
				if(varInfo!=NULL)
				{
					std::pair<std::multimap<const Value*,Info*>::iterator,std::multimap<const Value*,Info*>::iterator> ret1;
					ret1=M1.equal_range(BCVar);

					bool ifAlreadyMapped=false;
					//this is to check and avoid duplicate entries in the map M1 
					for(std::multimap<const Value*,Info*>::iterator it=ret1.first;it!=ret1.second;it++)
					{
						if(varInfo->isEqual(it->second))
						{
							ifAlreadyMapped=true;
							break;
						}
					}
					if(!ifAlreadyMapped)
					{
						std::pair<const Value*,Info*>hi=std::make_pair(BCVar,varInfo);
						M1.insert(hi);
					}
					dbgInstFlag=false;
				}
			}
		}
		BBM1.insert(std::make_pair(bb,bblineNoMin));
		BBM2.insert(std::make_pair(bb,bbcolumnNoMin));
	}	
}

//this functions evaluates a const PHINode* passed as argument, to compute its mapping to the corresponding Info*
//and stores them in std::vector<Info*> passed by reference to the function.
bool recoverName::evaluatePHINode(
		const PHINode *PHIN,
		std::vector<const PHINode*>& PHIvector, //stores PHINodes that have already been evaluated
		std::vector<Info*>& v) {

	std::vector<Info*> v2,intersectVector(10);
	std::vector<Info*>::iterator vit,vx;
	
	std::vector<const PHINode*>::iterator PHIvit;
	for(PHIvit=PHIvector.begin();PHIvit!=PHIvector.end();PHIvit++)
	{
		if((*PHIvit)->isIdenticalTo(PHIN))
		{
			return false;
		}
	}
	PHIvector.push_back(PHIN);

	std::pair<std::multimap<const Value*,Info*>::iterator,
	std::multimap<const Value*,Info*>::iterator> ret1,ret2;
	std::multimap<const Value*,Info*>::iterator it;
	
	Value* BCVar=PHIN->getIncomingValue(0);
	ret1=M1.equal_range(BCVar);
	ret2=M2.equal_range(BCVar);
	bool notAlreadyEvaluated1=true;

	//if the operand BCVar of the original PHINode is also a PHINode, then we first check if there is a mapping for BCVar in the map M2
	//if there is no such mapping in M2 for BCVar(which is of type PHINode), then we first call evaluatePHINode function on BCVar.  
	if(const PHINode *PHIN2=dyn_cast<PHINode>(BCVar)) 
	{
		if(ret2.first==ret2.second)
		{
			notAlreadyEvaluated1=evaluatePHINode(PHIN2,PHIvector,v);
		}
	}
	else if(ret1.first==ret1.second)
	{
		DEBUG(
			*Out<<"The argument "<<*BCVar<<" to the PHINode "<<*PHIN<<" has an empty mapping to any original source-variable !\n";
		);
	}

	for(it=ret1.first;it!=ret1.second;it++)
		v.push_back(it->second);
	for(it=ret2.first;it!=ret2.second;it++)
		v.push_back(it->second);
	
	int numIncomingValues=PHIN->getNumIncomingValues();
	for(int i=1;i<numIncomingValues;i++)
	{
		BCVar=PHIN->getIncomingValue(i);
		ret1=M1.equal_range(BCVar);	
		ret2=M2.equal_range(BCVar);
		bool notAlreadyEvaluated2=true;

		//if the operand BCVar of the original PHINode is also a PHINode, then we first check if there is a mapping for BCVar in the map M2
		//if there is no such mapping in M2 for BCVar(which is of type PHINode), then we first call evaluatePHINode function on BCVar. 
		if(const PHINode *PHIN2=dyn_cast<PHINode>(BCVar))
		{
			if(ret2.first==ret2.second)
			{
				notAlreadyEvaluated2=evaluatePHINode(PHIN2,PHIvector,v);
			}
		}
		else if(ret1.first==ret1.second)
		{
			DEBUG(
				*Out<<"The argument "<<*BCVar<<" to the PHINode "<<*PHIN<<" has an empty mapping to any original source-variable !\n";
			);
		}
		
		for(it=ret1.first;it!=ret1.second;it++)
			v2.push_back(it->second);
		for(it=ret2.first;it!=ret2.second;it++)
			v2.push_back(it->second);
		
		if(i==1 && !notAlreadyEvaluated1 && notAlreadyEvaluated2) //this to consider the case when the first operand of a PHINode is also a PHINOde and on which the evaluatePHINode
			v.assign(v2.begin(),v2.end());//function has already been called i.e. that first operand(of type const PHINode*) is already present in 
											//'PHIvector'
		else if(v.size() && v2.size())
		{
			compare_1 compObj;
			sort(v.begin(),v.end(),compObj);
			sort(v2.begin(),v2.end(),compObj);

			//intersection of vectors v and v2 is taken and stored in intersectVector 
			vit=set_intersection(v.begin(),v.end(),v2.begin(),v2.end(),intersectVector.begin(),compare_1());
						
			v.assign(intersectVector.begin(),vit); //since the intersection is taken repeatedly, intersection of two vectors is stored back  
												   //in the first vector
			
			intersectVector.clear();//clear the intersection vector for next iteration
		}

		//in case you want to display the intersection computed, remove the '/*' & '*/'below.. 
		/* 
						*Out<<"Intersection :"<<*PHIN;
						for(vx=v.begin();vx!=v.end();vx++)
						{
							(*vx)->display();						
							*Out<<" ";
						}
						*Out<<"\n";
		*/
		
	}
	return true;
}

//function pass2 simply calls evaluatePHINode function on every Instruction of type PHINode* and stores them alongwith
//the computed Info* objects in the map M2
void recoverName::pass2(Function *F) {
	for (Function::iterator bb = F->begin(), e = F->end(); bb != e; ++bb) {	
		for (BasicBlock::iterator I = bb->begin(), E = bb->end(); I != E; ++I) {
			if(const PHINode *PHIN=dyn_cast<PHINode>(I)) //for each PHI Instruction
			{
				std::vector<const PHINode*> PHIvector;
				std::vector<Info*> v;
				std::vector<Info*>::iterator vx;

				evaluatePHINode(PHIN,PHIvector,v);

				for(vx=v.begin();vx!=v.end();vx++) {
					// *Out<<"variable inserted in map M2 : ";(*vx)->display();*Out<<"\n";
					std::pair<const Value*,Info*>hi=std::make_pair(PHIN,*vx);
					M2.insert(hi);
				}
			}
		}
	}
}

Instruction * recoverName::getFirstMetadata(Function * F) {
	Function::iterator it = F->begin(), et = F->end();
	for (;it!=et;it++) {
		BasicBlock::iterator iit = it->begin(), eet = it->end();
		for (;iit!=eet;iit++) {
			if (iit->hasMetadata()) return iit;
		}
	}
	return NULL;
}

int recoverName::getFunctionLineNo(Function* F) {
	Instruction * I = getFirstMetadata(F);
	MDNode * MD = I->getMetadata(0);
	MDNode * MDNode_subprogram = get_DW_TAG_subprogram(MD);
	const ConstantInt *LineNo = dyn_cast<ConstantInt>(MDNode_subprogram->getOperand(7));
	return LineNo->getZExtValue();
}

std::string recoverName::getSourceFileName(Function * F) {
	// we only need the first instruction
	Instruction * I = getFirstMetadata(F);
	MDNode * MD = I->getMetadata(0);
	MDNode * MDNode_file_type = get_DW_TAG_file_type(MD);
	const MDString * Filename = dyn_cast<MDString>(MDNode_file_type->getOperand(1));

	std::string s = Filename->getString().str();
	return s;
}

std::string recoverName::getSourceFileDir(Function * F) {
	// we only need the first instruction
	Instruction * I = getFirstMetadata(F);
	MDNode * MD = I->getMetadata(0);
	MDNode * MDNode_file_type = get_DW_TAG_file_type(MD);
	const MDString * Filename = dyn_cast<MDString>(MDNode_file_type->getOperand(2));

	std::string s = Filename->getString().str();
	return s;
}

bool recoverName::is_readable(Function * F) { 
	std::string name = getSourceFileDir(F) + "/" + getSourceFileName(F);
    std::ifstream File(name.c_str()); 
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
			N = dyn_cast<MDNode>(MD->getOperand(1));
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
			N = dyn_cast<MDNode>(MD->getOperand(4));
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
