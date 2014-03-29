#include <fstream>

#include "AnalysisPass.h"

void AnalysisPass::generateAnnotatedFiles(Module * M, bool outputfile) {
	if (!useSourceName()) 
		return;
	std::map<std::string,std::multimap<std::pair<int,int>,BasicBlock*> > files;

	for (Module::iterator mIt = M->begin() ; mIt != M->end() ; ++mIt) {
		Function * F = mIt;
		if (!F->isDeclaration())
			computeResultsPositions(F,&files);
	}

	llvm::raw_ostream *Output;
	if (outputfile) {
		std::string OutputFilename(getAnnotatedFilename());
		// open the output stream
		raw_fd_ostream *FDOut = NULL;
		std::string error;
		FDOut = new raw_fd_ostream(OutputFilename.c_str(), error);
		if (!error.empty()) {
			errs() << error << '\n';
			delete FDOut;
			return;
		}
		Output = new formatted_raw_ostream(*FDOut, formatted_raw_ostream::DELETE_STREAM);
	} else {
		Output = Out;
	}

	std::map<std::string,std::multimap<std::pair<int,int>,BasicBlock*> >::iterator it = files.begin(), et = files.end();
	for(; it != et; it++) {
		std::string filename = it->first;
		std::multimap<std::pair<int,int>,BasicBlock*> positions = it->second;
		generateAnnotatedCode(Output,filename,&positions);
	}
	if (outputfile) {
		delete Output;
	}
}

void AnalysisPass::generateAnnotatedCode(
		llvm::raw_ostream * oss, 
		std::string filename, 
		std::multimap<std::pair<int,int>,BasicBlock*> * positions) {

	// we open the source file in read mode
	std::ifstream sourceFile(filename.c_str());
	int lineNo = 0;
	int columnNo;

	std::multimap<std::pair<int,int>,BasicBlock*>::iterator Iit, Iet;
	Iit = positions->begin();
	Iet = positions->end();

	while (Iit->first.first < 0) Iit++;

	if ( sourceFile )
	{
		std::string line;
		while ( std::getline( sourceFile, line ) )
		{
			lineNo++;
			columnNo = 1;
			std::string::iterator it = line.begin(); 
			while (it < line.end()) {
				
				if (lineNo == Iit->first.first && columnNo == Iit->first.second) {
					// here, we can print an invariant !
					BasicBlock * b = Iit->second;
					// compute the left padding
					std::string left = line.substr(0,columnNo-1);
					printInvariant(b,left,oss);
					Iit++;
				}
				if (lineNo == Iit->first.first && columnNo == Iit->first.second) {
					continue;
				} else {
					*oss << *it;
					columnNo++;
					it++;
				}
			}
			*oss << "\n";
		}
	}
}
		
void AnalysisPass::printResult(Function * F) {
	if (generateMetadata()) InstrumentLLVMBitcode(F);
	if (quiet_mode() || useSourceName()) return;
	printResult_oldoutput(F);
}

void AnalysisPass::printResult_oldoutput(Function * F) {
	BasicBlock * b;
	Node * n;
	Pr * FPr = Pr::getInstance(F);
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		b = i;
		n = Nodes[b];
		if ((!printAllInvariants() && FPr->inPr(b) && !ignored(F)) ||
		(printAllInvariants() && n->X_s.count(passID) && n->X_s[passID] != NULL && !ignored(F))) {
			Out->changeColor(raw_ostream::MAGENTA,true);

			Instruction * Inst = b->getFirstNonPHI();
			//Instruction * Inst = &b->front();
			std::vector<Value*> arr;
			
			if (generateMetadata()) {
				n->X_s[passID]->to_MDNode(Inst,&arr);
				LLVMContext& C = Inst->getContext();
				MDNode* N = MDNode::get(C,arr);
				Inst->setMetadata("pagai.invariant", N);
			}

			*Out << "\n\nRESULT FOR BASICBLOCK: -------------------" << *b << "-----\n";
			Out->resetColor();
			//n->X_i[passID]->print(true);
			n->X_s[passID]->print(true);
			if (FPr->inAssert(b)) {
				if (n->X_s[passID]->is_bottom()) {
					Out->changeColor(raw_ostream::GREEN,true);
					*Out << "assert OK\n";
				} else {
					Out->changeColor(raw_ostream::RED,true);
					*Out << "assert not proved\n";
				}
				Out->resetColor();
			}
			if (FPr->inUndefBehaviour(b)) {
				//
				//
				if (n->X_s[passID]->is_bottom()) {
					Out->changeColor(raw_ostream::GREEN,true);
					*Out << "safe\n";
				} else {
					Out->changeColor(raw_ostream::RED,true);
					*Out << "unsafe\n";
					*Out << getUndefinedBehaviourMessage(b) << "\n";
				}
				Out->resetColor();
			}
		}
	}
	*Out << Total_time[passID][F]->seconds() << "." << Total_time[passID][F]->microseconds() << " seconds\n";
	//*Out << SMT_time.tv_sec << " " << SMT_time.tv_usec  << " SMT_TIME " << "\n";
	*Out << "ASC ITERATIONS " << asc_iterations[passID][F] << "\n" ;
	*Out << "DESC ITERATIONS " << desc_iterations[passID][F] << "\n" ;
}

std::string AnalysisPass::getUndefinedBehaviourMessage(BasicBlock * b) {
	return "possible undefined behavior";
}

