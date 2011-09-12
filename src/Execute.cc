#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/LinkAllVMCore.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/system_error.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_os_ostream.h"

#include "llvm/Target/TargetData.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/Transforms/Scalar.h"

#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/LoopInfo.h"

#include "AI.h"
#include "AIopt.h"
#include "AIGopan.h"
#include "AIClassic.h"
#include "Node.h"
#include "Execute.h"
#include "Live.h"
#include "SMT.h"
#include "Compare.h"
#include "Analyzer.h"
#include "Expr.h"
#include "GenerateSMT.h"

using namespace llvm;

static cl::opt<std::string>
DefaultDataLayout("default-data-layout", 
          cl::desc("data layout string to use if not specified by module"),
          cl::value_desc("layout-string"), cl::init(""));


void execute::exec(std::string InputFilename, std::string OutputFilename) {

	//Module *M = NULL;
	raw_fd_ostream *FDOut = NULL;

	LLVMContext & Context = getGlobalContext();

	////
	
	std::string ErrorMessage;
	std::auto_ptr<Module> M;
	{
		OwningPtr<MemoryBuffer> BufferPtr;
		if (error_code ec = MemoryBuffer::getFileOrSTDIN(InputFilename, BufferPtr))
			ErrorMessage = ec.message();
		else
			M.reset(ParseBitcodeFile(BufferPtr.get(), Context, &ErrorMessage));
	}
	if (M.get() == 0) {
		errs() << ": ";
		if (ErrorMessage.size())
			errs() << ErrorMessage << "\n";
		else
			errs() << "bitcode didn't read correctly.\n";
		return;
	}

	TargetData * TD = 0;
	const std::string &ModuleDataLayout = M.get()->getDataLayout();
	if (!ModuleDataLayout.empty()) {
		TD = new TargetData(ModuleDataLayout);
	} else if (!DefaultDataLayout.empty()) {
		TD = new TargetData(DefaultDataLayout);
	}

	////

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
		sys::RemoveFileOnSignal(sys::Path(OutputFilename));
	} else {
		Out = &llvm::outs();
		Out->SetUnbuffered();
	}

	// Build up all of the passes that we want to do to the module.
	PassManager Passes;

	ModulePass *AIPass;
	if (onlyOutputsRho()) {
		AIPass = new GenerateSMT();
	} else { 
		switch (getTechnique()) {
			case LOOKAHEAD_WIDENING:
				AIPass = new AIGopan();
				break;
			case PATH_FOCUSING:
				AIPass = new AI();
				break;
			case LW_WITH_PF:
				AIPass = new AIopt();
				break;
			case SIMPLE:
				AIPass = new AIClassic();
				break;
		}
	}


	FunctionPass *LoopInfoPass = new LoopInfo();

	Passes.add(TD);
	Passes.add(createVerifierPass());
	Passes.add(createGCLoweringPass());
	//Passes.add(createLowerInvokePass());
	Passes.add(createPromoteMemoryToRegisterPass());
	Passes.add(createLoopSimplifyPass());	
	Passes.add(LoopInfoPass);

	// this pass converts SwitchInst instructions into a sequence of
	// chained binary branch instructions, much easier to deal with
	Passes.add(createLowerSwitchPass());	

	Passes.add(new Live());
	Passes.add(new SMT());
	Passes.add(AIPass);
	if (compareTechniques()) {
		Passes.add(new AI());
		Passes.add(new AIGopan());
		Passes.add(new AIClassic());
		Passes.add(new Compare());
	}

	Passes.run(*M.get());

	// we properly delete all the created Nodes
	std::map<BasicBlock*,Node*>::iterator it = Nodes.begin(), et = Nodes.end();
	for (;it != et; it++) {
		delete (*it).second;
	}

	//Out->flush();
	//delete FDOut;
	//delete Out;
	//delete AIPass;
	//delete LoopInfoPass;
}

