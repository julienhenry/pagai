#include "llvm/Support/raw_ostream.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/CodeGen/LinkAllAsmWriterComponents.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/CodeGen/ObjectCodeEmitter.h"
#include "llvm/Config/config.h"
#include "llvm/LinkAllVMCore.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/System/Host.h"
#include "llvm/System/Signals.h"
#include "llvm/Target/SubtargetFeature.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegistry.h"
#include "llvm/Target/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"

//#include "SMTBackend.h"
#include "execute.h"

using namespace llvm;

void execute::exec(std::string InputFilename, std::string OutputFilename) {
	
   Module *M = NULL;
	
   LLVMContext & Context = getGlobalContext();

   std::string ErrorMessage;

   if (MemoryBuffer * Buffer
       = MemoryBuffer::getFileOrSTDIN(InputFilename, &ErrorMessage)) {
       M = ParseBitcodeFile(Buffer, Context, &ErrorMessage);
       delete Buffer;
   } else {
	   std::cout << "Not able to initialize module from bitcode\n";
	   // ERROR("Not able to initialize module from bitcode\n");
   }

   if (OutputFilename != "-") {

       std::string error;
       raw_fd_ostream *FDOut = new raw_fd_ostream(OutputFilename.c_str(), true, true, error);
       if (!error.empty()) {
           errs() << error << '\n';
           delete FDOut;
           return;
       }
       Out = new formatted_raw_ostream(*FDOut,    formatted_raw_ostream::DELETE_STREAM);

       // Make sure that the Output file gets unlinked from the disk if we get a
       // SIGINT
       sys::RemoveFileOnSignal(sys::Path(OutputFilename));
   }

   // Build up all of the passes that we want to do to the module.
   PassManager Passes;

   //ModulePass *MB = new SMTBackend(*Out);

   Passes.add(new TargetData(M));
   Passes.add(createVerifierPass());
   Passes.add(createGCLoweringPass());
   Passes.add(createLowerAllocationsPass(true));
   Passes.add(createLowerInvokePass());
   Passes.add(createCFGSimplificationPass());    // clean up after lower invoke.
   //Passes.add(MB);
   Passes.add(createGCInfoDeleter());

   Passes.run(*M);

   Out->flush();

}
