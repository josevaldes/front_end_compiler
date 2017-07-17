/* irgen.cc -  LLVM IR generator
 *
 * You can implement any LLVM related functions here.
 */

#include "irgen.h"
#include "ast_type.h"
#include "ast_expr.h"

IRGenerator::IRGenerator() :
    context(NULL),
    module(NULL),
    currentFunc(NULL),
    currentBB(NULL)
{
   unreachable_flag = false;
}

IRGenerator::~IRGenerator() {
}

bool IRGenerator::getUnreachFlag()
{
   return unreachable_flag;
}

void IRGenerator::setUnreachFlag(bool flag)
{
   unreachable_flag = flag;  
}

llvm::Module *IRGenerator::GetOrCreateModule(const char *moduleID)
{
   if ( module == NULL ) {
     context = new llvm::LLVMContext();
     module  = new llvm::Module(moduleID, *context);
     module->setTargetTriple(TargetTriple);
     module->setDataLayout(TargetLayout);
   }
   return module;
}

void IRGenerator::SetFunction(llvm::Function *func) {
   currentFunc = func;
}

llvm::Function *IRGenerator::GetFunction() const {
   return currentFunc;
}

void IRGenerator::SetBasicBlock(llvm::BasicBlock *bb) {
   if(currentBB != bb)
   {
      unreachable_flag = false;
   }

   currentBB = bb;
}

llvm::BasicBlock *IRGenerator::GetBasicBlock() const {
   return currentBB;
}

llvm::BasicBlock *IRGenerator::GetDefaultBB() const {
   return defaultBB;
}

void IRGenerator::SetDefaultBB(llvm::BasicBlock *bb) {
  defaultBB = bb;
}

void IRGenerator::SetFooterBB(llvm::BasicBlock *bb) {
  footerBB.push(bb);
}

void IRGenerator::popFooterBB()
{
   if(!footerBB.empty())
   {
      footerBB.pop();
   }
}

llvm::BasicBlock *IRGenerator::GetFooterBB() const {
   return footerBB.top();
}

void IRGenerator::SetContinueBB(llvm::BasicBlock *bb) {
  continueBB.push(bb);
}

void IRGenerator::popContinueBB()
{
   if(!continueBB.empty())
   {
      continueBB.pop();
   }
}

void IRGenerator::SetSwitchInst(llvm::SwitchInst* switchInst) {
  switchStack.push(switchInst);
}

void IRGenerator::popSwitchInst()
{
   if(!switchStack.empty())
   {
      switchStack.pop();
   }
}

llvm::SwitchInst* IRGenerator::GetSwitchInst()
{
   return switchStack.top();
}

llvm::BasicBlock *IRGenerator::GetContinueBB() const {
   return continueBB.top();
}

llvm::Type *IRGenerator::GetIntType() const {
   llvm::Type *ty = llvm::Type::getInt32Ty(*context);
   return ty;
}

llvm::Type *IRGenerator::GetBoolType() const {
   llvm::Type *ty = llvm::Type::getInt1Ty(*context);
   return ty;
}

llvm::Type *IRGenerator::GetFloatType() const {
   llvm::Type *ty = llvm::Type::getFloatTy(*context);
   return ty;
}

llvm::Type *IRGenerator::GetVoidType() const {
  llvm::Type *ty = llvm::Type::getVoidTy(*context);
  return ty;
}

llvm::Type *IRGenerator::GetArrayType(ArrayType *t)  {
 llvm::Type *ty = llvm::ArrayType::get(getAstType(t->GetElemType()),t->getElemCount());
return ty;
}

llvm::Type *IRGenerator::GetVecType(unsigned int coor) const {
  llvm::VectorType *ty = llvm::VectorType::get(llvm::Type::getFloatTy(*context),coor);
  return ty;
}

const char *IRGenerator::TargetLayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128";

const char *IRGenerator::TargetTriple = "x86_64-redhat-linux-gnu";

llvm::Type *IRGenerator::getAstType(Type *t) {
  if (t->IsEquivalentTo(Type::intType)) {
    return this->GetIntType();
  }
  else if (t->IsEquivalentTo(Type::floatType)) {
    return this->GetFloatType();
  }
  else if (t->IsEquivalentTo(Type::boolType)) {
    return this->GetBoolType();
  }
  else if (t->IsEquivalentTo(Type::voidType)) {
    return this->GetVoidType();
  } 
  else if (t->IsEquivalentTo(Type::vec2Type)) {
    return this->GetVecType(2);
  }
  else if (t->IsEquivalentTo(Type::vec3Type)) {
    return this->GetVecType(3);
  }
  else if (t->IsEquivalentTo(Type::vec4Type)) {
    return this->GetVecType(4);
  }
  else if ((dynamic_cast<ArrayType*>(t)) != NULL) {
    return this->GetArrayType((dynamic_cast<ArrayType*>(t)));
  }
  else {
    Assert(false);
    return NULL;
  }
}
llvm::Constant *IRGenerator::isInit(Expr *e, Type *t) {
  llvm::Constant *init;
  IntConstant* i =  dynamic_cast <IntConstant*> (e);
  FloatConstant* f = dynamic_cast <FloatConstant*> (e);
  BoolConstant* b = dynamic_cast <BoolConstant*> (e);  

  if (i != NULL ) {
    init = llvm::ConstantInt::get(IRGenerator::getAstType(t),i->getValue());
  }
  else if ( f  != NULL) {
    init = llvm::ConstantFP::get(IRGenerator::getAstType(t),f->getValue());
  }
  else if (b != NULL) {
    init = llvm::ConstantInt::get(IRGenerator::getAstType(t),b->getValue());
  }
  else {
    return NULL;
  }
  return init;
}



