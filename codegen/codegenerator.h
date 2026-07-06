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
    QMap<QString, llvm::AllocaInst*> symbolTable;

    void generateStmt(const Stmt* stmt);
    llvm::Value* generateExpr(const Expr* expr);

    llvm::Type* getLLVMType(const ValueType &type);
    llvm::AllocaInst* createEntryAlloca(llvm::Function* func, const std::string& name, llvm::Type* type);

    llvm::Value* promote(llvm::Value*, ValueType from, ValueType to);

};

#endif // CODEGENERATOR_H
