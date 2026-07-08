#include "codegenerator.h"

#include <llvm/IR/Verifier.h>
#include <iostream>

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

void CodeGenerator::emitIR()
{
    std::cout << "\nIR [llvm-generated]:" << std::endl;
    Module->print(llvm::outs(), nullptr);
    //qDebug() << llvm::verifyModule(*Module, &llvm::errs());
}

void CodeGenerator::buildTarget()
{
    //INIT TARGET
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    //CREA TAGETMACHINE
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();

    if (llvm::verifyModule(*Module, &llvm::errs())) {
        llvm::errs() << "Modulo LLVM invalido\n";
        return;
    }
    Module->setTargetTriple(TargetTriple);

    std::string Error;

    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    if (!Target) {
        llvm::errs() << Error;
        return;
    }

    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>();

    auto TargetMachine = Target->createTargetMachine(TargetTriple, "x86-64", "", opt, RM);
    if (!TargetMachine) {
        llvm::errs() << "Impossibile creare TargetMachine\n";
        return;
    }

    Module->setDataLayout(TargetMachine->createDataLayout());

    //CREAZIONE FILE OBJ
    std::error_code EC;

    llvm::raw_fd_ostream dest("code.obj", EC, llvm::sys::fs::OF_None);
    if (EC) {
        qDebug() << "Errore apertura file:" << QString::fromStdString(EC.message());
        return;
    }
    llvm::legacy::PassManager pass;

    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile))
    {
        llvm::errs() << "Cannot emit object file\n";
        return;
    }

    std::cout << "\nDebug info:" << std::endl;
    std::cout << "Target triple:" << TargetTriple << std::endl;
    std::cout << "Numero funzioni nel modulo:" << Module->size() << std::endl;
    std::cout << "dest.has_error():" << dest.has_error() << std::endl;

    pass.run(*Module.get());
    dest.flush();

    std::cout << std::endl;
    std::cout << "-Obj creation terminated in code.obj, now link the file with one of the following commands:" << std::endl;
    std::cout << "clang <main>.obj -o <main>.exe" << std::endl;
    std::cout << "gcc <main>.obj -o <main>.exe" << std::endl;
    std::cout << "link.exe main.obj /OUT:main.exe" << std::endl;
}

/// --- UTILITIES --- ///

/*
 * Funzione di utility, permette di convertire i tipi dell'ast in tipi llvm.
 */

llvm::Type* CodeGenerator::getLLVMType(const ValueType &type)
{
    switch(type) {
        case ValueType::Int : return llvm::Type::getInt32Ty(Context);
            break;
        case ValueType::Double : return llvm::Type::getDoubleTy(Context);
            break;
        case ValueType::Bool : return llvm::Type::getInt1Ty(Context);
            break;
        case ValueType::Char : return llvm::Type::getInt8Ty(Context);
            break;
        default : return nullptr;
            break;
    }
}

/*
 * Inversa di getLLVMType
 */

ValueType CodeGenerator::getValueType(llvm::Type *type)
{
    if (type->isIntegerTy(1))
        return ValueType::Bool;

    if (type->isIntegerTy(8))
        return ValueType::Char;

    if (type->isIntegerTy(32))
        return ValueType::Int;

    if (type->isDoubleTy())
        return ValueType::Double;

    if (type->isVoidTy())
        return ValueType::Void;

    // String e altri tipi saranno gestiti successivamente
    return ValueType::Error;
}

/*
 * Funzione che applica le conversioni implicite
 */

llvm::Value* CodeGenerator::castValue(llvm::Value *value, ValueType from, ValueType to)
{
    if (from == to) return value;

    switch (from)
    {
    case ValueType::Int:

        if (to == ValueType::Double)
            return Builder.CreateSIToFP(value, getLLVMType(ValueType::Double), "sitofp");

        break;

    case ValueType::Double:
        //non ancora utilizzato in types.h, non valido
        if (to == ValueType::Int)
            return Builder.CreateFPToSI(value, getLLVMType(ValueType::Int), "fptosi");

        break;

    default:
        break;
    }

    // Cast non supportato
    return value;
}

/// --- ENTRY POINT, CODEGEN --- ///

