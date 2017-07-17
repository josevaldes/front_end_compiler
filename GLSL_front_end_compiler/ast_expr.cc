/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */

#include <string.h>
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "symtable.h"
#include "irgen.h"

IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
    ty = Type::intType;
    flag = false;
}
void IntConstant::PrintChildren(int indentLevel) { 
    printf("%d", value);
}

void IntConstant::setExprVal()
{
   llvm_val = llvm::ConstantInt::get(Node::irgen.GetIntType(), value);
}

FloatConstant::FloatConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
    ty = Type::floatType;
    flag = false;
}
void FloatConstant::PrintChildren(int indentLevel) { 
    printf("%g", value);
}

void FloatConstant::setExprVal()
{
   llvm_val = llvm::ConstantFP::get(Node::irgen.GetFloatType(), value);
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
    ty = Type::boolType;
    flag = false;
}
void BoolConstant::PrintChildren(int indentLevel) { 
    printf("%s", value ? "true" : "false");
}

void BoolConstant::setExprVal()
{
   llvm_val = llvm::ConstantInt::get(Node::irgen.GetBoolType(), value);
}

VarExpr::VarExpr(yyltype loc, Identifier *ident) : Expr(loc) {
    Assert(ident != NULL);
    this->id = ident;
    flag = false;
}

void VarExpr::PrintChildren(int indentLevel) {
    id->Print(indentLevel+1);
}

void VarExpr::setType()
{
   Decl* decl = Node::symtable->GetDecl(id);
   Assert(decl != NULL);
   VarDecl* var = dynamic_cast<VarDecl*>(decl);
   if(var != NULL)
   {
      ty = var->GetType();
   }
   else
   {
      FnDecl* fn = dynamic_cast<FnDecl*>(decl);
      Assert(fn != NULL);
      ty = fn->GetType();
   }
}

void VarExpr::setExprVal()
{
   Decl* decl = Node::symtable->GetDecl(id);
   Assert(decl != NULL);
   VarDecl* var = dynamic_cast<VarDecl*>(decl);
   Assert(var != NULL); // To be removed when considering functions
   llvm_val = var->getValue();
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

void Operator::PrintChildren(int indentLevel) {
    printf("%s",tokenString);
}

bool Operator::IsOp(const char *op) const {
    return strcmp(tokenString, op) == 0;
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r) 
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this); 
    (right=r)->SetParent(this);
    flag = false;
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r) 
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL; 
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
    flag = false;
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o) 
  : Expr(Join(l->GetLocation(), o->GetLocation())) {
    Assert(l != NULL && o != NULL);
    (left=l)->SetParent(this);
    (op=o)->SetParent(this);
    flag = false;
}

void CompoundExpr::PrintChildren(int indentLevel) {
   if (left) left->Print(indentLevel+1);
   op->Print(indentLevel+1);
   if (right) right->Print(indentLevel+1);
}
   
ConditionalExpr::ConditionalExpr(Expr *c, Expr *t, Expr *f)
  : Expr(Join(c->GetLocation(), f->GetLocation())) {
    Assert(c != NULL && t != NULL && f != NULL);
    (cond=c)->SetParent(this);
    (trueExpr=t)->SetParent(this);
    (falseExpr=f)->SetParent(this);
    flag = false;
}

void ConditionalExpr::PrintChildren(int indentLevel) {
    cond->Print(indentLevel+1, "(cond) ");
    trueExpr->Print(indentLevel+1, "(true) ");
    falseExpr->Print(indentLevel+1, "(false) ");
}

void ConditionalExpr::Emit()
{
   getExprVal();
}

void ConditionalExpr::setType()
{
   ty = trueExpr->getType();
}

void ConditionalExpr::setExprVal()
{
   llvm_val = llvm::SelectInst::Create(cond->getExprVal(), trueExpr->getExprVal(), falseExpr->getExprVal(), 
                                        "", Node::irgen.GetBasicBlock());
}
void AssignExpr::Emit()
{
   getExprVal();
}

void AssignExpr::setType()
{
   ty = left->getType();
}

