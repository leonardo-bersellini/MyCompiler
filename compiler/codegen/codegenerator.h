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
    PrimitiveType type;
};

class CodeGenerator
{
public:
    CodeGenerator();
    ~CodeGenerator() = default;

    void generate(const Program& program);

    void emitIR();      //ir code
    void buildTargetObj(const std::string &target_path, bool debug = false); //creazione target obj
    bool link(const std::string &objFile, const std::string &outputExe, bool debug = false); //linking exe from obj

private:
    llvm::LLVMContext Context;                              // Contesto llvm incluso dalle funzioni
    llvm::IRBuilder<> Builder;                              // Genera l'IR llvm
    std::unique_ptr<llvm::Module> Module;                   // Contenitore del codice generato

    // stack di scopes per mantenere lo shadowing dei valori allocati
    std::vector<std::unordered_map<std::string, llvm::AllocaInst*>> allocaScopeStack;

    void declareSymbol(const std::string& name, llvm::AllocaInst* alloca);
    llvm::AllocaInst* lookupSymbol(const std::string& name);

    void pushScope();
    void popScope();

    bool isGlobalScope() const;

    llvm::Type* getLLVMType(const Type &type);
    PrimitiveType getPrimitiveType(llvm::Type *type);
    llvm::Value* castValue(llvm::Value* value, PrimitiveType from, PrimitiveType to);

    void copyArrayElements(llvm::Value* source, llvm::Value* destination, llvm::ArrayType* arrType, llvm::Type* elementType);
    void generateArrayAssignment(const LiteralArrayExpr* arrLit, llvm::Value* destination);
    

    void generateStmt(const Stmt* stmt);

    void generateScopeStmt(const BlockStmt* st);
    void generateAssignStmt(const AssignmentStmt* st);
    void generateDeclarationStmt(const DeclarationStmt* st);
    void generateFunctionStmt(const FunctionStmt* st);
    void generateReturnStmt(const ReturnStmt* st);
    void generateIfStmt(const IfStmt* st);
    void generateForStmt(const ForStmt* st);
    void generateWhileStmt(const WhileStmt* st);
    void generateSwitchStmt(const SwitchStmt* st);

    std::vector<llvm::ConstantInt*> collectCaseLabels(const CaseStmt* c, const CaseStmt*& leaf);
    llvm::ConstantInt* generateConstantLabel(const Expr* label);

    ExprGenResult generateExpr(const Expr* expr);

    ExprGenResult generateBinaryExpr(const BinaryExpr* s);
    ExprGenResult generateUnaryExpr(const UnaryExpr* expr);


    //llvm::AllocaInst* createEntryAlloca(llvm::Function* func, const std::string& name, llvm::Type* type);
    //llvm::Value* promote(llvm::Value*, ValueType from, ValueType to);

};

#endif // CODEGENERATOR_H
