/**
 * \file Execute.cc
 * \brief Implementation of the Execute class
 * \author Julien Henry
 */
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/system_error.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/IPO.h"

#include "AIpf.h"
#include "AIpf_incr.h"
#include "AIopt.h"
#include "AIopt_incr.h"
#include "AIGopan.h"
#include "AIClassic.h"
#include "AIdis.h"
#include "AIGuided.h"
#include "ModulePassWrapper.h"
#include "Node.h"
#include "Execute.h"
#include "Live.h"
#include "SMTpass.h"
#include "SMTlib.h"
#include "Compare.h"
#include "CompareDomain.h"
#include "CompareNarrowing.h"
#include "Analyzer.h"
#include "GenerateSMT.h"
#include "instrOverflow.h"
#include "globaltolocal.h"
#include "taginline.h"
#include "RemoveUndet.h"

#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/IR/Module.h"

#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"

using namespace llvm;

static cl::opt<std::string>
DefaultDataLayout("default-data-layout", 
          cl::desc("data layout string to use if not specified by module"),
          cl::value_desc("layout-string"), cl::init(""));

bool is_Cfile(std::string InputFilename) {
	if (InputFilename.compare(InputFilename.size()-2,2,".c") == 0)
		return true;
	return false;
}

void execute::exec(std::string InputFilename, std::string OutputFilename) {

	raw_fd_ostream *FDOut = NULL;

	if (OutputFilename != "") {

		std::string error;
		FDOut = new raw_fd_ostream(OutputFilename.c_str(), error);
		if (!error.empty()) {
			errs() << error << '\n';
			delete FDOut;
			return;
		}
		Out = new formatted_raw_ostream(*FDOut, formatted_raw_ostream::DELETE_STREAM);

		// Make sure that the Output file gets unlinked from the disk if we get a
		// SIGINT
		//sys::RemoveFileOnSignal(sys::Path(OutputFilename));
	} else {
		Out = &llvm::outs();
		Out->SetUnbuffered();
	}
	
	llvm::Module * M;
	std::auto_ptr<Module> M_ptr;

		// Arguments to pass to the clang frontend
		std::vector<const char *> args;
		args.push_back(InputFilename.c_str());
		args.push_back("-g");
		if (check_overflow()) {
			//args.push_back("-ftrapv");
			args.push_back("-fsanitize=bool");
			args.push_back("-fsanitize=enum");
			args.push_back("-fsanitize=null");
			args.push_back("-fsanitize=signed-integer-overflow");
			args.push_back("-fsanitize=unsigned-integer-overflow");
			args.push_back("-fsanitize=integer-divide-by-zero");
			args.push_back("-fsanitize=float-divide-by-zero");
			args.push_back("-fsanitize=float-cast-overflow");
			args.push_back("-fsanitize=array-bounds");
			args.push_back("-fsanitize=local-bounds");
			//args.push_back("-fsanitize=local-bounds");
		}

		llvm::OwningPtr<clang::CodeGenAction> Act(new clang::EmitLLVMOnlyAction());
		 
		clang::DiagnosticOptions * Diagopt = new clang::DiagnosticOptions();

		clang::DiagnosticIDs *  DiagID = new clang::DiagnosticIDs();
		clang::DiagnosticsEngine * Diags = new clang::DiagnosticsEngine(DiagID, Diagopt);
		clang::DiagnosticConsumer * client = new clang::DiagnosticConsumer();
		Diags->setClient(client);

		// Create the compiler invocation
		llvm::OwningPtr<clang::CompilerInvocation> CI(new clang::CompilerInvocation);
		clang::CompilerInvocation::CreateFromArgs(*CI, &args[0], &args[0] + args.size(), *Diags);
		
		clang::CompilerInstance Clang;
		// equivalent to the -gcolumn-info command line option for clang
		CI->getCodeGenOpts().DebugColumnInfo = 1;
		Clang.setInvocation(CI.take());
		Clang.setDiagnostics(Diags);

	if (!is_Cfile(InputFilename)) {
		SMDiagnostic SM;
		LLVMContext & Context = getGlobalContext();
		M = ParseIRFile(InputFilename,SM,Context); 
	} else {

		//Clang.createDiagnostics(args.size(), &args[0]);
		
		if (!Clang.ExecuteAction(*Act)) {
			*Out << "Unable to produce LLVM bitcode. Please use Clang with the appropriate options.\n";
		    return;
		}
		llvm::Module *module = Act->takeModule();
		M = module;
	}

	if (M == NULL) {
		*Out << "ERROR: Unable to read bitcode file.\n";
		return;
	}

	PassRegistry &Registry = *PassRegistry::getPassRegistry();
	initializeAnalysis(Registry);

	// Build up all of the passes that we want to do to the module.
	PassManager InitialPasses;
	PassManager AnalysisPasses;

	if (optimizeBC()) {
		// may degrade precision of the analysis
		PassManagerBuilder Builder; 
		Builder.OptLevel = 3; 
		Builder.populateModulePassManager(InitialPasses);
	}

	FunctionPass *LoopInfoPass = new LoopInfo();

	InitialPasses.add(createGCLoweringPass());
	
	// this pass converts SwitchInst instructions into a sequence of
	// binary branch instructions, easier to deal with
	InitialPasses.add(createLowerSwitchPass());	
	InitialPasses.add(createLowerInvokePass());
	//Passes.add(createLoopSimplifyPass());	
	InitialPasses.add(LoopInfoPass);

	// in case we want to run an Alias analysis pass : 
	//Passes.add(createGlobalsModRefPass());
	//Passes.add(createBasicAliasAnalysisPass());
	//Passes.add(createScalarEvolutionAliasAnalysisPass());
	//Passes.add(createTypeBasedAliasAnalysisPass());
	//
	TagInline * taginlinepass = new TagInline();
	if (inline_functions()) {
		InitialPasses.add(taginlinepass); // this pass has to be run before the internalizepass, since it builds the list of functions to analyze
		InitialPasses.add(llvm::createAlwaysInlinerPass());
	}
		
	if (check_overflow()) InitialPasses.add(new instrOverflow());

	// make sure everything is run before AI analysis
	InitialPasses.run(*M);
	if (inline_functions()) {
		PassManager InlinePasses;
		InlinePasses.add(createInternalizePass(TagInline::GetFunctionsToAnalyze()));
		InlinePasses.add(createGlobalDCEPass());
		InlinePasses.add(createGlobalOptimizerPass());
		InlinePasses.run(*M);
	}
	
	PassManager OptPasses;
	OptPasses.add(new GlobalToLocal());
	OptPasses.add(new RemoveUndet());
	OptPasses.add(createPromoteMemoryToRegisterPass());
	OptPasses.run(*M);

	if (dumpll()) {
		*Out << *M;
		return;
	}
	if (onlyOutputsRho()) {
		AnalysisPasses.add(new GenerateSMT());
	} else if (compareTechniques()) {
		AnalysisPasses.add(new Compare(getComparedTechniques()));
	} else if (compareNarrowing()) {
		switch (getTechnique()) {
			case LOOKAHEAD_WIDENING:
				AnalysisPasses.add(new CompareNarrowing<LOOKAHEAD_WIDENING>());
				break;
			case GUIDED:
				AnalysisPasses.add(new CompareNarrowing<GUIDED>());
				break;
			case PATH_FOCUSING:
				AnalysisPasses.add(new CompareNarrowing<PATH_FOCUSING>());
				break;
			case PATH_FOCUSING_INCR:
				AnalysisPasses.add(new CompareNarrowing<PATH_FOCUSING_INCR>());
				break;
			case LW_WITH_PF:
				AnalysisPasses.add(new CompareNarrowing<LW_WITH_PF>());
				break;
			case COMBINED_INCR:
				AnalysisPasses.add(new CompareNarrowing<COMBINED_INCR>());
				break;
			case SIMPLE:
				AnalysisPasses.add(new CompareNarrowing<SIMPLE>());
				break;
			case LW_WITH_PF_DISJ:
				AnalysisPasses.add(new CompareNarrowing<LW_WITH_PF_DISJ>());
				break;
		}
	} else if (compareDomain()) {
		switch (getTechnique()) {
			case LOOKAHEAD_WIDENING:
				AnalysisPasses.add(new CompareDomain<LOOKAHEAD_WIDENING>());
				break;
			case GUIDED:
				AnalysisPasses.add(new CompareNarrowing<GUIDED>());
				break;
			case PATH_FOCUSING:
				AnalysisPasses.add(new CompareDomain<PATH_FOCUSING>());
				break;
			case PATH_FOCUSING_INCR:
				AnalysisPasses.add(new CompareDomain<PATH_FOCUSING_INCR>());
				break;
			case LW_WITH_PF:
				AnalysisPasses.add(new CompareDomain<LW_WITH_PF>());
				break;
			case COMBINED_INCR:
				AnalysisPasses.add(new CompareDomain<COMBINED_INCR>());
				break;
			case SIMPLE:
				AnalysisPasses.add(new CompareDomain<SIMPLE>());
				break;
			case LW_WITH_PF_DISJ:
				AnalysisPasses.add(new CompareDomain<LW_WITH_PF_DISJ>());
				break;
		}
	} else { 
		ModulePass *AIPass;
		switch (getTechnique()) {
			case LOOKAHEAD_WIDENING:
				AIPass = new ModulePassWrapper<AIGopan, 0>();
				break;
			case GUIDED:
				AIPass = new ModulePassWrapper<AIGuided, 0>();
				break;
			case PATH_FOCUSING:
				AIPass = new ModulePassWrapper<AIpf, 0>();
				break;
			case PATH_FOCUSING_INCR:
				AIPass = new ModulePassWrapper<AIpf_incr, 0>();
				break;
			case LW_WITH_PF:
				AIPass = new ModulePassWrapper<AIopt, 0>();
				break;
			case COMBINED_INCR:
				AIPass = new ModulePassWrapper<AIopt_incr, 0>();
				break;
			case SIMPLE:
				AIPass = new ModulePassWrapper<AIClassic, 0>();
				break;
			case LW_WITH_PF_DISJ:
				AIPass = new ModulePassWrapper<AIdis, 0>();
				break;
		}
		AnalysisPasses.add(AIPass);
	}
	AnalysisPasses.run(*M);

	//*Out << *M;
	std::string error;

	if (generateMetadata()) {
		raw_fd_ostream * BitcodeOutput = new raw_fd_ostream(getAnnotatedBCFilename().c_str(), error);
		WriteBitcodeToFile(M, *BitcodeOutput);
		BitcodeOutput->close();
	}

	if (onlyOutputsRho()) {
		return;
	}
	// we properly delete all the created Nodes
	std::map<BasicBlock*,Node*>::iterator it = Nodes.begin(), et = Nodes.end();
	for (;it != et; it++) {
		delete (*it).second;
	}

	Pr::releaseMemory();
	SMTpass::releaseMemory();
	ReleaseTimingData();
	Expr::clear_exprs();

	if (OutputFilename != "") {
		delete Out;
	}
}