void AssignExpr::setExprVal()
{
   ty = right->getType();

   llvm_val = right->getExprVal();
   
   if(ty->IsEquivalentTo(Type::intType))
   {
      if(op->IsOp("+="))
      {
         llvm_val = llvm::BinaryOperator::CreateAdd(left->getExprVal(), right->getExprVal(),
	                                            "", Node::irgen.GetBasicBlock());
      }
      else if(op->IsOp("-="))
      {
         llvm_val = llvm::BinaryOperator::CreateSub(left->getExprVal(), right->getExprVal(),
	                                            "", Node::irgen.GetBasicBlock());
      }

      else if(op->IsOp("*="))
      {
         llvm_val = llvm::BinaryOperator::CreateMul(left->getExprVal(), right->getExprVal(),
	                                            "", Node::irgen.GetBasicBlock());
      }
      else if(op->IsOp("/="))
      {
         llvm_val = llvm::BinaryOperator::CreateSDiv(left->getExprVal(), right->getExprVal(),
	                                            "", Node::irgen.GetBasicBlock());
      }
   }
   else if(ty->IsEquivalentTo(Type::floatType) || ty->IsVector())
   {
      llvm::Value* val = right->getExprVal();
      if(left->getType()->IsVector() && right->getType()->IsEquivalentTo(Type::floatType))
      {
	 unsigned int vect_size;
	 if(left->getType()->IsEquivalentTo(Type::vec2Type))
	 {
            vect_size = 2;
	 }
	 else if(left->getType()->IsEquivalentTo(Type::vec3Type))
	 {
            vect_size = 3;
	 }
	 else
	 {
            vect_size = 4;
	 }
	
	llvm::Value* secondVect = new llvm::AllocaInst(Node::irgen.GetVecType(vect_size), "", Node::irgen.GetBasicBlock());
        secondVect = new llvm::LoadInst(secondVect, "", Node::irgen.GetBasicBlock());

	for(unsigned int i = 0; i < vect_size; ++i)
	{
            secondVect = llvm::InsertElementInst::Create(secondVect, val, 
                                               llvm::ConstantInt::get(Node::irgen.GetIntType(), i),
                                               "", Node::irgen.GetBasicBlock());
					       
        }

	val = secondVect;

      }
      if(op->IsOp("+="))
      {
         llvm_val = llvm::BinaryOperator::CreateFAdd(left->getExprVal(), val,
	                                            "", Node::irgen.GetBasicBlock());
      }
      else if(op->IsOp("-="))
      {
         llvm_val = llvm::BinaryOperator::CreateFSub(left->getExprVal(), val,
	                                            "", Node::irgen.GetBasicBlock());
      }

      else if(op->IsOp("*="))
      {
         llvm_val = llvm::BinaryOperator::CreateFMul(left->getExprVal(), val,
	                                            "", Node::irgen.GetBasicBlock());
      }
      else if(op->IsOp("/="))
      {
         llvm_val = llvm::BinaryOperator::CreateFDiv(left->getExprVal(), val,
	                                            "", Node::irgen.GetBasicBlock());
      }
   }
   if (dynamic_cast<ArrayAccess*>(left) != NULL) {
       ArrayAccess* l = dynamic_cast <ArrayAccess*>(left);
       l->storeArrayElem(llvm_val);
   }

   else if(dynamic_cast<FieldAccess*>(left) != NULL)
   {
      dynamic_cast<FieldAccess*>(left)->storeVectorElem(llvm_val);  
   }
   else {
    Node::symtable->setValue(dynamic_cast<VarExpr*>(left)->GetIdentifier(), llvm_val);
   }
}

void LogicalExpr::setType()
{
   ty = Type::boolType;
}

void LogicalExpr::setExprVal() 
{
  ty = Type::boolType;
  llvm::Value* leftVal  = left->getExprVal();
  llvm::Value* rightVal = right->getExprVal(); 
  
  if (op->IsOp("&&")) {

    llvm_val = llvm::BinaryOperator::CreateAnd(leftVal, rightVal,"", Node::irgen.GetBasicBlock()); 
  }
  else if (op -> IsOp("||")) {
    llvm_val = llvm::BinaryOperator::CreateOr(leftVal, rightVal, "", Node::irgen.GetBasicBlock());
  }
}

void RelationalExpr::setType()
{
   ty = Type::boolType;
}

