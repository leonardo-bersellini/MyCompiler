#include "codegenerator.h"

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
    Module = std::make_unique<llvm::Module>("my_module", Context);
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

/// --- ENTRY POINT, CODEGEN --- ///

/*
 * : Entry Point :
 * Funzione "entry point" della generazione del codice macchina.
 * Attiva la codegen a partire da un istanza di AST.
 */

void CodeGenerator::generate(const Program &program)
{
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
    if(auto s = dynamic_cast<const DeclarationStmt*>(stmt))
    {
        // alloca variabile
        auto* alloc = Builder.CreateAlloca(getLLVMType(s->type), nullptr, s->name.toStdString());
        symbolTable.insert(s->name, alloc);

        if(s->initializer) {
            // store del valore in inizializzazione
            auto *val = generateExpr(s->initializer.get());
            Builder.CreateStore(val, alloc);
        }
    }
}

/*
 * Funzione di codegen per ogni expr degli statement.
 */

llvm::Value* CodeGenerator::generateExpr(const Expr *expr)
{
    // Number Expression
    if(auto s = dynamic_cast<const NumberExpr*>(expr))
    {
        if(s->isInteger)
            return llvm::ConstantInt::get(getLLVMType(ValueType::Int), (int)s->value);
        else
            return llvm::ConstantFP::get(getLLVMType(ValueType::Double), s->value);
    }

    // String Expression
    else if(auto s = dynamic_cast<const StringExpr*>(expr))
    {
        // TODO In getLLVMType
    }

    // Char Expression
    else if(auto s = dynamic_cast<const CharExpr*>(expr))
    {
        return llvm::ConstantInt::get(getLLVMType(ValueType::Char), s->value.toLatin1());
    }

    // Boolean Expression
    else if(auto s = dynamic_cast<const BooleanExpr*>(expr))
    {
        return llvm::ConstantInt::get(llvm::Type::getInt1Ty(Context), s->value);
    }

    // Variable Expression
    else if(auto s = dynamic_cast<const VariableExpr*>(expr))
    {
        llvm::AllocaInst *val = symbolTable[s->name];

        return Builder.CreateLoad(
            val->getAllocatedType(),
            val,
            s->name.toStdString());
    }

    // Function Call Expression
    else if(auto s = dynamic_cast<const CallExpr*>(expr))
    {
        llvm::Function *callee = Module->getFunction(s->name.toStdString());
        if(!callee) return nullptr;

        std::vector<llvm::Value*> args;
        for(const std::unique_ptr<Expr> &arg : s->args) {
            args.push_back(generateExpr(arg.get()));
        }

        return Builder.CreateCall(callee, args, s->name.toStdString());
    }

    // Binary Expression
    else if(auto s = dynamic_cast<const BinaryExpr*>(expr))
    {
        auto *L = generateExpr(s->left.get());
        auto *R = generateExpr(s->right.get());

        //TODO -> cast impliciti : somma int + double ecc.
        //TODO -> controllo float (double) per ogni operazione
        //TODO -> add NOT logico

        switch(s->op) {
            case TokenType::Plus : return Builder.CreateAdd(L, R, "addtmp");
                break;
            case TokenType::Minus : return Builder.CreateSub(L, R, "subtmp");;
                break;
            case TokenType::Star : return Builder.CreateMul(L, R, "multmp");
                break;
            case TokenType::Slash : return Builder.CreateSDiv(L, R, "divtmp");
                break;
            case TokenType::EqualEqual : return Builder.CreateICmpEQ(L, R, "cmptmp");
                break;
            case TokenType::NotEqual : return Builder.CreateICmpNE(L, R, "cmptmp");
                break;
            case TokenType::Less : return Builder.CreateICmpSLT(L, R, "cmptmp");
                break;
            case TokenType::Greater : return Builder.CreateICmpSGT(L, R, "cmptmp");
                break;
            case TokenType::LessEqual : return Builder.CreateICmpSLE(L, R);
                break;
            case TokenType::GreaterEqual : return Builder.CreateICmpSGE(L, R);
                break;
            case TokenType::LogicalAnd : return Builder.CreateAnd(L, R, "andtmp");
                break;
            case TokenType::LogicalOr : return Builder.CreateOr(L, R, "ortmp");
                break;
        }
    }

    // Unary Expression
    else if(auto s = dynamic_cast<const UnaryExpr*>(expr))
    {
        //recursive call
        return generateExpr(s->operand.get());
    }

    return nullptr;
}




