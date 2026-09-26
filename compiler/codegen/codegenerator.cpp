#include "codegenerator.h"

#include <llvm/IR/Verifier.h>
#include <iostream>
#include <variant>

#include "linker/linker.h"

#include "utils/ansi/ansi.h"
namespace clr = ansi::color;

/**
 * CODE GENERATION
 * Il codice viene generato utilizzando il framework llvm.
 * La classe CodeGenerator (this) funge da visitor dell'ast prodotto, che viene convertito
 * in codice llvm ir e llvm.
 *
 * Il codice passato al generatore non deve contenere error stmt, per una questione pratica e logica.
 * Per struttura stessa della codegen, infatti, il codice non può essere creato con errori; da questo
 * deriva il fatto che il generatore non prevede azioni per errorStmt o simili, eccetto controlli
 * di sicurezza.
 */

CodeGenerator::CodeGenerator()
    : Builder(Context)
{
    Module = std::make_unique<llvm::Module>("mycompiler_module", Context);
}

/*
 * Questa funzione si occupa di emettere, ovvero stampare in output, gli statement llvm ir
 * accumulati nel modulo, ovvero il programma convertito in llvm ir code.
 */

void CodeGenerator::emitIR()
{
    std::cout << "\nIR [llvm-generated]:" << std::endl;
    std::cout << "> module status: " << llvm::verifyModule(*Module, &llvm::errs()) << std::endl;
    
    std::cout << ansi::color::bright_black;
    Module->print(llvm::outs(), nullptr);
    std::cout << ansi::color::reset;
}

/*
 * Questa funzione si occupa di costruire un target e compilare il vero codice obj,
 * partendo dal codice IR di llvm.
 * Le funzioni utilizzate sono chiamatedi basso livello llvm che si occupano di inizializzare e
 * richiamare correttamente il sistea operativo per cui si genera il codice.
 * Questo codice llvm verrà compilato per un'architettura Windows.
 */

void CodeGenerator::buildTargetObj(const std::string& target_path, bool debug)
{
    //INIT TARGET
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    //CREA TARGETMACHINE
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();

    if (llvm::verifyModule(*Module, &llvm::errs())) {
        llvm::errs() << (clr::red + "Modulo LLVM invalido\n" + clr::reset);
        return;
    }
    Module->setTargetTriple(TargetTriple);

    std::string Error;

    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    if (!Target) {
        llvm::errs() << (clr::red + Error + clr::reset);
        return;
    }

    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>();

    auto TargetMachine = Target->createTargetMachine(TargetTriple, "x86-64", "", opt, RM);
    if (!TargetMachine) {
        llvm::errs() << (clr::red + "Impossibile creare TargetMachine\n" + clr::reset);
        return;
    }

    Module->setDataLayout(TargetMachine->createDataLayout());

    //CREAZIONE FILE OBJ
    std::error_code EC;

    llvm::raw_fd_ostream dest(target_path, EC, llvm::sys::fs::OF_None);
    if (EC) {
        std::cout << clr::red;
        std::cout << "Errore apertura file:" << EC.message() << std::endl;
        std::cout << clr::reset;
        return;
    }
    llvm::legacy::PassManager pass;

    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile))
    {
        llvm::errs() << (clr::red + "Cannot emit object file\n" + clr::reset);
        return;
    }

    pass.run(*Module.get());
    dest.flush();

    if(!debug) return;

    std::cout << "\nDebug info:";

    std::cout << clr::bright_black << std::endl;
    std::cout << "Target triple:" << TargetTriple << std::endl;
    std::cout << "Numero funzioni nel modulo:" << Module->size() << std::endl;
    std::cout << "dest.has_error():" << dest.has_error() << std::endl;

    std::cout << clr::reset << std::endl;

    return;
}

/*
 * Questa funzione si interfaccia con il vero codice di linking, in modo da isolare la
 * logica del linker e le inclusioni di windows.
 */

bool CodeGenerator::link(const std::string &objFile, const std::string &outputExe, bool debug)
{
    return linker::lld_link(objFile, outputExe, debug);
}

/// --- UTILITIES --- ///

/*
 * Funzione di utility, permette di convertire i tipi dell'ast in tipi llvm.
 */

llvm::Type* CodeGenerator::getLLVMType(const Type &type)
{
    return std::visit(TypeVisitor{
        [this](const PrimitiveType& t) -> llvm::Type* {
            switch(t) {
                case PrimitiveType::Int : return llvm::Type::getInt32Ty(Context);
                    break;
                case PrimitiveType::Double : return llvm::Type::getDoubleTy(Context);
                    break;
                case PrimitiveType::Bool : return llvm::Type::getInt1Ty(Context);
                    break;
                case PrimitiveType::Char : return llvm::Type::getInt8Ty(Context);
                    break;
                case PrimitiveType::Void : return llvm::Type::getVoidTy(Context);
                    break;
                default : return nullptr;
                    break;
            }
        },
        [this](const ArrayType& a) -> llvm::Type* {
            return llvm::ArrayType::get(getLLVMType(Type(a.elementType)), a.size);
        },
    }, type.category);
}

/*
 * Inversa di getLLVMType, permette di convertire i tipi di llvm in tipi
 * dell'enumerazione standard del nostro ast.
 */