void RelationalExpr::setExprVal()
{
  Type* typeExpr = left->getType();
  ty = Type::boolType; 
  if(typeExpr->IsEquivalentTo(Type::intType)) {
    if(op->IsOp(">")) {
      llvm_val = llvm::CmpInst::Create(llvm::CmpInst::ICmp, llvm::CmpInst::ICMP_SGT, left->getExprVal(), right->getExprVal(), "", Node::irgen.GetBasicBlock());
    }
    else if(op->IsOp(">=")) {
      llvm_val = llvm::CmpInst::Create(llvm::CmpInst::ICmp, llvm::CmpInst::ICMP_SGE, left->getExprVal(), right->getExprVal(), "", Node::irgen.GetBasicBlock());
    }
    else if(op->IsOp("<")) {
      llvm_val = llvm::CmpInst::Create(llvm::CmpInst::ICmp, llvm::CmpInst::ICMP_SLT, left->getExprVal(), right->getExprVal(), "", Node::irgen.GetBasicBlock());
    }
    else if(op->IsOp("<=")) {
      llvm_val = llvm::CmpInst::Create(llvm::CmpInst::ICmp, llvm::CmpInst::ICMP_SLE, left->getExprVal(), right->getExprVal(), "", Node::irgen.GetBasicBlock());
    }
  }
  else if (typeExpr->IsEquivalentTo(Type::floatType)) {
    if(op->IsOp(">")) {
      llvm_val = llvm::CmpInst::Create(llvm::CmpInst::FCmp, llvm::CmpInst::FCMP_OGT, left->getExprVal(), right->getExprVal(), "", Node::irgen.GetBasicBlock());
    }
    else if(op->IsOp(">=")) {
      llvm_val = llvm::CmpInst::Create(llvm::CmpInst::FCmp, llvm::CmpInst::FCMP_OGE, left->getExprVal(), right->getExprVal(), "", Node::irgen.GetBasicBlock());
    }
    else if(op->IsOp("<")) {
      llvm_val = llvm::CmpInst::Create(llvm::CmpInst::FCmp, llvm::CmpInst::FCMP_OLT, left->getExprVal(), right->getExprVal(), "", Node::irgen.GetBasicBlock());
    }
    else if(op->IsOp("<=")) {
      llvm_val = llvm::CmpInst::Create(llvm::CmpInst::FCmp, llvm::CmpInst::FCMP_OLE, left->getExprVal(), right->getExprVal(), "", Node::irgen.GetBasicBlock());
    }
  }
  
}

void ArithmeticExpr::Emit()
{
   getExprVal();
}

