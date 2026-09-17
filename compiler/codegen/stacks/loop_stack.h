#ifndef LOOP_STACK_H
#define LOOP_STACK_H

#include <vector>

#include <llvm/IR/BasicBlock.h>

struct LoopContext 
{
    LoopContext() = delete;
    LoopContext(llvm::BasicBlock* break_, llvm::BasicBlock* continue_) 
        : breakTarget(break_), continueTarget(continue_) {}

    llvm::BasicBlock* breakTarget;
    llvm::BasicBlock* continueTarget;
};

using LoopStack = std::vector<LoopContext>;

#endif //LOOP_STACK_H