Type CodeGenerator::getType(llvm::Type *type)
{
    if (type->isIntegerTy(1))
        return Type(PrimitiveType::Bool);

    if (type->isIntegerTy(8))
        return Type(PrimitiveType::Char);

    if (type->isIntegerTy(32))
        return Type(PrimitiveType::Int);

    if (type->isDoubleTy())
        return Type(PrimitiveType::Double);

    if (type->isVoidTy())
        return Type(PrimitiveType::Void);

    if(type->isArrayTy()) {
        PrimitiveType elementTy = getType(type->getArrayElementType()).asPrimitive();

        //dall'elemento singolo si deduce il tipo dell'array
        switch (elementTy) {
            case PrimitiveType::Int:
            case PrimitiveType::Double:
            case PrimitiveType::Char:
            case PrimitiveType::Bool:
                return Type{ArrayType(elementTy, type->getArrayNumElements())};

            default: return Type(PrimitiveType::Error);
        }
    }

    return Type(PrimitiveType::Error);
}

/*
 * Questa funzione restituisce un valore llvm di default per ogni tipo richiesto.
 * I valori di default sono utilizzati per inizializzare variabili globali non inizializzate 
 * nel codice ricevuto.
 */

llvm::Constant* CodeGenerator::getDefaultValue(const Type& type) 
{
    llvm::Type* llvmType = getLLVMType(type);
    return llvm::Constant::getNullValue(llvmType);
} 

/*
 * Questa funzione permette di risalire al tipo allocato in una variabile letta tramite lookup,
 * distinguendo correttamente tra llvm Alloca e llvm Global.
 */

llvm::Type* CodeGenerator::getAllocatedType(llvm::Value* ptr)
{
    if (auto* alloc = llvm::dyn_cast<llvm::AllocaInst>(ptr))
        return alloc->getAllocatedType();
    if (auto* glob = llvm::dyn_cast<llvm::GlobalVariable>(ptr))
        return glob->getValueType();

    throw std::runtime_error("codegen internal error: unexpected pointer kind in getAllocatedType");
}

/*
 * Funzione che applica le conversioni implicite di tipo ai valori.
 * Questa funzione applica in automatico le conversioni fisiche di valore tramite api llvm,
 * ai values (llvm::Value*) di llvm.
 *
 * Nota: E' di estrema importanza che rispecchi le regole di tipo condivise in types.h, in quanto
 * questa funzione non legge le regole, ma le applica soltanto.
 */

llvm::Value* CodeGenerator::castValue(llvm::Value *value, PrimitiveType from, PrimitiveType to)
{
    if (from == to) return value;

    switch (from)
    {
    case PrimitiveType::Int:

        if (to == PrimitiveType::Double)
            return Builder.CreateSIToFP(value, getLLVMType(Type(PrimitiveType::Double)), "sitofp");

        break;

    case PrimitiveType::Double:
        //non ancora utilizzato in types.h, non valido
        if (to == PrimitiveType::Int)
            return Builder.CreateFPToSI(value, getLLVMType(Type(PrimitiveType::Int)), "fptosi");

        break;

    default:
        break;
    }

    // Cast non supportato
    return value;
}

/*
 * Funzione di utility della codegeneration, permette di risalire all'indirizzo di alloca
 * di una variabile (considerata lvalue).
 * Risponde alla necessità di risalire all'indirizzo delle variabili su cui salvare un valore.
 */

llvm::Value* CodeGenerator::generateLValueAddress(const Expr* target)
{
    if(auto varExpr = dynamic_cast<const VariableExpr*>(target)) {
        return scopeStack.lookupSymbol(varExpr->name).value(); // ritorna direttamente l'AllocaInst*
    }

    if(auto arrAccess = dynamic_cast<const ArrayAccessExpr*>(target)) 
    {
        // indirizzo dell'array
        auto arr = generateLValueAddress(arrAccess->base.get());
        // valore scalare di index
        auto index = generateExpr(arrAccess->index.get()).llvm_value;

        auto type = getAllocatedType(arr);

        std::vector<llvm::Value*> idx = {
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 0),
            index
        };

        return Builder.CreateGEP(type, arr, idx, "arr.elem.addr");
    }

    throw std::runtime_error("codegen internal error: unsupported lvalue expression");
}

/*
 * Funzione di utility per array, permette di racchiudere la logica di copia degli elementi da 
 * un array ad un altro.
 */

void CodeGenerator::copyArrayElements(llvm::Value* source, llvm::Value* destination, llvm::ArrayType* arrType, llvm::Type* elementType)
{
    for(unsigned i = 0; i < arrType->getNumElements(); ++i) {
        std::vector<llvm::Value*> idx = {
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 0),
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), i)
        };

        llvm::Value* srcPtr = Builder.CreateGEP(arrType, source, idx, "src.elem");
        llvm::Value* destPtr = Builder.CreateGEP(arrType, destination, idx, "dest.elem");

        llvm::Value* elementVal = Builder.CreateLoad(elementType, srcPtr);
        Builder.CreateStore(elementVal, destPtr);
    }
}

/*
 * Funzione di utility, racchiude la logica di assegnazione di un array, riutilizza
 * internamente la copia degli elementi.
 */