void ArithmeticExpr::setType()
{
   ty = right->getType();
   if(left != NULL && left->getType()->IsVector())
   {
      ty = left->getType();
   }

}
void ArithmeticExpr::setExprVal()
{
   Type* typeExpr = right->getType();
   
   Type* leftType = NULL;
   if (left != NULL) {
     leftType = left->getType();
   }
   
   if((typeExpr->IsEquivalentTo(Type::floatType)  && left != NULL && leftType->IsVector())
      || (typeExpr->IsVector() && left != NULL && leftType->IsEquivalentTo(Type::floatType)))
   {
      
      llvm::Value* vect;
      llvm::Value* num;
      unsigned int vect_size;
      bool isVecLeft = false;
      if(typeExpr->IsVector())
      {
         vect = right->getExprVal();
	 num  = left->getExprVal();
	 if(typeExpr->IsEquivalentTo(Type::vec2Type))
	 {
            vect_size = 2;
	 }
	 else if(typeExpr->IsEquivalentTo(Type::vec3Type))
	 {
            vect_size = 3;
	 }
	 else
	 {
            vect_size = 4;
	 }
      }
      else
      {
         vect = left->getExprVal();
	 num  = right->getExprVal();
         isVecLeft = true;

	 if(leftType->IsEquivalentTo(Type::vec2Type))
	 {
            vect_size = 2;
	 }
	 else if(leftType->IsEquivalentTo(Type::vec3Type))
	 {
            vect_size = 3;
	 }
	 else
	 {
            vect_size = 4;
	 }
      }
	llvm::Value* secondVect = new llvm::AllocaInst(Node::irgen.GetVecType(vect_size), "", Node::irgen.GetBasicBlock());
        secondVect = new llvm::LoadInst(secondVect, "", Node::irgen.GetBasicBlock());

	for(unsigned int i = 0; i < vect_size; ++i)
	{
            secondVect = llvm::InsertElementInst::Create(secondVect, num, 
                                               llvm::ConstantInt::get(Node::irgen.GetIntType(), i),
                                               "", Node::irgen.GetBasicBlock());
					       
       }

       if(op->IsOp("+"))
       {
          llvm_val = llvm::BinaryOperator::CreateFAdd(vect, secondVect, 
	                                               "", Node::irgen.GetBasicBlock());
       }
       else if(op->IsOp("-"))
       {
         if(isVecLeft)
	 {
	    llvm_val = llvm::BinaryOperator::CreateFSub(vect, secondVect, 
	                                            "", Node::irgen.GetBasicBlock());
	 }					 
         else
	 {
	    llvm_val = llvm::BinaryOperator::CreateFSub(secondVect, vect, 
	                                            "", Node::irgen.GetBasicBlock());
	 }					 
       }
       else if(op->IsOp("*"))
       { 
          llvm_val = llvm::BinaryOperator::CreateFMul(vect, secondVect, 
	                                            "", Node::irgen.GetBasicBlock());
       }
       else if(op->IsOp("/"))
       { 
         if(isVecLeft)
	 {
	    llvm_val = llvm::BinaryOperator::CreateFDiv(vect, secondVect, 
	                                            "", Node::irgen.GetBasicBlock());
	 }					 
         else
	 {
	    llvm_val = llvm::BinaryOperator::CreateFDiv(secondVect, vect, 
	                                            "", Node::irgen.GetBasicBlock());
	 }					 
      }     
   }
   
   else if(typeExpr->IsEquivalentTo(Type::intType))
   {
      if(op->IsOp("+") && left != NULL)
      {
	 llvm_val = llvm::BinaryOperator::CreateAdd(left->getExprVal(), right->getExprVal(), 
	                                            "", Node::irgen.GetBasicBlock());
      }
      else if (op->IsOp("+") && left == NULL) {
         llvm_val = right->getExprVal();
      }
      else if(op->IsOp("-") && left != NULL)
      {
	 llvm_val = llvm::BinaryOperator::CreateSub(left->getExprVal(), right->getExprVal(), 
	                                            "", Node::irgen.GetBasicBlock());
      }
      else if (op->IsOp("-") && left == NULL) {
         llvm_val = llvm::BinaryOperator::CreateMul(llvm::ConstantInt::get(Node::irgen.GetIntType(), -1)
						    , right->getExprVal(), "", Node::irgen.GetBasicBlock());
      }
      else if(op->IsOp("*"))
      { 

	 llvm_val = llvm::BinaryOperator::CreateMul(left->getExprVal(), right->getExprVal(), 
	                                            "", Node::irgen.GetBasicBlock());
      }
      else if(op->IsOp("/"))
      { 
	 llvm_val = llvm::BinaryOperator::CreateSDiv(left->getExprVal(), right->getExprVal(), 
	                                            "", Node::irgen.GetBasicBlock());
      }
      else if(op->IsOp("++"))
      {
	 llvm::Value* val = llvm::ConstantInt::get(Node::irgen.GetIntType(), 1);
	 llvm_val = llvm::BinaryOperator::CreateAdd(val, right->getExprVal(), 
	                                            "", Node::irgen.GetBasicBlock());
	 if(dynamic_cast<ArrayAccess*>(right) != NULL)
	 {
            dynamic_cast<ArrayAccess*>(right)->storeArrayElem(llvm_val);
	 }
	 else
	 {
	    Node::symtable->setValue(dynamic_cast<VarExpr*>(right)->GetIdentifier(), llvm_val);
	 }
      }

      else if(op->IsOp("--"))
      {
	 llvm::Value* val = llvm::ConstantInt::get(Node::irgen.GetIntType(), 1);
	 llvm_val = llvm::BinaryOperator::CreateSub( right->getExprVal(), val,
	                                            "", Node::irgen.GetBasicBlock());
	 if(dynamic_cast<ArrayAccess*>(right) != NULL)
	 {
            dynamic_cast<ArrayAccess*>(right)->storeArrayElem(llvm_val);
	 }
	 else
	 {
	    Node::symtable->setValue(dynamic_cast<VarExpr*>(right)->GetIdentifier(), llvm_val);
	 }
      }
   }
   else if(   typeExpr->IsEquivalentTo(Type::floatType) 
           || typeExpr->IsEquivalentTo(Type::vec2Type)
	   || typeExpr->IsEquivalentTo(Type::vec3Type)
	   || typeExpr->IsEquivalentTo(Type::vec4Type)
	  )
   {
        if(op->IsOp("+") && left != NULL)
        {
	   llvm_val = llvm::BinaryOperator::CreateFAdd(left->getExprVal(), right->getExprVal(), 
	                                            "", Node::irgen.GetBasicBlock());
        }
        else if (op->IsOp("+") && left == NULL) {
          llvm_val = right->getExprVal();
        }
        else if(op->IsOp("-") && left != NULL)
        {
	   llvm_val = llvm::BinaryOperator::CreateFSub(left->getExprVal(), right->getExprVal(), 
	                                            "", Node::irgen.GetBasicBlock());
        }
        else if (op->IsOp("-") && left == NULL) {
         llvm_val = llvm::BinaryOperator::CreateFMul(llvm::ConstantFP::get(Node::irgen.GetFloatType(), -1.0)
                                                    , right->getExprVal(), "", Node::irgen.GetBasicBlock());
        }

        else if(op->IsOp("*"))
        { 
	   llvm_val = llvm::BinaryOperator::CreateFMul(left->getExprVal(), right->getExprVal(), 
	                                            "", Node::irgen.GetBasicBlock());
        }
        else if(op->IsOp("/"))
        { 
	   llvm_val = llvm::BinaryOperator::CreateFDiv(left->getExprVal(), right->getExprVal(), 
	                                            "", Node::irgen.GetBasicBlock());
        }
        else if(op->IsOp("++"))
        {
	   llvm::Value* val = llvm::ConstantFP::get(Node::irgen.GetFloatType(), 1.0);
	   if(right->getType()->IsVector())
	   {
               unsigned int vec_size;

	       if(right->getType()->IsEquivalentTo(Type::vec2Type))
	       {
                  vec_size = 2;
	       }
	       else if(right->getType()->IsEquivalentTo(Type::vec3Type))
	       {
                  vec_size = 3;
	       }
               else
	       {
                  vec_size = 4;
	       }
	       llvm::Value* local = new llvm::AllocaInst(Node::irgen.GetVecType(vec_size), "", Node::irgen.GetBasicBlock());
	       local = new llvm::LoadInst(local, "", Node::irgen.GetBasicBlock());

	       for(unsigned int i = 0; i < vec_size; ++i)
	       {
                  local = llvm::InsertElementInst::Create(local, val, 
                                               llvm::ConstantInt::get(Node::irgen.GetIntType(), i),
                                               "", Node::irgen.GetBasicBlock());
	       }

	       val = local;
	   }

	   llvm_val = llvm::BinaryOperator::CreateFAdd(val, right->getExprVal(), 
	                                            "", Node::irgen.GetBasicBlock());
	   if(dynamic_cast<ArrayAccess*>(right) != NULL)
	   {
              dynamic_cast<ArrayAccess*>(right)->storeArrayElem(llvm_val);
	   }
	   else if(dynamic_cast<FieldAccess*>(right) != NULL)
	   {
              FieldAccess* vec = dynamic_cast<FieldAccess*>(right);
	      vec->storeVectorElem(llvm_val);
	   }
	   else
	   {
	      Node::symtable->setValue(dynamic_cast<VarExpr*>(right)->GetIdentifier(), llvm_val);
	   }
        }

        else if(op->IsOp("--"))
        {
	   llvm::Value* val = llvm::ConstantFP::get(Node::irgen.GetFloatType(), 1.0);
	   if(right->getType()->IsVector())
	   {
               unsigned int vec_size;

	       if(right->getType()->IsEquivalentTo(Type::vec2Type))
	       {
                  vec_size = 2;
	       }
	       else if(right->getType()->IsEquivalentTo(Type::vec3Type))
	       {
                  vec_size = 3;
	       }
               else
	       {
                  vec_size = 4;
	       }
	       llvm::Value* local = new llvm::AllocaInst(Node::irgen.GetVecType(vec_size), "", Node::irgen.GetBasicBlock());
	       local = new llvm::LoadInst(local, "", Node::irgen.GetBasicBlock());

	       for(unsigned int i = 0; i < vec_size; ++i)
	       {
                  local = llvm::InsertElementInst::Create(local, val, 
                                               llvm::ConstantInt::get(Node::irgen.GetIntType(), i),
                                               "", Node::irgen.GetBasicBlock());
	       }

	       val = local;
	   }
	   llvm_val = llvm::BinaryOperator::CreateFSub(right->getExprVal(), val,
	                                            "", Node::irgen.GetBasicBlock());
	   if(dynamic_cast<ArrayAccess*>(right) != NULL)
	   {
              dynamic_cast<ArrayAccess*>(right)->storeArrayElem(llvm_val);
	   }
	   else if(dynamic_cast<FieldAccess*>(right) != NULL)
	   {
              FieldAccess* vec = dynamic_cast<FieldAccess*>(right);
	      vec->storeVectorElem(llvm_val);
	   }
	   else
	   {
	      Node::symtable->setValue(dynamic_cast<VarExpr*>(right)->GetIdentifier(), llvm_val);
	   }
        }
   }
}

