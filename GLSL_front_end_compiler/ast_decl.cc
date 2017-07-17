/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "symtable.h"        
#include <vector>  
       
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}

VarDecl::VarDecl(Identifier *n, Type *t, Expr *e) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
    if (e) (assignTo=e)->SetParent(this);
    typeq = NULL;
    global_flag = true;
    load_flag = true;
}

VarDecl::VarDecl(Identifier *n, TypeQualifier *tq, Expr *e) : Decl(n) {
    Assert(n != NULL && tq != NULL);
    (typeq=tq)->SetParent(this);
    if (e) (assignTo=e)->SetParent(this);
    type = NULL;
    global_flag = true;
    load_flag = true;
}

VarDecl::VarDecl(Identifier *n, Type *t, TypeQualifier *tq, Expr *e) : Decl(n) {
    Assert(n != NULL && t != NULL && tq != NULL);
    (type=t)->SetParent(this);
    (typeq=tq)->SetParent(this);
    if (e) (assignTo=e)->SetParent(this);
    global_flag = true;
    load_flag = true;
}
  
void VarDecl::PrintChildren(int indentLevel) { 
   if (typeq) typeq->Print(indentLevel+1);
   if (type) type->Print(indentLevel+1);
   if (id) id->Print(indentLevel+1);
   if (assignTo) assignTo->Print(indentLevel+1, "(initializer) ");
}

void VarDecl::Emit() {
  bool isConst;
  bool isSet = false;

 
  if (typeq ==  NULL) {
    isConst = false;
  }
  else {
    if (!strcmp(typeq->getName(), "const")) {
      isConst = true;
    }
    else {
      isConst = false;
    }
  }
  if (global_flag) {
    val = Node::irgen.isInit(assignTo, type);
    if (val == NULL) {
      llvm::GlobalVariable *global = new llvm::GlobalVariable (*Node::irgen.GetOrCreateModule(NULL), 
				Node::irgen.getAstType(type),
 				isConst,
				llvm::GlobalVariable::ExternalLinkage, 
    				llvm::Constant::getNullValue(Node::irgen.getAstType(type)), 
				id->GetName());
       Node::irgen.GetOrCreateModule(NULL)->getOrInsertGlobal(id->GetName(), Node::irgen.getAstType(type));
       val = global;
    }
    else {
      llvm::GlobalVariable *global = new llvm::GlobalVariable (*Node::irgen.GetOrCreateModule(NULL),
                                Node::irgen.getAstType(type),
                                isConst,
                                llvm::GlobalVariable::ExternalLinkage,
				Node::irgen.isInit(assignTo,type), 
                                id->GetName());
	Node::irgen.GetOrCreateModule(NULL)->getOrInsertGlobal(id->GetName(), Node::irgen.getAstType(type));
	val = global;
    } 
  }
  else if (param_flag) {
    llvm::Function *f = Node::irgen.GetFunction();
    llvm::Argument *arg = NULL;
    for (llvm::Function::arg_iterator b = f->arg_begin(), e = f->arg_end(); b != e; ++b) {
      if (!strcmp(b->getName().str().c_str(), id->GetName())){
        arg = b;
      }
    }
    llvm::AllocaInst *local = new llvm::AllocaInst(Node::irgen.getAstType(type), id->GetName(), Node::irgen.GetBasicBlock());
   
    Assert(arg != NULL);
    llvm::StoreInst *initLocal = new llvm::StoreInst( arg, local, Node::irgen.GetBasicBlock());
    val = local;
  }
  else  {
     llvm::AllocaInst *local = new llvm::AllocaInst(Node::irgen.getAstType(type), id->GetName(), Node::irgen.GetBasicBlock());
    if (strcmp(GetType()->GetPrintNameForNode(), "ArrayType")) {
      if (assignTo != NULL) {
        llvm::StoreInst *initLocal = new llvm::StoreInst(assignTo->getExprVal(), local, Node::irgen.GetBasicBlock());
      }
      else {
        llvm::StoreInst *initLocal = new llvm::StoreInst( llvm::Constant::getNullValue(Node::irgen.getAstType(type)), local, Node::irgen.GetBasicBlock());
      }
    }
   val = local; 
  }
  
  Node::symtable->addDecl(this); 
}

void VarDecl::setGlobalFlag(bool tf) {
    global_flag = tf;
}

void VarDecl::setParamFlag(bool tf) {
    param_flag = tf;
}

bool VarDecl::getParamFlag() {
    return param_flag;
}

llvm::Value* VarDecl::getValue()
{
   llvm::LoadInst* load = new llvm::LoadInst(val, "", Node::irgen.GetBasicBlock());
   return load;
}

void VarDecl::setValue(llvm::Value* value)
{
   llvm::StoreInst *initLocal  = new llvm::StoreInst(value, val, Node::irgen.GetBasicBlock());
   load_flag = true;
}

FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
    returnTypeq = NULL;
}

FnDecl::FnDecl(Identifier *n, Type *r, TypeQualifier *rq, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r != NULL && rq != NULL&& d != NULL);
    (returnType=r)->SetParent(this);
    (returnTypeq=rq)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
}

void FnDecl::SetFunctionBody(Stmt *b) { 
    (body=b)->SetParent(this);
}

void FnDecl::PrintChildren(int indentLevel) {
    if (returnType) returnType->Print(indentLevel+1, "(return type) ");
    if (id) id->Print(indentLevel+1);
    if (formals) formals->PrintAll(indentLevel+1, "(formals) ");
    if (body) body->Print(indentLevel+1, "(body) ");
}

void FnDecl::Emit() {
  Node::symtable->addDecl(this);
  vector<llvm::Type *> argType;
  
  for (int i = 0; i < formals->NumElements(); ++i) {
    argType.push_back(Node::irgen.getAstType(formals->Nth(i)->GetType()));
    formals->Nth(i)->setGlobalFlag(false);
    formals->Nth(i)->setParamFlag(true);
  }
  llvm::ArrayRef<llvm::Type *> argArray(argType);
  llvm::FunctionType *ft = llvm::FunctionType::get(Node::irgen.getAstType(returnType),argArray, false);
  llvm::Function *f = llvm::cast<llvm::Function>(Node::irgen.GetOrCreateModule(NULL)->getOrInsertFunction(id->GetName(), ft));
  Node::irgen.SetFunction(f);
  llvm_fn = f;
  
  int i = 0;
  for (llvm::Function::arg_iterator b = f->arg_begin(), e = f->arg_end(); b != e; ++b) {
    b->setName(formals->Nth(i)->GetIdentifier()->GetName());
    ++i;
  }
  if (body != NULL) {
     Node::symtable->pushScope();
     llvm::LLVMContext *context = Node::irgen.GetContext();
     llvm::BasicBlock *bb = llvm::BasicBlock::Create(*context, "entry", f);
     Node::irgen.SetBasicBlock(bb);
  } 
 
  for (int i = 0; i < formals->NumElements(); ++i) {
    formals->Nth(i)->Emit();
    Node::symtable->addDecl(formals->Nth(i));
  }

  if (body != NULL) {
    StmtBlock *blk = dynamic_cast <StmtBlock*>(body);
    blk->setFun(f);
    blk->Emit();
  }
  if (Node::irgen.GetBasicBlock()->getTerminator() == NULL) {
    llvm::ReturnInst::Create(*Node::irgen.GetContext(), Node::irgen.GetBasicBlock());
  }
}