void CodeGenerator::generateArrayAssignment(const LiteralArrayExpr* arrLit, llvm::Value* destination)
{
    auto srcVal = generateExpr(arrLit); // puntatore all'array temporaneo del literal
    llvm::ArrayType* arrType = llvm::cast<llvm::ArrayType>(getLLVMType(Type(arrLit->type)));
    llvm::Type* elementType = arrType->getElementType();

    copyArrayElements(srcVal.llvm_value, destination, arrType, elementType);
}

/// --- ENTRY POINT, CODEGEN --- ///

/*
 * : Entry Point :
 * Funzione "entry point" della generazione del codice macchina.
 * Attiva la codegen a partire da un istanza di AST.
 * Come per le altre strutture, richiama ogni stmt in modo ricorsivo.
 */

void CodeGenerator::generate(const Program &program)
{
    scopeStack.push(); //scope globale

    for(const auto& st : program.statements)
    {
        generateStmt(st.get());
    }

    scopeStack.pop();
}

void CodeGenerator::assignNamespaceTable(NamespaceTable& namespaceTable)
{
    this->namespaceTable = &namespaceTable;
}

/*
 * Funzione di codegen per ogni stmt del programma.
 * Il codice di generazione di ogni stmt è racchiuso in funzioni helper per chiarezza e
 * pulizia del codice.
 */

void CodeGenerator::generateStmt(const Stmt *stmt)
{
    // Dichiarazione
    if(auto s = dynamic_cast<const DeclarationStmt*>(stmt))
    {
        generateDeclarationStmt(s); 
    }

    // Espressione
    else if(auto s = dynamic_cast<const ExpressionStmt*>(stmt))
    {
        generateExpr(s->expr.get()); 
    }

    // Scopes
    else if(auto s = dynamic_cast<const BlockStmt*>(stmt))
    {
        generateScopeStmt(s);
    }

    // Funzioni
    else if(auto s = dynamic_cast<const FunctionStmt*>(stmt))
    {
        generateFunctionStmt(s);
    }

    // If Stmt
    else if(auto s = dynamic_cast<const IfStmt*>(stmt))
    {
        generateIfStmt(s);
    }

    // While Stmt
    else if(auto s = dynamic_cast<const WhileStmt*>(stmt))
    {
        generateWhileStmt(s);
    }

    // For Stmt
    else if(auto s = dynamic_cast<const ForStmt*>(stmt))
    {
        generateForStmt(s);
    }

    // Return Stmt
    else if(auto s = dynamic_cast<const ReturnStmt*>(stmt))
    {
        generateReturnStmt(s);
    }

    // Switch Stmt
    else if(auto s = dynamic_cast<const SwitchStmt*>(stmt))
    {
        generateSwitchStmt(s);
    }

    // Namespace Stmt
    else if(auto s = dynamic_cast<const NamespaceStmt*>(stmt))
    {
        generateNamespaceStmt(s);
    }

    // Break Stmt
    else if(auto s = dynamic_cast<const BreakStmt*>(stmt))
    {
        //sposta l'insert point nel blocco break attuale

        llvm::Function* function = Builder.GetInsertBlock()->getParent();
        Builder.CreateBr(loopStack.back().breakTarget);

        llvm::BasicBlock* deadBB = llvm::BasicBlock::Create(Context, "after.break", function);
        Builder.SetInsertPoint(deadBB);
    }

    // Continue Stmt
    else if(auto s = dynamic_cast<const ContinueStmt*>(stmt))
    {
        //sposta l'insert point nel blocco continue attuale

        llvm::Function* function = Builder.GetInsertBlock()->getParent();

        Builder.CreateBr(loopStack.back().continueTarget);

        llvm::BasicBlock* deadBB = llvm::BasicBlock::Create(Context, "after.continue", function);
        Builder.SetInsertPoint(deadBB);
    }

    // Print Stmt
    else if(auto s = dynamic_cast<const PrintStmt*>(stmt))
    {
        generatePrintStmt(s);
    }

    return;
}

/**
 * FUNZIONI DI GENERAZIONE STMT
 * Queste funzioni hanno il solo scopo di racchiudere la logica di generazione di ogni stmt,
 * in modo da esser richiamate dal dispath principale in caso di bisogno.
 * Questi codici utilizzano il builder llvm, che genera stmts in base alle variabili di ogni nodo ast.
 */

void CodeGenerator::generateScopeStmt(const BlockStmt *st)
{
    scopeStack.push();

    for(const auto& st : st->statements) {
        generateStmt(st.get());

        if (Builder.GetInsertBlock()->getTerminator()) {
            break;
        }
    }

    scopeStack.pop();
}

