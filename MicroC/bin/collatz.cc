#include "llvm/ADT/APInt.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

using namespace llvm;

// Create the Collatz function and insert it into module M.
static Function *CreateCollatzFunction(Module *M, LLVMContext &Context) {

  // Return int64_t, take int64_t.
  FunctionType *SumTy = FunctionType::get(Type::getInt64Ty(Context), {Type::getInt64Ty(Context)}, false);

  Function *CollatzFn = Function::Create(SumTy, Function::ExternalLinkage, "collatz", M);

  // Add a basic block to the function.
  BasicBlock *EntryBB = BasicBlock::Create(Context, "EntryBlock", CollatzFn);

  // Get pointers to constants used.
  Value *Zero = ConstantInt::get(Type::getInt64Ty(Context), 0);
  Value *One = ConstantInt::get(Type::getInt64Ty(Context), 1);
  Value *Two = ConstantInt::get(Type::getInt64Ty(Context), 2);
  Value *Three = ConstantInt::get(Type::getInt64Ty(Context), 3);

  Argument *ArgX = &*CollatzFn->arg_begin(); // Get the arg.
  ArgX->setName("Arg");                      // For display

  // Create the base case block
  BasicBlock *RetBB = BasicBlock::Create(Context, "ReturnBlock", CollatzFn);

  // Create the recursive case block
  BasicBlock *RecurseBB = BasicBlock::Create(Context, "RecurseBlock", CollatzFn);

  // Case switch
  Value *CaseInst = new ICmpInst(EntryBB, ICmpInst::ICMP_EQ, ArgX, One, "Case");
  BranchInst::Create(RetBB, RecurseBB, CaseInst, EntryBB);

  // Base case
  ReturnInst::Create(Context, Zero, RetBB);

  // Recursive case
  BasicBlock *RecurseEvenBB = BasicBlock::Create(Context, "EvenBlock", CollatzFn);
  BasicBlock *RecurseOddBB = BasicBlock::Create(Context, "OddBlock", CollatzFn);

  Value *IsOddP = BinaryOperator::CreateAnd(ArgX, One, "IsOddP", RecurseBB);

  // Path switch
  Value *PathInst = new ICmpInst(RecurseBB, ICmpInst::ICMP_EQ, IsOddP, Zero, "Path");
  BranchInst::Create(RecurseEvenBB, RecurseOddBB, PathInst, RecurseBB);

  // Even path
  Value *EvenArg = BinaryOperator::CreateUDiv(ArgX, Two, "EvenDiv", RecurseEvenBB);
  CallInst *CallEvenPath = CallInst::Create(CollatzFn, EvenArg, "EvenPath", RecurseEvenBB);
  // CallEvenPath->setTailCall();
  Value *EvenVal = BinaryOperator::CreateAdd(CallEvenPath, One, "EvenVal", RecurseEvenBB);
  ReturnInst::Create(Context, EvenVal, RecurseEvenBB);

  // Odd path
  Value *OddMul = BinaryOperator::CreateMul(ArgX, Three, "OddMul", RecurseOddBB);
  Value *OddAdd = BinaryOperator::CreateAdd(OddMul, One, "OddAdd", RecurseOddBB);
  CallInst *CallOddPath = CallInst::Create(CollatzFn, OddAdd, "OddPath", RecurseOddBB);

  Value *OddVal = BinaryOperator::CreateAdd(CallOddPath, One, "OddVal", RecurseOddBB);
  ReturnInst::Create(Context, OddVal, RecurseOddBB);

  return CollatzFn;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("collatz: missing argument\n");
    exit(-1);
  }

  int64_t n = atol(argv[1]);

  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  LLVMContext Context;

  // Create some module to put our function into it.
  std::unique_ptr<Module> CollatzOwner(new Module("CollatzM", Context));
  Module *CollatzM = CollatzOwner.get();

  // Create the function
  Function *CollatzF = CreateCollatzFunction(CollatzM, Context);

  // An engine for JIT
  std::string errStr;
  ExecutionEngine *EE =
      EngineBuilder(std::move(CollatzOwner))
          .setErrorStr(&errStr)
          .create();

  if (!EE) {
    errs() << argv[0] << ": Failed to construct ExecutionEngine: " << errStr << "\n";
    return 1;
  }

  errs() << "Verifying... ";
  if (verifyModule(*CollatzM, &outs())) {
    errs() << argv[0] << ": Error constructing function!";
    return 1;
  }

  errs() << "OK\n";
  errs() << "The Collatz LLVM module:\n\n";
  errs() << "---------\n"
         << *CollatzM;
  errs() << "---------\n";
  errs() << "\n";

  errs() << "Obtaining function pointer... ";
  auto fn_ptr = EE->getFunctionAddress("collatz");
  if (!fn_ptr) {
    printf("Count not find function");
    exit(-1);
  }
  errs() << "OK\n";

  errs() << "\n";

  errs() << "Calling Collatz(" << n << ") with JIT...\n";

  int64_t (*fn)(int64_t) = (int64_t (*)(int64_t))fn_ptr;

  int64_t GV = fn(n);

  // import result of execution
  outs() << "Result: " << GV << "\n";

  return 0;
}
