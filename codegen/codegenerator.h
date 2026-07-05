#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "llvm/IR/IRBuilder.h"
#include <llvm/IR/Module.h>
#include "llvm/IR/LLVMContext.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>

#include "AbstractSintaxTree.h"

class CodeGenerator
{
public:
    CodeGenerator();
    ~CodeGenerator() = default;

    void generate(const Program& program);

private:
    llvm::LLVMContext Context;                              // Contesto llvm incluso dalle funzioni
    llvm::IRBuilder<> Builder;                              // Genera l'IR llvm
    std::unique_ptr<llvm::Module> Module;                   // Contenitore del codice generato
    std::map<std::string, llvm::AllocaInst*> symbolTable;

    llvm::Value* generateExpr(const Expr* expr);
    void generateStmt(const Stmt* stmt);
};

#endif // CODEGENERATOR_H