void CodeGenerator::generateDeclarationStmt(const DeclarationStmt *st)
{
    std::string st_name = namespaceTable->mangleName(st->name);

    if(scopeStack.isGlobalScope()) 
    {
        llvm::Constant* constant;

        if(st->initializer)
        {   
            if(auto n = dynamic_cast<const NumberExpr*>(st->initializer.get())) {
                if(n->isInteger) {
                    constant = llvm::ConstantInt::get(getLLVMType(Type(PrimitiveType::Int)), n->value);
                } else {
                    constant = llvm::ConstantFP::get(getLLVMType(Type(PrimitiveType::Double)), n->value);
                }
            }
            else if(auto c = dynamic_cast<const CharExpr*>(st->initializer.get())) {
                constant = llvm::ConstantInt::get(getLLVMType(Type(PrimitiveType::Char)), c->value);
            }
            else if(auto b = dynamic_cast<const BooleanExpr*>(st->initializer.get())) {
                constant = llvm::ConstantInt::get(getLLVMType(Type(PrimitiveType::Bool)), b->value);
            }
            else throw std::runtime_error("internal error: global variable invalid initializer");

        } else {
            constant = getDefaultValue(st->type);
        }

        llvm::GlobalVariable* var = new llvm::GlobalVariable(
            *Module,
            getLLVMType(st->type), 
            st->isConst, 
            llvm::GlobalValue::ExternalLinkage,
            constant, 
            st_name
        );
        scopeStack.declareGlobal(st_name, var);

        return;
    }

    // alloca variabile locale
    auto* alloc = Builder.CreateAlloca(getLLVMType(st->type), nullptr, st_name);
    scopeStack.declareSymbol(st_name, alloc);

    if(st->initializer) 
    {
        if(st->type.isArray()) {
            if(auto arrLiteral = dynamic_cast<const LiteralArrayExpr*>(st->initializer.get())) {
                generateArrayAssignment(arrLiteral, alloc);
            } else {
                //inizializzazione arr1 = arr2
                auto varExpr = dynamic_cast<const VariableExpr*>(st->initializer.get());
                llvm::Value* source = scopeStack.lookupSymbol(varExpr->name).value();
                
                llvm::ArrayType* arrType = llvm::cast<llvm::ArrayType>(getAllocatedType(scopeStack.lookupSymbol(st_name).value()));
                llvm::Type* elementType = arrType->getElementType();

                copyArrayElements(source, alloc, arrType, elementType);
            }
        } else {
            // store del valore in inizializzazione
            auto val = generateExpr(st->initializer.get());
            auto casted = castValue(val.llvm_value, val.type.asPrimitive(), st->type.asPrimitive());
            Builder.CreateStore(casted, alloc);
        }
    }
}

void CodeGenerator::generateFunctionStmt(const FunctionStmt *st)
{
    std::string st_name = namespaceTable->mangleName(st->name);

    std::vector<llvm::Type*> args;
    for(const FunctionParam& p : st->params) {
        args.push_back(getLLVMType(p.type));
    }

    auto *funcType = llvm::FunctionType::get(getLLVMType(st->returnType), args, false);

    auto *function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                            st_name, Module.get());

    auto *entry = llvm::BasicBlock::Create(Context, "entry", function);

    Builder.SetInsertPoint(entry);

    //creazione dello scope della funzione, con dichiarazione dei parametri come variabili
    scopeStack.push();

    int i = 0;
    for(auto& arg : function->args())
    {
        const FunctionParam& p = st->params[i];

        arg.setName(p.name);

        auto* alloc = Builder.CreateAlloca(getLLVMType(p.type), nullptr, p.name);
        Builder.CreateStore(&arg, alloc);

        scopeStack.declareSymbol(p.name, alloc);

        i++;
    }

    generateStmt(st->body.get());

    //le funzioni void possono terminare senza return esplicito
    if(!Builder.GetInsertBlock()->getTerminator()) {
        if(st->returnType.is(PrimitiveType::Void))
            Builder.CreateRetVoid();
    }

    scopeStack.pop();
}

void CodeGenerator::generateReturnStmt(const ReturnStmt *st)
{
    if(st->value) {
        //return con value

        llvm::Value* value = generateExpr(st->value.get()).llvm_value;
        Builder.CreateRet(value);

    } else {
        //return senza value
        Builder.CreateRetVoid();
    }
}

void CodeGenerator::generateIfStmt(const IfStmt *st)
{
    llvm::Value* condition = generateExpr(st->condition.get()).llvm_value;

    llvm::Function *function = Builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* thenBB = nullptr;
    llvm::BasicBlock* elseBB = nullptr;

    if(st->thenBranch) {
        // questo caso è garantito dal parser come sempre valido (!= nullptr)
        thenBB = llvm::BasicBlock::Create(Context, "then", function);
    }

    // prima di creare l'else, creiamo il blocco dove inserirlo
    if(st->elseBranch) {
        elseBB = llvm::BasicBlock::Create(Context, "else", function);
    }

    //merge block, punto di unione delle branch logiche dell'if
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(Context, "ifcont", function);

    //creazione delle branch codition
    llvm::BasicBlock* falseBlock = elseBB ? elseBB : mergeBB;

    Builder.CreateCondBr(condition, thenBB, falseBlock);
    Builder.SetInsertPoint(thenBB);

    //THEN
    if(st->thenBranch) {
        generateStmt(st->thenBranch.get());

        if (!Builder.GetInsertBlock()->getTerminator()) {
            Builder.CreateBr(mergeBB);
        }
    }

    //ELSE
    if(elseBB) {
        Builder.SetInsertPoint(elseBB);

        generateStmt(st->elseBranch.get());

        if (!Builder.GetInsertBlock()->getTerminator()) {
            Builder.CreateBr(mergeBB);
        }
    }

    //CONTINUA DOPO L'IF
    Builder.SetInsertPoint(mergeBB);
}

