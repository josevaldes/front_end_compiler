/**
 * File: irgen.h
 * -----------
 *  This file defines a class for LLVM IR Generation.
 *
 *  All LLVM instruction related functions or utilities can be implemented
 *  here. You'll need to customize this class heavily to provide any helpers
 *  or untility as you need.
 */

#ifndef _H_IRGen
#define _H_IRGen

#include <stack>
// LLVM headers
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"


class Type;
class ArrayType;
class Expr;

using namespace std;

class IRGenerator {
  public:
    IRGenerator();
    ~IRGenerator();

    llvm::Module   *GetOrCreateModule(const char *moduleID);
    llvm::LLVMContext *GetContext() const { return context; }

    // Add your helper functions here
    llvm::Function *GetFunction() const;
    void      SetFunction(llvm::Function *func);

    llvm::BasicBlock *GetBasicBlock() const;
    void        SetBasicBlock(llvm::BasicBlock *bb);
    
    llvm::BasicBlock *GetDefaultBB() const;
    void        SetDefaultBB(llvm::BasicBlock *bb);
    
    llvm::BasicBlock *GetFooterBB() const;
    void       SetFooterBB(llvm::BasicBlock *bb);
    void  popFooterBB(); 

    llvm::BasicBlock *GetContinueBB() const;
    void       SetContinueBB(llvm::BasicBlock *bb);
    void  popContinueBB(); 

    llvm::SwitchInst* GetSwitchInst();
    void SetSwitchInst(llvm::SwitchInst* switchInst);
    void popSwitchInst();

    llvm::Type *GetIntType() const;
    llvm::Type *GetBoolType() const;
    llvm::Type *GetFloatType() const;
    llvm::Type *GetVoidType() const;
    llvm::Type *GetVecType(unsigned int coor) const;
    llvm::Type *getAstType(Type *t);
    llvm::Constant *isInit(Expr *e, Type *t);
    llvm::Type *GetArrayType(ArrayType *t);


    void setUnreachFlag(bool flag);
    bool getUnreachFlag();

  private:
    llvm::LLVMContext *context;
    llvm::Module      *module;

    // track which function or basic block is active
    llvm::Function    *currentFunc;
    llvm::BasicBlock  *currentBB;
    llvm::BasicBlock  *defaultBB;
    stack<llvm::BasicBlock*>  footerBB;
    stack<llvm::BasicBlock*>  continueBB;
    stack<llvm::SwitchInst*>  switchStack;   
    bool unreachable_flag;
    static const char *TargetTriple;
    static const char *TargetLayout;
};

#endif