void PostfixExpr::Emit()
{
   getExprVal();
}
void PostfixExpr::setType()
{
  ty = left->getType();
}

void PostfixExpr::setExprVal()
{

  Type* typeExpr = left->getType();
      
  if(typeExpr->IsEquivalentTo(Type::intType)) {
    if(op->IsOp("++")){
      

      llvm_val = llvm::BinaryOperator::CreateAdd(left->getExprVal(),
						llvm::ConstantInt::get(Node::irgen.getAstType(typeExpr), 1),
						"", Node::irgen.GetBasicBlock());
      
      if(dynamic_cast<ArrayAccess*>(left) != NULL)
      {
         dynamic_cast<ArrayAccess*>(left)->storeArrayElem(llvm_val);
      } 
      
      else
      {
         Node::symtable->setValue(dynamic_cast<VarExpr*>(left)->GetIdentifier(), llvm_val);
      }
      llvm_val = left->getExprVal();
    }
    if(op->IsOp("--")){
      llvm_val = llvm::BinaryOperator::CreateSub(left->getExprVal(),
                                                llvm::ConstantInt::get(Node::irgen.getAstType(typeExpr), 1),
                                                "", Node::irgen.GetBasicBlock());
      
      if(dynamic_cast<ArrayAccess*>(left) != NULL)
      {
         dynamic_cast<ArrayAccess*>(left)->storeArrayElem(llvm_val);
      }
      else
      {
         Node::symtable->setValue(dynamic_cast<VarExpr*>(left)->GetIdentifier(), llvm_val);
      }

      llvm_val = left->getExprVal();

    }
  }
  else if (typeExpr->IsEquivalentTo(Type::floatType)
           || typeExpr->IsVector() ) {
    
    if(op->IsOp("++")){
	   llvm::Value* val = llvm::ConstantFP::get(Node::irgen.GetFloatType(), 1.0);
	   if(left->getType()->IsVector())
	   {
               unsigned int vec_size;

	       if(left->getType()->IsEquivalentTo(Type::vec2Type))
	       {
                  vec_size = 2;
	       }
	       else if(left->getType()->IsEquivalentTo(Type::vec3Type))
	       {
                  vec_size = 3;
	       }
               else
	       {
                  vec_size = 4;
	       }
	       llvm::Value* local = new llvm::AllocaInst(Node::irgen.GetVecType(vec_size), "", Node::irgen.GetBasicBlock());
	       local = new llvm::LoadInst(local, "", Node::irgen.GetBasicBlock());

	       for(unsigned int i = 0; i < vec_size; ++i)
	       {
                  local = llvm::InsertElementInst::Create(local, val, 
                                               llvm::ConstantInt::get(Node::irgen.GetIntType(), i),
                                               "", Node::irgen.GetBasicBlock());
	       }

	       val = local;
	   }
      llvm_val = llvm::BinaryOperator::CreateFAdd(left->getExprVal(),val,
                                                "", Node::irgen.GetBasicBlock());
      
      if(dynamic_cast<ArrayAccess*>(left) != NULL)
      {
         dynamic_cast<ArrayAccess*>(left)->storeArrayElem(llvm_val);
      }
      
      else if(dynamic_cast<FieldAccess*>(left) != NULL)
      {
         FieldAccess* vec = dynamic_cast<FieldAccess*>(left);
	 vec->storeVectorElem(llvm_val);
      }
      else
      {
         Node::symtable->setValue(dynamic_cast<VarExpr*>(left)->GetIdentifier(), llvm_val);
      }
      
      llvm_val = left->getExprVal();
    }
    if(op->IsOp("--")){
      
	   llvm::Value* val = llvm::ConstantFP::get(Node::irgen.GetFloatType(), 1.0);
	   if(left->getType()->IsVector())
	   {
               unsigned int vec_size;

	       if(left->getType()->IsEquivalentTo(Type::vec2Type))
	       {
                  vec_size = 2;
	       }
	       else if(left->getType()->IsEquivalentTo(Type::vec3Type))
	       {
                  vec_size = 3;
	       }
               else
	       {
                  vec_size = 4;
	       }
	       llvm::Value* local = new llvm::AllocaInst(Node::irgen.GetVecType(vec_size), "", Node::irgen.GetBasicBlock());
	       local = new llvm::LoadInst(local, "", Node::irgen.GetBasicBlock());

	       for(unsigned int i = 0; i < vec_size; ++i)
	       {
                  local = llvm::InsertElementInst::Create(local, val, 
                                               llvm::ConstantInt::get(Node::irgen.GetIntType(), i),
                                               "", Node::irgen.GetBasicBlock());
	       }

	       val = local;
	   }

      llvm_val = llvm::BinaryOperator::CreateFSub(left->getExprVal(),
                                                llvm::ConstantFP::get(Node::irgen.getAstType(typeExpr), 1.0),
                                                "", Node::irgen.GetBasicBlock());
      
      if(dynamic_cast<ArrayAccess*>(left) != NULL)
      {
         dynamic_cast<ArrayAccess*>(left)->storeArrayElem(llvm_val);
      }
      
      else if(dynamic_cast<FieldAccess*>(left) != NULL)
      {
         FieldAccess* vec = dynamic_cast<FieldAccess*>(left);
	 vec->storeVectorElem(llvm_val);
      }

      else
      {
         Node::symtable->setValue(dynamic_cast<VarExpr*>(left)->GetIdentifier(), llvm_val);
      }


      llvm_val = left->getExprVal();
    }   
  }
}