void CodeGenerator::generateForStmt(const ForStmt *st)
{
    llvm::Function* function = Builder.GetInsertBlock()->getParent();

    scopeStack.push();
    //init
    if (st->init) generateStmt(st->init.get());

    //creazione dei blocchi necessari
    llvm::BasicBlock* condBB =llvm::BasicBlock::Create(Context, "for.cond", function);
    llvm::BasicBlock* bodyBB =llvm::BasicBlock::Create(Context, "for.body", function);
    llvm::BasicBlock* updateBB = llvm::BasicBlock::Create(Context, "for.update", function);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(Context, "for.end", function);

    //popola lo stack dei loopcontext
    loopStack.push_back(LoopContext(afterBB, updateBB));

    //entra nel controllo della condizione
    Builder.CreateBr(condBB);
    Builder.SetInsertPoint(condBB);

    //crea il ciclo come branch logica
    if(st->condition) {
        llvm::Value *condition = generateExpr(st->condition.get()).llvm_value;
        Builder.CreateCondBr(condition, bodyBB, afterBB);
    } else {
        //loop infinito (senza condizione)
        Builder.CreateBr(bodyBB);
    }

    //body
    Builder.SetInsertPoint(bodyBB);
    if(st->body) generateStmt(st->body.get());

    //vai all'update al termine del ciclo
    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateBr(updateBB);
    }

    Builder.SetInsertPoint(updateBB);
    if(st->update) generateExpr(st->update.get());

    // ritorna al controllo della condizione
    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateBr(condBB);
    }

    Builder.SetInsertPoint(afterBB);

    loopStack.pop_back();
    scopeStack.pop();
}

void CodeGenerator::generateWhileStmt(const WhileStmt *st)
{
    llvm::Function* function = Builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(Context, "wcond", function);
    Builder.CreateBr(condBB);

    //sposta il builder dentro condition block e genera la condizione
    Builder.SetInsertPoint(condBB);
    llvm::Value* condition = generateExpr(st->condition.get()).llvm_value;

    //creazione basicblocks, body e after
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(Context, "wbody", function);
    llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(Context, "wend", function);

    loopStack.push_back(LoopContext(afterBB, condBB));

    //creazione condition branch
    Builder.CreateCondBr(condition, bodyBB, afterBB);

    //generazione body
    Builder.SetInsertPoint(bodyBB);
    if(st->body) {
        generateStmt(st->body.get());
    }

    loopStack.pop_back();

    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateBr(condBB);
    }

    //spostamento fuori dal while
    Builder.SetInsertPoint(afterBB);
}

void CodeGenerator::generateSwitchStmt(const SwitchStmt* st)
{
    llvm::Value* scrutinee_val = generateExpr(st->scrutinee.get()).llvm_value;
    llvm::Function* currentFunc = Builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* defaultBB = llvm::BasicBlock::Create(Context, "switch.default", currentFunc);
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(Context, "switch.end", currentFunc);

    //creazione dello switch
    llvm::SwitchInst* switchInst = Builder.CreateSwitch(scrutinee_val, defaultBB);

    //generazione dei case
    for(const auto& c : st->cases)
    {
        const CaseStmt* leaf = nullptr; 
        auto labels = collectCaseLabels(c.get(), leaf);

        llvm::BasicBlock* caseBB = llvm::BasicBlock::Create(Context, "switch.case", currentFunc);

        for(auto* labelVal : labels) {
            switchInst->addCase(labelVal, caseBB);
        }

        //leaf contiene il body effettivo, che può essere condiviso da più labels
        Builder.SetInsertPoint(caseBB);
        //generazione del body
        for(const auto& s : leaf->body) {
            generateStmt(s.get());
        }
        if(!Builder.GetInsertBlock()->getTerminator()) {
            Builder.CreateBr(mergeBB);
        }
    }

    //creazione del default
    Builder.SetInsertPoint(defaultBB);
    if (st->_default) {
        for (const auto& s : st->_default->body) {
            generateStmt(s.get());
        }
    }
    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateBr(mergeBB);
    }

    Builder.SetInsertPoint(mergeBB);

}

/*
 * Funzione di generazione di un namespace.
 */

void CodeGenerator::generateNamespaceStmt(const NamespaceStmt* st)
{
    namespaceTable->push(st->name);

    // ogni stmt interessato gestisce in automatico la propria presenza dentro un namespace
    for(const auto& s : st->body) {
        generateStmt(s.get());
    }

    namespaceTable->pop();
}

/*
 * Funzione di generazione degli stmt di print (call di funzione buit-in).
 * La funzione di print deve convertire in modo automatico ogni tipo di variabile in
 * un array di caratteri.
 * 
 * La funzione di print si adatta anche a tipi non string, richiamando altre funzione runtime
 * di print specifiche, dedotte in base al tipo.
 */

