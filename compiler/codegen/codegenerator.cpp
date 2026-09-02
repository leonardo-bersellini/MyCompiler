#include "codegenerator.h"

// per utilizzo di processi figli di cmd.exe
#ifdef _WIN32
    #define popen _popen
    #define pclose _pclose
#endif

#include <llvm/IR/Verifier.h>
#include <iostream>
#include <filesystem>
#include <windows.h>

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

    //CREA TAGETMACHINE
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();

    if (llvm::verifyModule(*Module, &llvm::errs())) {
        llvm::errs() << clr::red << "Modulo LLVM invalido\n" << clr::reset;
        return;
    }
    Module->setTargetTriple(TargetTriple);

    std::string Error;

    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    if (!Target) {
        llvm::errs() << clr::red << Error << clr::reset;
        return;
    }

    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>();

    auto TargetMachine = Target->createTargetMachine(TargetTriple, "x86-64", "", opt, RM);
    if (!TargetMachine) {
        llvm::errs() << clr::red << "Impossibile creare TargetMachine\n" << clr::reset;
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
        llvm::errs() << clr::red << "Cannot emit object file\n" << clr::reset;
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
 * Questa funzione si occupa di utilizzare il linker del progetto per costruire un eseguibile,
 * linkando i file oggetto indicati.
 * Si utilizza il linker lld-link.exe di msys64-ucrt64
 * il flag debug è impostato nella chiamata da parte di compilerdriver. (in base alle opzioni verbose).
 */

bool CodeGenerator::link(const std::string &objFile, const std::string &outputExe, bool debug)
{
    wchar_t wbuf[MAX_PATH];
    GetModuleFileNameW(nullptr, wbuf, MAX_PATH);
    std::filesystem::path appDir = std::filesystem::path(wbuf).parent_path();

    std::string linkerPath = appDir.string() + "/lld-link.exe";
    std::string libDir     = appDir.string() + "/libs";

    //argomenti per lld-link
    std::vector<std::string> args = 
    {
        objFile,
        "-out:" + outputExe,
        "-subsystem:console",
        "-libpath:" + libDir,
        "crt2.o",
        "libmingw32.a",
        "libgcc.a",
        "libgcc_eh.a",
        "libmoldname.a",
        "libmingwex.a",
        "libucrt.a",
        "libadvapi32.a",
        "libshell32.a",
        "libuser32.a",
        "libkernel32.a"
    };

    if(debug) {
        std::cout << "linker path: " << clr::bright_black << linkerPath << clr::reset << std::endl;
        std::cout << "exists: " << std::filesystem::exists(linkerPath) << std::endl;
    }

    std::string cmd = linkerPath;
    for (const auto& a : args) cmd += " " + a;
    cmd += " 2>&1";

    if(debug) std::cout << "executing command: " << clr::bright_black << cmd << clr::reset << std::endl;

    FILE* pipe = popen(cmd.c_str(), "r");
    std::string output;
    char _buf[256];
    while (fgets(_buf, sizeof(_buf), pipe)) output += _buf;
    int exitCode = pclose(pipe);

    if (exitCode != 0) {
        if (debug) std::cout << "\nlinker exit code: " << exitCode << "\n" << output << std::endl;
        return false;
    }

    if(debug) {
        std::cout << "linker: " << clr::bright_black << "lld-link.exe [msys64-ucrt64]\n" << clr::reset << std::endl;
        std::cout << "linker exit code: " << exitCode  << std::endl;
    }

    return true;
}

/// -- STACK FUNCTIONS --- ///

/*
 * Dichiara un simbolo nello scope corrente (l'ultimo elemento dello stack),
 * associandolo alla sua alloca llvm. Non verifica scope esterni per permettere shadowing.
 */

void CodeGenerator::declareSymbol(const std::string& name, llvm::AllocaInst* alloca)
{
    allocaScopeStack.back()[name] = alloca;
}

/*
 * Cerca un simbolo per nome, scorrendo lo stack in ordine inverso: dallo scope più
 * interno (corrente) verso quello più esterno, per rispettare lo shadowing.
 * Se non trovato in nessun livello, il programma lancia un eccezione. Si tratta di un
 * bug interno del compiler, l'analisi semantica avrebbe dovuto bloccarlo.
 */

llvm::AllocaInst* CodeGenerator::lookupSymbol(const std::string& name) 
{
    for (auto it = allocaScopeStack.rbegin(); it != allocaScopeStack.rend(); ++it)
    {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }

    throw std::runtime_error("codegen internal error: undeclared symbol '" + name + "'");
    return nullptr;
}

/* 
 * Aggiunge un nuovo scope (una mappa vuota) in cima allo stack. 
 */

void CodeGenerator::pushScope() {
    allocaScopeStack.push_back(std::unordered_map<std::string, llvm::AllocaInst*>());
}

/*
 * Esce dallo scope corrente rimuovendo un livelo dallo stack 
 */

void CodeGenerator::popScope() {
    allocaScopeStack.pop_back();
}

/*
 * Questa funzione permette di controllare se lo scope corrente è quello globale.
 * Risulta utile per controlli come la distinzione tra declaration e global-declaration
 */

bool CodeGenerator::isGlobalScope() const
{
    return allocaScopeStack.size() == 1;
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
    pushScope(); //scope globale

    for(const auto& st : program.statements)
    {
        generateStmt(st.get());
    }

    popScope();
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

    // Assegnazione
    else if(auto s = dynamic_cast<const AssignmentStmt*>(stmt))
    {
        generateAssignStmt(s); 
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
    pushScope();

    for(const auto& st : st->statements) {
        generateStmt(st.get());

        if (Builder.GetInsertBlock()->getTerminator()) {
            break;
        }
    }

    popScope();
}

void CodeGenerator::generateAssignStmt(const AssignmentStmt *st)
{
    auto symbol = lookupSymbol(st->name);
    Type symbolType = getType(symbol->getAllocatedType());

    if(symbolType.isArray()) 
    {
        if(auto arrLit = dynamic_cast<const LiteralArrayExpr*>(st->value.get())) {
            generateArrayAssignment(arrLit, symbol);
        } else {
            // assegnazione di un array ad un altro: arr1 = arr2;
            auto varExpr = dynamic_cast<const VariableExpr*>(st->value.get());
            llvm::Value* source = lookupSymbol(varExpr->name);

            llvm::ArrayType* arrType = llvm::cast<llvm::ArrayType>(symbol->getAllocatedType());
            llvm::Type* elementType = arrType->getElementType();

            copyArrayElements(source, symbol, arrType, elementType);
        }
    } else {
        auto value = generateExpr(st->value.get());
        // conversione dal tipo del valore al tipo della variabile
        auto casted = castValue(value.llvm_value, value.type.asPrimitive(), getType(symbol->getAllocatedType()).asPrimitive());

        Builder.CreateStore(casted, symbol);
    }
}

void CodeGenerator::generateDeclarationStmt(const DeclarationStmt *st)
{
    if(isGlobalScope()) 
    {
        throw std::runtime_error("codegen internal error: global variable declarations not yet supported");
    }

    // alloca variabile locale
    auto* alloc = Builder.CreateAlloca(getLLVMType(st->type), nullptr, st->name);
    declareSymbol(st->name, alloc);

    if(st->initializer) 
    {
        if(st->type.isArray()) {
            if(auto arrLiteral = dynamic_cast<const LiteralArrayExpr*>(st->initializer.get())) {
                generateArrayAssignment(arrLiteral, alloc);
            } else {
                //inizializzazione arr1 = arr2
                auto varExpr = dynamic_cast<const VariableExpr*>(st->initializer.get());
                llvm::Value* source = lookupSymbol(varExpr->name);
                
                llvm::ArrayType* arrType = llvm::cast<llvm::ArrayType>(lookupSymbol(st->name)->getAllocatedType());
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
    std::vector<llvm::Type*> args;
    for(const FunctionParam& p : st->params) {
        args.push_back(getLLVMType(p.type));
    }

    auto *funcType = llvm::FunctionType::get(getLLVMType(st->returnType), args, false);

    auto *function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                            st->name, Module.get());

    auto *entry = llvm::BasicBlock::Create(Context, "entry", function);

    Builder.SetInsertPoint(entry);

    //creazione dello scope della funzione, con dichiarazione dei parametri come variabili
    pushScope();

    int i = 0;
    for(auto& arg : function->args())
    {
        const FunctionParam& p = st->params[i];

        arg.setName(p.name);

        auto* alloc = Builder.CreateAlloca(getLLVMType(p.type), nullptr, p.name);
        Builder.CreateStore(&arg, alloc);

        declareSymbol(p.name, alloc);

        i++;
    }

    generateStmt(st->body.get());

    //le funzioni void possono terminare senza return esplicito
    if(!Builder.GetInsertBlock()->getTerminator()) {
        if(st->returnType.is(PrimitiveType::Void))
            Builder.CreateRetVoid();
    }

    popScope();
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

    pushScope();
    //init
    if (st->init) generateStmt(st->init.get());

    //creazione dei blocchi necessari
    llvm::BasicBlock* condBB =llvm::BasicBlock::Create(Context, "for.cond", function);
    llvm::BasicBlock* bodyBB =llvm::BasicBlock::Create(Context, "for.body", function);
    llvm::BasicBlock* updateBB = llvm::BasicBlock::Create(Context, "for.update", function);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(Context, "for.end", function);

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

    popScope();
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

    //creazione condition branch
    Builder.CreateCondBr(condition, bodyBB, afterBB);

    //generazione body
    Builder.SetInsertPoint(bodyBB);
    if(st->body) {
        generateStmt(st->body.get());
    }

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

    // String Expression
    else if(auto s = dynamic_cast<const StringExpr*>(expr))
    {
        // TODO In getLLVMType
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

    // Variable Expression
    else if(auto s = dynamic_cast<const VariableExpr*>(expr))
    {
        llvm::AllocaInst *val = lookupSymbol(s->name);
        Type type = getType(val->getAllocatedType());

        if(type.isArray()) {
            // non si può eseguire un load singolo su un array
            return ExprGenResult{val, type};
        }

        return ExprGenResult {
            Builder.CreateLoad(val->getAllocatedType(), val, s->name), 
            type,
        };
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
        llvm::Function *callee = Module->getFunction(s->name);
        if(!callee) return ExprGenResult{};

        std::vector<llvm::Value*> args;
        for(const std::unique_ptr<Expr> &arg : s->args) {
            args.push_back(generateExpr(arg.get()).llvm_value);
        }

        return ExprGenResult {
            Builder.CreateCall(callee, args, s->name),
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

    //TODO -> add NOT logico

    PrimitiveType resultType = types::binaryResultType(s->op, left.type.asPrimitive(), right.type.asPrimitive());
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

    switch(s->op) {

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