void EqualityExpr::setType()
{
   ty = Type::boolType;
}

void EqualityExpr::setExprVal()
{
   if(left->getType()->IsEquivalentTo(Type::intType))
   {
      if(op->IsOp("=="))
      {
         llvm_val = llvm::CmpInst::Create(llvm::CmpInst::ICmp,
	                                  llvm::CmpInst::ICMP_EQ,
					  left->getExprVal(),
                                          right->getExprVal(),"",
					  Node::irgen.GetBasicBlock());
      }

      else if(op->IsOp("!="))
      {
         llvm_val = llvm::CmpInst::Create(llvm::CmpInst::ICmp,
	                                  llvm::CmpInst::ICMP_NE,
					  left->getExprVal(),
                                          right->getExprVal(), "",
					  Node::irgen.GetBasicBlock());
      }
   }
   else if(left->getType()->IsEquivalentTo(Type::floatType))
   {
      if(op->IsOp("=="))
      {
         llvm_val = llvm::CmpInst::Create(llvm::CmpInst::FCmp,
	                                  llvm::CmpInst::FCMP_OEQ,
					  left->getExprVal(),
                                          right->getExprVal(), "",
					  Node::irgen.GetBasicBlock());
      }

      else if(op->IsOp("!="))
      {
         llvm_val = llvm::CmpInst::Create(llvm::CmpInst::FCmp,
	                                  llvm::CmpInst::FCMP_ONE,
					  left->getExprVal(),
                                          right->getExprVal(),"",
					  Node::irgen.GetBasicBlock());
      }
   }
}

ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
    flag = false;
}