void CodeGenerator::generatePrintStmt(const PrintStmt* st)
{
    ExprGenResult expr = generateExpr(st->content.get());

    llvm::Type* voidTy = getLLVMType(Type(PrimitiveType::Void));
    llvm::Type* exprTy = getLLVMType(expr.type);

    auto printWithConversion = [&](const std::string funcName)
    {
        llvm::FunctionType* funcTy = llvm::FunctionType::get(voidTy, { exprTy }, false);
        llvm::FunctionCallee callee = Module->getOrInsertFunction(funcName, funcTy);

        Builder.CreateCall(callee, { expr.llvm_value });
    };

    auto printString = [&](llvm::Value* str, const size_t len) 
    {
        llvm::Type* charPtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(Context));
        llvm::Type* sizeTy = llvm::Type::getInt64Ty(Context); //la funzione runtime legge size_t (uint64)
        llvm::Value* lenValue = llvm::ConstantInt::get(sizeTy, len);

        //funzione: void bsm_print(char[], size)
        llvm::FunctionType* funcTy = llvm::FunctionType::get(voidTy, {charPtrTy, sizeTy}, false);
        llvm::FunctionCallee callee = Module->getOrInsertFunction("bsm_print", funcTy);

        Builder.CreateCall(callee, {str, lenValue});
    };

    //dispath che richiama o costruisce la corretta funzione runtime di print
    //ritorna un booleano di controllo che specifica se il tipo è stato gestito
    bool handled = std::visit(TypeVisitor
    {
        [&](const PrimitiveType& p) -> bool {
            switch(p) {
                case PrimitiveType::Int:    printWithConversion("bsm_print_int");    return true;
                case PrimitiveType::Double: printWithConversion("bsm_print_double"); return true;
                case PrimitiveType::Bool:   printWithConversion("bsm_print_bool");   return true;
                case PrimitiveType::Char:   printWithConversion("bsm_print_char");   return true;
            }
            return false;
        },
        [&](const ArrayType& a) -> bool {
            switch(a.elementType) {
                case PrimitiveType::Char:   printString(expr.llvm_value, a.size);    return true;
            }
            return false;
        },
    }, expr.type.category);

    if(!handled) {
        throw std::runtime_error("codegen internal error: unsupported type for print. type: " + types::toString(expr.type));
    }

    return;
}

/*
 * Questa funzione di utility si occupa di raccogliere le label costanti di una serie di 
 * case stmts, che condividono lo stesso body annidato.
 * Ritorna anche il case finale (quello che contiene il body comune agli altri) per valore.
 * Il ritorno principale è un vettore di costanti, necessarie per llvm.
 */

std::vector<llvm::ConstantInt*> CodeGenerator::collectCaseLabels(const CaseStmt* c, const CaseStmt*& leaf) 
{
    std::vector<llvm::ConstantInt*> labels;
    labels.push_back(generateConstantLabel(c->label.get()));

    // se il body contiene un solo CaseStmt annidato, scendi ricorsivamente
    if (c->body.size() == 1) {
        if (auto nested = dynamic_cast<const CaseStmt*>(c->body[0].get())) {
            auto nestedLabels = collectCaseLabels(nested, leaf);
            labels.insert(labels.end(), nestedLabels.begin(), nestedLabels.end());
            return labels;
        }
    }

    leaf = c; // questo è il case con il body vero
    return labels;
}

/*
 * Questa utility permette di ottenere un oggetto llvm ConstantInt a partire da una label
 * di un case. Per tanto è costruita sulla struttura di un case label.
 */

llvm::ConstantInt* CodeGenerator::generateConstantLabel(const Expr* label)
{
    if (auto n = dynamic_cast<const NumberExpr*>(label)) {
        return llvm::ConstantInt::get(Context, llvm::APInt(32, static_cast<uint64_t>(n->value), true));
    }
    if (auto c = dynamic_cast<const CharExpr*>(label)) {
        return llvm::ConstantInt::get(Context, llvm::APInt(8, static_cast<uint64_t>(c->value), false));
    }

    // non dovrebbe mai accadere, garantito dall'analisi semantica
    throw std::runtime_error("internal error: non-constant case label reached codegen");
}


/**
 * FUNZIONI DI GENERAZIONE EXPR
 * Questa funzione si occupa del dispatch generale della generazione di ogni nodo expr.
 * La generazione dei nodi più complessi è racchiusa in funzioni helper per chiarezza e pulizia
 * del codice.
 */

