#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "llvm/IR/IRBuilder.h"
#include <llvm/IR/Module.h>
#include "llvm/IR/LLVMContext.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>

//build target
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>

#include "AbstractSintaxTree.h"

// struttura di ritorno della generazione delle espressioni
struct ExprGenResult {
    llvm::Value* llvm_value;
    ValueType type;
};

class CodeGenerator
{
public:
    CodeGenerator();
    ~CodeGenerator() = default;

    void generate(const Program& program);

    void emitIR();      //ir code
    void buildTargetObj(const QString &target_path); //creazione target/exe

private:
    llvm::LLVMContext Context;                              // Contesto llvm incluso dalle funzioni
    llvm::IRBuilder<> Builder;                              // Genera l'IR llvm
    std::unique_ptr<llvm::Module> Module;                   // Contenitore del codice generato
    QMap<QString, llvm::AllocaInst*> symbolTable;

    llvm::Type* getLLVMType(const ValueType &type);
    ValueType getValueType(llvm::Type *type);
    llvm::Value* castValue(llvm::Value* value, ValueType from, ValueType to);

    void generateStmt(const Stmt* stmt);
    ExprGenResult generateExpr(const Expr* expr);

    void generateScopeStmt(const BlockStmt* st);
    void generateAssignStmt(const AssignmentStmt* st);
    void generateDeclarationStmt(const DeclarationStmt* st);
    void generateFunctionStmt(const FunctionStmt* st);
    void generateReturnStmt(const ReturnStmt* st);
    void generateIfStmt(const IfStmt* st);
    void generateForStmt(const ForStmt* st);
    void generateWhileStmt(const WhileStmt* st);

    ExprGenResult generateBinaryExpr(const BinaryExpr* s);
    ExprGenResult generateUnaryExpr(const UnaryExpr* expr);


    //llvm::AllocaInst* createEntryAlloca(llvm::Function* func, const std::string& name, llvm::Type* type);
    //llvm::Value* promote(llvm::Value*, ValueType from, ValueType to);

};

#endif // CODEGENERATOR_H