void ArrayAccess::PrintChildren(int indentLevel) {
    base->Print(indentLevel+1);
    subscript->Print(indentLevel+1, "(subscript) ");
}
void ArrayAccess::setType()
{
  ArrayType* arrType = dynamic_cast<ArrayType*>(base->getType());
  Assert(arrType != NULL);
  ty = arrType->GetElemType();
}

void ArrayAccess::Emit() 
{
  setExprVal();
}

void ArrayAccess::storeArrayElem(llvm::Value* rhs)
{
    VarDecl* array = dynamic_cast<VarDecl*> (Node::symtable->GetDecl(dynamic_cast<VarExpr*>(getBase())->GetIdentifier()));
    vector<llvm::Value*> indexVals;
    indexVals.push_back(llvm::ConstantInt::get(Node::irgen.GetIntType(), 0));
    indexVals.push_back(getSub()->getExprVal());
    llvm::ArrayRef<llvm::Value*>indRef(indexVals);

    llvm::Value* ptr = llvm::GetElementPtrInst::Create(array->getVal(), indRef, "", Node::irgen.GetBasicBlock());
    llvm::StoreInst *store = new llvm::StoreInst(rhs, ptr, Node::irgen.GetBasicBlock());
}

FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
    flag = false;
    
}


void FieldAccess::PrintChildren(int indentLevel) {
    if (base) base->Print(indentLevel+1);
    field->Print(indentLevel+1);
}

void FieldAccess::setType()
{
  string fields = field->GetName();
  switch(fields.length())
  {
     case 1:
        ty = Type::floatType;
	break;
     case 2:
        ty = Type::vec2Type;
        break;
     case 3:
        ty = Type::vec3Type;
        break;
     case 4:
        ty = Type::vec4Type;
        break;
     default:;
  }

}

void FieldAccess::Emit() {
  getExprVal();
}