ExprGenResult CodeGenerator::generateExpr(const Expr *expr)
{
    // Number Expression
    if(auto s = dynamic_cast<const NumberExpr*>(expr))
    {
        if(s->isInteger)
            return ExprGenResult {
                llvm::ConstantInt::get(getLLVMType(Type(PrimitiveType::Int)), (int)s->value),
                PrimitiveType::Int
            };
        else
            return ExprGenResult {
                llvm::ConstantFP::get(getLLVMType(Type(PrimitiveType::Double)), s->value),
                PrimitiveType::Double
            };
    }

    // Char Expression
    else if(auto s = dynamic_cast<const CharExpr*>(expr))
    {
        return ExprGenResult {
            llvm::ConstantInt::get(getLLVMType(Type(PrimitiveType::Char)), s->value),
            PrimitiveType::Char
        };
    }

    // Boolean Expression
    else if(auto s = dynamic_cast<const BooleanExpr*>(expr))
    {
        return ExprGenResult {
            llvm::ConstantInt::get(llvm::Type::getInt1Ty(Context), s->value),
            PrimitiveType::Bool
        };
    }

    // Assign Expr
    else if(auto s = dynamic_cast<const AssignmentExpr*>(expr))
    {
        auto symbol = generateLValueAddress(s->target.get());
        Type symbolType = getType(getAllocatedType(symbol));

        if(symbolType.isArray()) 
        {
            if(auto arrLit = dynamic_cast<const LiteralArrayExpr*>(s->value.get())) {
                generateArrayAssignment(arrLit, symbol);
            } else {
                // assegnazione di un array ad un altro: arr1 = arr2;
                auto varExpr = dynamic_cast<const VariableExpr*>(s->value.get());
                
                llvm::Value* source = nullptr;
                if (!varExpr->qualifiers.empty()) {
                    std::string mangled = namespaceTable->mangleQualifiedName(varExpr->qualifiers, varExpr->name);
                    source = Module->getGlobalVariable(mangled);
                } else {
                    source = scopeStack.lookupSymbol(varExpr->name).value();
                }

                llvm::ArrayType* arrType = llvm::cast<llvm::ArrayType>(getAllocatedType(symbol));
                llvm::Type* elementType = arrType->getElementType();

                copyArrayElements(source, symbol, arrType, elementType);
            }
            return ExprGenResult{symbol, symbolType};

        } else {
            auto value = generateExpr(s->value.get());
            // conversione dal tipo del valore al tipo della variabile
            auto casted = castValue(value.llvm_value, value.type.asPrimitive(), getType(getAllocatedType(symbol)).asPrimitive());

            Builder.CreateStore(casted, symbol);

            return ExprGenResult{casted, symbolType};
        }
    }

    // Operator-Composed Assignment Expr
    else if(auto s = dynamic_cast<const OpComposedAssignmentExpr*>(expr))
    {
        llvm::Value* symbol = generateLValueAddress(s->assignment->target.get());
        Type symbolType = getType(getAllocatedType(symbol));

        auto loadSymbol = Builder.CreateLoad(getAllocatedType(symbol), symbol);
        auto rhsValue = generateExpr(s->assignment->value.get());

        ExprGenResult currentValue{loadSymbol, symbolType};
        ExprGenResult opResult = generateBinaryOp(s->op, currentValue, rhsValue);

        auto casted = castValue(opResult.llvm_value, opResult.type.asPrimitive(), symbolType.asPrimitive());

        Builder.CreateStore(casted, symbol);

        return ExprGenResult{casted, symbolType};
    }

    // Variable Expression
    else if(auto s = dynamic_cast<const VariableExpr*>(expr))
    {
        llvm::Value *val = nullptr;
        Type type;

        if(!s->qualifiers.empty()) 
        {
            //variabile qualified, cerca direttamente nel module tramite nome qualified
            std::string mangled = namespaceTable->mangleQualifiedName(s->qualifiers, s->name);
            val = Module->getGlobalVariable(mangled);
            type = getType(getAllocatedType(val));

        } else {
            //nome singolo, carca nello scope
            val = scopeStack.lookupSymbol(s->name).value();
            type = getType(getAllocatedType(val));
        }

        if(type.isArray()) {
            // non si può eseguire un load singolo su un array
            return ExprGenResult{val, type};
        }

        return ExprGenResult {
            Builder.CreateLoad(getAllocatedType(val), val, s->name), 
            type,
        };
    }

    // Array Access Expr
    else if(auto s = dynamic_cast<const ArrayAccessExpr*>(expr))
    {
        llvm::Value* baseAddr = generateLValueAddress(s->base.get());
        llvm::ArrayType* arrType = llvm::cast<llvm::ArrayType>(getAllocatedType(baseAddr));
        llvm::Type* llvmTy = arrType->getElementType();

        //indirizzo del singolo elemento generato dall'accesso (indirizzo di arr[i])
        llvm::Value* addr = generateLValueAddress(s); 

        return ExprGenResult{Builder.CreateLoad(llvmTy, addr), Type(getType(llvmTy))};
    }

    // Array Literal Expression
    else if(auto s = dynamic_cast<const LiteralArrayExpr*>(expr))
    {
        llvm::ArrayType* arrType = llvm::cast<llvm::ArrayType>(getLLVMType(Type{s->type}));
        llvm::AllocaInst* array = Builder.CreateAlloca(arrType);

        int index = 0;
        for(const auto& e : s->elements) 
        {
            llvm::Value* elementValue = generateExpr(e.get()).llvm_value;
            
            //utilizzato per calcolare la posizione dell'elemento nell'array
            std::vector<llvm::Value*> indices = {
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 0),
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), index)
            };

            llvm::Value* elementPtr = Builder.CreateGEP(arrType, array, indices, "arr.elem");
            Builder.CreateStore(elementValue, elementPtr);

            index++;
        }

        return ExprGenResult{array, Type{s->type}};
    }

    // Function Call Expression
    else if(auto s = dynamic_cast<const CallExpr*>(expr))
    {
        std::string func_name;

        if(!s->qualifiers.empty()) {
            //nome qualified
            func_name = namespaceTable->mangleQualifiedName(s->qualifiers, s->name);
        } else {
            func_name = namespaceTable->mangleName(s->name);
        }
        
        llvm::Function *callee = Module->getFunction(func_name);
        if(!callee) throw std::runtime_error("internal compiler error: function not found in module: " + func_name);

        std::vector<llvm::Value*> args;
        for(const std::unique_ptr<Expr> &arg : s->args) {
            args.push_back(generateExpr(arg.get()).llvm_value);
        }

        return ExprGenResult {
            Builder.CreateCall(callee, args, func_name),
            getType(callee->getReturnType())
        };
    }

    // Binary Expression
    else if(auto s = dynamic_cast<const BinaryExpr*>(expr))
    {
        return generateBinaryExpr(s);
    }

    // Unary Expression
    else if(auto s = dynamic_cast<const UnaryExpr*>(expr))
    {
        return generateUnaryExpr(s);
    }

    return ExprGenResult{};
}

