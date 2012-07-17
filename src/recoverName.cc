#include "recoverName.h"
#include<algorithm>
#define MAX 0xFFFFFFFF

// map M1 for pass1 and M2 for pass2
std::multimap<const Value*,Info*> M1,M2;

//Basic Block Mapping to the starting line no. and column no. of basicblock in
//original code.
std::map<BasicBlock*,int> BBM1,BBM2; 

Info* recoverName::getMDInfos(const Value* V) {
	std::pair<std::multimap<const Value*,Info*>::iterator,std::multimap<const Value*,Info*>::iterator> ret1,ret2;
	ret1=M1.equal_range(V);ret2=M2.equal_range(V);
	std::multimap<const Value*,Info*>::iterator it;
	if(ret2.first!=ret2.second)
		it=ret2.first;
	else
		it=ret1.first;
	//it++;
	return (it)->second;
}

int recoverName::process(Function *F) {
	M1.clear();M2.clear();
	pass1(F);	
	pass2(F);

	//to display the multimap for Function *F..
	std::multimap<const Value*,Info*>::iterator itt;
	*Out<<"MAPPING OF VARIABLES ...\nMap1\n";
	for ( itt=M1.begin() ; itt != M1.end(); itt++ )
	{
		*Out<< *(*itt).first << " => ";//(*itt).second->display(); *Out<<"\n";
		Info* IN=getMDInfos(itt->first);
		(*itt).second->display();
		//IN->display(); 
		*Out<<"\n";
	}
	*Out<<"Map2\n";
	for ( itt=M2.begin() ; itt != M2.end(); itt++ )
	{
		*Out<< *(*itt).first << " => ";//(*itt).second->display(); *Out<<"\n";
		Info* IN=getMDInfos(itt->first);
		(*itt).second->display();
		//IN->display(); 
		*Out<<"\n";
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
	if(assigned){
	if (const MDString * MDS1 = dyn_cast<MDString>(md->getOperand(varNameLoc))) 
	{
		name=MDS1->getString().str();
    } 		
	if(const ConstantInt *LineNo = dyn_cast<ConstantInt>(md->getOperand(lineNoLoc)))
	{
		lineNo=LineNo->getZExtValue();	
	}

	bool foundTypeName=false;
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
	return obj;}
	return NULL;
}

void recoverName::pass1(Function *F) {
	MDNode * md; 
	
	*Out<<"Function:"<<F->getName()<<"\n";
	const Value* BCVar;
	
	for (Function::iterator bb = F->begin(), e = F->end(); bb != e; ++bb)
	{
		bool bbLineFlag=false;
		unsigned bblineNo,bbcolumnNo,bblineNoMin=MAX,bbcolumnNoMin=MAX;
		for (BasicBlock::iterator I = bb->begin(), E = bb->end(); I != E; ++I)
		{
			if(I->hasMetadata())
			{
				SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
				I->getAllMetadata(MDs);

				SmallVectorImpl<std::pair<unsigned, MDNode *> >::iterator MI = MDs.begin();
				if(MDNode *BlockMD=dyn_cast<MDNode>(MI->second))
				{
					if(const ConstantInt *BBLineNo = dyn_cast<ConstantInt>(BlockMD->getOperand(0)))
					{
						bblineNo=BBLineNo->getZExtValue();
						//BBM1.insert(std::make_pair(bb,BBLineNo->getZExtValue())); //BasicBlock LineNo. stored in a map
						//bbLineFlag=true;
					}
					if(const ConstantInt *BBColumnNo = dyn_cast<ConstantInt>(BlockMD->getOperand(1)))
					{
						bbcolumnNo=BBColumnNo->getZExtValue();
						//BBM2.insert(std::make_pair(bb,BBColumnNo->getZExtValue())); //BasicBlock ColumnNo. stored in a map
						//bbLineFlag=true;
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
					std::multimap<const Value*,Info*>::iterator it;
					bool check=true;

					ret1=M1.equal_range(BCVar);
					for(it=ret1.first;it!=ret1.second;it++)
					{
						if(varInfo->isEqual(it->second))
						{
							check=false;
							break;
						}
					}
					if(check)
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

bool recoverName::heyPHINode(
		const PHINode *PHIN,
		std::vector<const PHINode*>& PHIvector,
		std::vector<Info*>& v) {

	std::vector<Info*> v2,myV(10);
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
	bool one=true;
	if(const PHINode *PHIN2=dyn_cast<PHINode>(BCVar))
	{
		if(ret2.first==ret2.second)
		{
			one=heyPHINode(PHIN2,PHIvector,v);
		}
	}
	else if(ret1.first==ret1.second)
	{
		*Out<<"foo..\n";
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
		bool two=true;
		if(const PHINode *PHIN2=dyn_cast<PHINode>(BCVar))
		{
			if(ret2.first==ret2.second)
			{
				two=heyPHINode(PHIN2,PHIvector,v);
			}
		}
		else if(ret1.first==ret1.second)
		{
			*Out<<"foo..\n";
		}
		
		for(it=ret1.first;it!=ret1.second;it++)
			v2.push_back(it->second);
		for(it=ret2.first;it!=ret2.second;it++)
			v2.push_back(it->second);
		
		if(i==1 && !one && two)
			v.assign(v2.begin(),v2.end());
		else if(v.size() && v2.size())
		{
			compare_1 compObj;
			sort(v.begin(),v.end(),compObj);
			sort(v2.begin(),v2.end(),compObj);
						
			vit=set_intersection(v.begin(),v.end(),v2.begin(),v2.end(),myV.begin(),compare_1());
						
			v.assign(myV.begin(),vit);
			myV.clear();
		}

		
		/* 
		*Out<<"Intersection :"<<*PHIN;
						// *Out<<"Size="<<myV.size()<<"\n";
						//if(myV.size()>1)
						for(vx=v.begin();vx!=v.end();vx++)
						{
							(*vx)->display();						
							*Out<<" ";
						}
		*Out<<"\n";*/
		
	}
	return true;
}

void recoverName::pass2(Function *F) {
	for (Function::iterator bb = F->begin(), e = F->end(); bb != e; ++bb) {	
		for (BasicBlock::iterator I = bb->begin(), E = bb->end(); I != E; ++I) {
			if(const PHINode *PHIN=dyn_cast<PHINode>(I)) //for each PHI Instruction
			{
				std::vector<const PHINode*> PHIvector;
				std::vector<Info*> v;
				std::vector<Info*>::iterator vx;

				heyPHINode(PHIN,PHIvector,v);

				for(vx=v.begin();vx!=v.end();vx++) {
					// *Out<<"HI ";(*vx)->display();*Out<<"\n";
					std::pair<const Value*,Info*>hi=std::make_pair(PHIN,*vx);
					M2.insert(hi);
				}
			}
		}
	}
}