/*
 * : Entry Point :
 * Funzione "entry point" della generazione del codice macchina.
 * Attiva la codegen a partire da un istanza di AST.
 */

void CodeGenerator::generate(const Program &program)
{
    //TODO refactory di ogni cosa spostando il codice in funzioni solo per chiarezza

    for(const auto& st : program.statements)
    {
        generateStmt(st.get());
    }
}

/*
 * Funzione di codegen per ogni stmt del programma.
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

    return;
}

void CodeGenerator::generateScopeStmt(const BlockStmt *st)
{
    for(const auto& st : st->statements) {
        generateStmt(st.get());

        if (Builder.GetInsertBlock()->getTerminator()) {
            break;
        }
    }
}

void CodeGenerator::generateAssignStmt(const AssignmentStmt *st)
{
    auto symbol = symbolTable[st->name];

    auto value = generateExpr(st->value.get());

    // conversione dal tipo del valore al tipo della variabile
    auto casted = castValue(value.llvm_value, value.type, getValueType(symbol->getAllocatedType()));

    Builder.CreateStore(casted, symbol);
}

void CodeGenerator::generateDeclarationStmt(const DeclarationStmt *st)
{
    // alloca variabile
    auto* alloc = Builder.CreateAlloca(getLLVMType(st->type), nullptr, st->name.toStdString());
    symbolTable.insert(st->name, alloc);

    if(st->initializer) {
        // store del valore in inizializzazione
        auto *val = generateExpr(st->initializer.get()).llvm_value;
        Builder.CreateStore(val, alloc);
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
                                            st->name.toStdString(), Module.get());

    auto *entry = llvm::BasicBlock::Create(Context, "entry", function);

    Builder.SetInsertPoint(entry);

    generateStmt(st->body.get());

    if (!Builder.GetInsertBlock()->getTerminator()) {
        if (st->returnType == ValueType::Void)
            Builder.CreateRetVoid();
    }
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
    llvm::Value *condition = generateExpr(st->condition.get()).llvm_value;
    Builder.CreateCondBr(condition, bodyBB, afterBB);

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


/*
 * Funzione di codegen per ogni expr degli statement.
 */