void FieldAccess::setExprVal() {
  setType();
  map<char,unsigned int> fieldsID;
  string fields = field->GetName();
  fieldsID['x'] = 0;
  fieldsID['y'] = 1;
  fieldsID['z'] = 2;
  fieldsID['w'] = 3;

  if(ty->IsEquivalentTo(Type::floatType))
  {
    llvm_val  = llvm::ExtractElementInst::Create(base->getExprVal(), 
                                                 llvm::ConstantInt::get(Node::irgen.GetIntType(), fieldsID[field->GetName()[0]]) ,
						 "",
						 Node::irgen.GetBasicBlock());
  }
  else
  {
     llvm::Value *local = new llvm::AllocaInst(Node::irgen.GetVecType(fields.length()), "", Node::irgen.GetBasicBlock());
     local = new llvm::LoadInst(local, "", Node::irgen.GetBasicBlock());
     for(unsigned int i = 0; i < fields.length(); ++i)
     {
       llvm::Value* elem  = llvm::ExtractElementInst::Create(base->getExprVal(), 
                                                 llvm::ConstantInt::get(Node::irgen.GetIntType(), fieldsID[field->GetName()[i]]),
						 "",
						 Node::irgen.GetBasicBlock());
       
       local = llvm::InsertElementInst::Create(local, elem, 
                                               llvm::ConstantInt::get(Node::irgen.GetIntType(), i),
                                               "", Node::irgen.GetBasicBlock());
     }
     
     llvm_val = local;
  }
  
}

void FieldAccess::storeVectorElem(llvm::Value* rhs)
{
	 map<char, unsigned int> fieldID;
	 fieldID['x'] = 0;
	 fieldID['y'] = 1;
	 fieldID['z'] = 2;
	 fieldID['w'] = 3;
	 string vect_fields = getField()->GetName();
	 unsigned int vecSize = vect_fields.length();
	 
	 Expr* lhs = getBase();
	 llvm::Value* newVector = lhs->getExprVal();

	 if(vecSize == 1)
	 {
	    newVector  = llvm::InsertElementInst::Create(newVector, rhs,
	                                                llvm::ConstantInt::get(Node::irgen.GetIntType(), fieldID[getField()->GetName()[0]]),
							"", Node::irgen.GetBasicBlock());

	 }

	 else
	 {
	    for(unsigned int i = 0; i < vecSize; ++i)
	    {
               llvm::Value* elem = llvm::ExtractElementInst::Create(rhs, 
	                                                         llvm::ConstantInt::get(Node::irgen.GetIntType(), i),
								 "", Node::irgen.GetBasicBlock());
	       newVector  = llvm::InsertElementInst::Create(newVector, elem,
	                                                llvm::ConstantInt::get(Node::irgen.GetIntType(), fieldID[getField()->GetName()[i]]),
							"", Node::irgen.GetBasicBlock());
	     }
	 }
         
         if(dynamic_cast<VarExpr*>(lhs) != NULL)
	 {
	    VarExpr* var = dynamic_cast<VarExpr*>(lhs);
	    Node::symtable->setValue(var->GetIdentifier(), newVector);
         }
	 else if(dynamic_cast<ArrayAccess*>(lhs) != NULL)
	 {
            dynamic_cast<ArrayAccess*>(lhs)->storeArrayElem(newVector);
	 }   
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
    flag = false;
}

void Call::PrintChildren(int indentLevel) {
   if (base) base->Print(indentLevel+1);
   if (field) field->Print(indentLevel+1);
   if (actuals) actuals->PrintAll(indentLevel+1, "(actuals) ");
}

void Call::Emit()
{
   setExprVal();
}

void Call::setType()
{
   getExprVal();
   ty = Node::symtable->getCurrentCall()->GetType();
}

void Call::setExprVal()
{
   Node::symtable->setCurrentCall(field);
   llvm::Function* f = Node::symtable->getCurrentCall()->GetFun();

   vector<llvm::Value*> args;
   for(int i = 0; i < actuals->NumElements(); ++i)
   {
      args.push_back(actuals->Nth(i)->getExprVal());
   }
   
   llvm::ArrayRef<llvm::Value*> argsRef(args);

   llvm_val = llvm::CallInst::Create(f, argsRef, "", Node::irgen.GetBasicBlock());
}
void ArrayAccess::setExprVal()
{
   Node::symtable->setArray((dynamic_cast<VarExpr*>(base))->GetIdentifier());
   
   llvm::Value *ar = Node::symtable->getArray()->getVal();

   vector<llvm::Value*> indexVals;
   indexVals.push_back(llvm::ConstantInt::get(Node::irgen.GetIntType(), 0));
   indexVals.push_back(subscript->getExprVal());
   
   llvm::ArrayRef<llvm::Value*>inxRef(indexVals); 
   llvm_val = llvm::GetElementPtrInst::Create(ar, inxRef, "", Node::irgen.GetBasicBlock());
   llvm_val = new llvm::LoadInst(llvm_val, "", Node::irgen.GetBasicBlock()); 
}