ExprGenResult CodeGenerator::generateBinaryExpr(const BinaryExpr *s)
{
    auto left = generateExpr(s->left.get());
    auto right = generateExpr(s->right.get());

    return generateBinaryOp(s->op, left, right);
}

// helper che racchiude la logica di generazione di un operatore binario
ExprGenResult CodeGenerator::generateBinaryOp(TokenType op, ExprGenResult left, ExprGenResult right)
{
    PrimitiveType resultType = types::binaryResultType(op, left.type.asPrimitive(), right.type.asPrimitive());
    PrimitiveType promoteType = types::promotionType(left.type.asPrimitive(), right.type.asPrimitive());

    // conversione implicita
    llvm::Value *L = castValue(left.llvm_value, left.type.asPrimitive(), promoteType);
    llvm::Value *R = castValue(right.llvm_value, right.type.asPrimitive(), promoteType);

    auto createArithmeticOp = [&](auto intOp, auto floatOp) -> ExprGenResult {
        llvm::Value *value;

        if(promoteType == PrimitiveType::Double) {
            value = floatOp();
        } else {
            value = intOp();
        }

        return {value, resultType};
    };

    switch(op) {

    case TokenType::Plus :
        return createArithmeticOp(
            [&] { return Builder.CreateAdd(L, R, "addtmp"); },
            [&] { return Builder.CreateFAdd(L, R, "faddtmp"); });

    case TokenType::Minus :
        return createArithmeticOp(
            [&] { return Builder.CreateSub(L, R, "subtmp"); },
            [&] { return Builder.CreateFSub(L, R, "fsubtmp"); });

    case TokenType::Star:
        return createArithmeticOp(
            [&] { return Builder.CreateMul(L, R, "multmp"); },
            [&] { return Builder.CreateFMul(L, R, "fmultmp"); });

    case TokenType::Slash:
        return createArithmeticOp(
            [&] { return Builder.CreateSDiv(L, R, "sdivtmp"); },
            [&] { return Builder.CreateFDiv(L, R, "fdivtmp"); });

    case TokenType::EqualEqual:
        return createArithmeticOp(
            [&] { return Builder.CreateICmpEQ(L, R, "cmptmp"); },
            [&] { return Builder.CreateFCmpOEQ(L, R, "fcmptmp"); });

    case TokenType::NotEqual:
        return createArithmeticOp(
            [&] { return Builder.CreateICmpNE(L, R, "cmptmp"); },
            [&] { return Builder.CreateFCmpONE(L, R, "fcmptmp"); });

    case TokenType::Less:
        return createArithmeticOp(
            [&] { return Builder.CreateICmpSLT(L, R, "cmptmp"); },
            [&] { return Builder.CreateFCmpOLT(L, R, "fcmptmp"); });

    case TokenType::Greater:
        return createArithmeticOp(
            [&] { return Builder.CreateICmpSGT(L, R, "cmptmp"); },
            [&] { return Builder.CreateFCmpOGT(L, R, "fcmptmp"); });

    case TokenType::LessEqual:
        return createArithmeticOp(
            [&] { return Builder.CreateICmpSLE(L, R, "cmptmp"); },
            [&] { return Builder.CreateFCmpOLE(L, R, "fcmptmp"); });

    case TokenType::GreaterEqual:
        return createArithmeticOp(
            [&] { return Builder.CreateICmpSGE(L, R, "cmptmp"); },
            [&] { return Builder.CreateFCmpOGE(L, R, "fcmptmp"); });

    case TokenType::LogicalAnd :

        return ExprGenResult{
            Builder.CreateAnd(L, R, "andtmp"),
            resultType
        };

    case TokenType::LogicalOr :

        return ExprGenResult{
            Builder.CreateOr(L, R, "ortmp"),
            resultType
        };

    } //end switch

    return ExprGenResult{};
}

ExprGenResult CodeGenerator::generateUnaryExpr(const UnaryExpr *expr)
{
    //recursive call
    auto operand = generateExpr(expr->operand.get());

    auto createUnaryOp = [&](auto intOp, auto floatOp) -> ExprGenResult {
        llvm::Value* value;
        PrimitiveType resultType;

        if(operand.type.is(PrimitiveType::Double))
        {
            value = floatOp();
            resultType = PrimitiveType::Double;
        } else {

            value = intOp();
            resultType = PrimitiveType::Int;
        }

        return ExprGenResult { value, resultType };
    };

    switch(expr->op)
    {
    case TokenType::Minus :
        return createUnaryOp(
            [&]() { return Builder.CreateNeg(operand.llvm_value, "negtmp"); },
            [&]() { return Builder.CreateFNeg(operand.llvm_value, "fnegtmp"); });

    case TokenType::LogicalNot :
        // in llvm il ! logico si crea confrontando il valore per 0
        // !A => A != 0
        return createUnaryOp(
            [&]() { return Builder.CreateICmpEQ(operand.llvm_value, llvm::ConstantInt::get(operand.llvm_value->getType(), 0), "nottmp"); },
            [&]() { return Builder.CreateFCmpOEQ(operand.llvm_value, llvm::ConstantFP::get(operand.llvm_value->getType(), 0.0), "fnottmp"); }
            );
    }

    return ExprGenResult{};
}