ExprGenResult CodeGenerator::generateExpr(const Expr *expr)
{
    // Number Expression
    if(auto s = dynamic_cast<const NumberExpr*>(expr))
    {
        if(s->isInteger)
            return ExprGenResult {
                llvm::ConstantInt::get(getLLVMType(ValueType::Int), (int)s->value),
                ValueType::Int
            };
        else
            return ExprGenResult {
                llvm::ConstantFP::get(getLLVMType(ValueType::Double), s->value),
                ValueType::Double
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
            llvm::ConstantInt::get(getLLVMType(ValueType::Char), s->value.toLatin1()),
            ValueType::Char
        };
    }

    // Boolean Expression
    else if(auto s = dynamic_cast<const BooleanExpr*>(expr))
    {
        return ExprGenResult {
            llvm::ConstantInt::get(llvm::Type::getInt1Ty(Context), s->value),
            ValueType::Bool
        };
    }

    // Variable Expression
    else if(auto s = dynamic_cast<const VariableExpr*>(expr))
    {
        llvm::AllocaInst *val = symbolTable[s->name];

        return ExprGenResult {
            Builder.CreateLoad(val->getAllocatedType(), val, s->name.toStdString()),
            getValueType(val->getAllocatedType()),
        };
    }

    // Function Call Expression
    else if(auto s = dynamic_cast<const CallExpr*>(expr))
    {
        llvm::Function *callee = Module->getFunction(s->name.toStdString());
        if(!callee) return ExprGenResult{};

        std::vector<llvm::Value*> args;
        for(const std::unique_ptr<Expr> &arg : s->args) {
            args.push_back(generateExpr(arg.get()).llvm_value);
        }

        return ExprGenResult {
            Builder.CreateCall(callee, args, s->name.toStdString()),
            getValueType(callee->getReturnType())
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

    ValueType resultType = Type::binaryResultType(s->op, left.type, right.type);
    ValueType promoteType = Type::promotionType(left.type, right.type);

    // conversione implicita
    llvm::Value *L = castValue(left.llvm_value, left.type, promoteType);
    llvm::Value *R = castValue(right.llvm_value, right.type, promoteType);

    switch(s->op) {

    case TokenType::Plus :

        if (promoteType == ValueType::Double) {
            return ExprGenResult{
                Builder.CreateFAdd(L, R, "faddtmp"),
                resultType
            };
        }

        return ExprGenResult{
            Builder.CreateAdd(L, R, "addtmp"),
            resultType
        };

    case TokenType::Minus :

        if (promoteType == ValueType::Double) {
            return ExprGenResult{
                Builder.CreateFSub(L, R, "fsubtmp"),
                resultType
            };
        }

        return ExprGenResult{
            Builder.CreateSub(L, R, "subtmp"),
            resultType
        };

    case TokenType::Star :

        if (promoteType == ValueType::Double) {
            return ExprGenResult{
                Builder.CreateFMul(L, R, "fmultmp"),
                resultType
            };
        }

        return ExprGenResult{
            Builder.CreateMul(L, R, "multmp"),
            resultType
        };

    case TokenType::Slash :

        if (promoteType == ValueType::Double) {
            return ExprGenResult{
                Builder.CreateFDiv(L, R, "fdivtmp"),
                resultType
            };
        }

        return ExprGenResult{
            Builder.CreateSDiv(L, R, "divtmp"),
            resultType
        };

    case TokenType::EqualEqual :

        if (promoteType == ValueType::Double) {
            return ExprGenResult{
                Builder.CreateFCmpOEQ(L, R, "fcmptmp"),
                resultType
            };
        }

        return ExprGenResult{
            Builder.CreateICmpEQ(L, R, "cmptmp"),
            resultType
        };

    case TokenType::NotEqual :

        if (promoteType == ValueType::Double) {
            return ExprGenResult{
                Builder.CreateFCmpONE(L, R, "fcmptmp"),
                resultType
            };
        }

        return ExprGenResult{
            Builder.CreateICmpNE(L, R, "cmptmp"),
            resultType
        };

    case TokenType::Less :

        if (promoteType == ValueType::Double) {
            return ExprGenResult{
                Builder.CreateFCmpOLT(L, R, "fcmptmp"),
                resultType
            };
        }

        return ExprGenResult{
            Builder.CreateICmpSLT(L, R, "cmptmp"),
            resultType
        };

    case TokenType::Greater :

        if (promoteType == ValueType::Double) {
            return ExprGenResult{
                Builder.CreateFCmpOGT(L, R, "fcmptmp"),
                resultType
            };
        }

        return ExprGenResult{
            Builder.CreateICmpSGT(L, R, "cmptmp"),
            resultType
        };

    case TokenType::LessEqual :

        if (promoteType == ValueType::Double) {
            return ExprGenResult{
                Builder.CreateFCmpOLE(L, R, "fcmptmp"),
                resultType
            };
        }

        return ExprGenResult{
            Builder.CreateICmpSLE(L, R, "cmptmp"),
            resultType
        };

    case TokenType::GreaterEqual :

        if (promoteType == ValueType::Double) {
            return ExprGenResult{
                Builder.CreateFCmpOGE(L, R, "fcmptmp"),
                resultType
            };
        }

        return ExprGenResult{
            Builder.CreateICmpSGE(L, R, "cmptmp"),
            resultType
        };

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

    switch(expr->op)
    {
    case TokenType::Minus :

        if(operand.type == ValueType::Double)
        {
            return ExprGenResult {
                Builder.CreateFNeg(operand.llvm_value, "fnegtmp"),
                ValueType::Double
            };
        }

        return ExprGenResult {
            Builder.CreateNeg(operand.llvm_value, "negtmp"),
            ValueType::Int
        };

    case TokenType::LogicalNot :

        if(operand.type == ValueType::Double)
        {
            return ExprGenResult {
                Builder.CreateNot(operand.llvm_value, "fnottmp"),
                ValueType::Double
            };
        }

        return ExprGenResult {
            Builder.CreateNot(operand.llvm_value, "nottmp"),
            ValueType::Int
        };
    }

    return ExprGenResult{};
}
