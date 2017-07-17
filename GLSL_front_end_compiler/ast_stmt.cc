/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include <stack>
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "symtable.h"

#include "irgen.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/raw_ostream.h"                                                   


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::PrintChildren(int indentLevel) {
    decls->PrintAll(indentLevel+1);
    printf("\n");
}

void Program::Emit() {
    // TODO:
    // This is just a reference for you to get started
    //
    // You can use this as a template and create Emit() function
    // for individual node to fill in the module structure and instructions.
    //
    //IRGenerator irgen;
    llvm::Module *mod = Node::irgen.GetOrCreateModule("Name_The_Module.bc");
    
    for (int i = 0; i < decls->NumElements(); ++i) {
      decls->Nth(i)->Emit();
    }
     
    // write the BC into standard output
  //  mod->dump();
    llvm::WriteBitcodeToFile(mod, llvm::outs());
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
    fun = NULL;
    isBody = false;
    return_flag = false;
}

void StmtBlock::PrintChildren(int indentLevel) {
    decls->PrintAll(indentLevel+1);
    stmts->PrintAll(indentLevel+1);
}

void StmtBlock::setFun(llvm::Function *f) {
  fun = f;
}

void StmtBlock::setIsBody(bool flag)
{
   isBody = flag;
}

void StmtBlock::setReturnFlag(bool flag)
{
   return_flag = flag;
}

bool StmtBlock::getReturnFlag()
{
   return return_flag;
}

void StmtBlock::Emit() {
   llvm::BasicBlock* footer = NULL;
  // Flag needed to recognize this block as body  
  
   if(fun == NULL && isBody == false) {
       llvm::BasicBlock *bb = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "block", Node::irgen.GetFunction());
       llvm::BranchInst::Create(bb, Node::irgen.GetBasicBlock());

       Node::irgen.SetBasicBlock(bb);
       Node::symtable->pushScope();
   }

    bool isUnreachable = false;
    int i;
    int lastStmt = 0;
    for (i = 0; i < stmts->NumElements(); ++i) {
      
      stmts->Nth(i)->Emit();


      return_flag = stmts->Nth(i)->getReturnFlag();
      lastStmt = i;
      if(return_flag)
      {
         if(dynamic_cast<IfStmt*>(stmts->Nth(i)) != NULL)
         {
	    isUnreachable = true;
         }
	 break;
      }
    }
    
    if(stmts->NumElements() > 0 && dynamic_cast<ReturnStmt*>(stmts->Nth(lastStmt)) == NULL && fun != NULL && !isUnreachable)
    {
       if(fun->getReturnType()->getTypeID() == 0)
       {
          llvm::ReturnInst::Create(*Node::irgen.GetContext(), Node::irgen.GetBasicBlock());    
       }
       else
       {
          llvm::ReturnInst::Create(*Node::irgen.GetContext(), llvm::Constant::getNullValue(fun->getReturnType()), Node::irgen.GetBasicBlock());    
       }
    }

    if(fun == NULL && isBody == false)
    {
       if(!return_flag)
       {
          footer = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "footer", Node::irgen.GetFunction());
          llvm::BranchInst::Create(footer, Node::irgen.GetBasicBlock());
       }
       Node::irgen.SetBasicBlock(footer);
    }
   

    Node::symtable->popScope();
}

DeclStmt::DeclStmt(Decl *d) {
    Assert(d != NULL);
    (decl=d)->SetParent(this);
}

void DeclStmt::PrintChildren(int indentLevel) {
    decl->Print(indentLevel+1);
}

void DeclStmt::Emit() {
  VarDecl* vd = dynamic_cast <VarDecl*>(decl); 
  vd->setGlobalFlag(false);
  vd->Emit();
}
ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && b != NULL);
    (init=i)->SetParent(this);
    step = s;
    if ( s )
      (step=s)->SetParent(this);
}

void ForStmt::PrintChildren(int indentLevel) {
    init->Print(indentLevel+1, "(init) ");
    test->Print(indentLevel+1, "(test) ");
    if ( step )
      step->Print(indentLevel+1, "(step) ");
    body->Print(indentLevel+1, "(body) ");
}

void WhileStmt::PrintChildren(int indentLevel) {
    test->Print(indentLevel+1, "(test) ");
    body->Print(indentLevel+1, "(body) ");
}


void ForStmt::Emit()
{
  init->getExprVal();
  llvm::BasicBlock *cmpBlock = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "cmpBlock", Node::irgen.GetFunction());
  llvm::BasicBlock *incBlock = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "incBlock", Node::irgen.GetFunction());  
  llvm::BasicBlock *forBlock = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "forBlock", Node::irgen.GetFunction());
  llvm::BasicBlock *footer   = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "footer"  , Node::irgen.GetFunction());
  
  Node::irgen.SetFooterBB(footer);
  Node::irgen.SetContinueBB(incBlock);
  
  llvm::BranchInst::Create(cmpBlock, Node::irgen.GetBasicBlock());
  Node::irgen.SetBasicBlock(cmpBlock);

  llvm::Value *cmp = test->getExprVal();
  llvm::BranchInst::Create(forBlock, footer, cmp, Node::irgen.GetBasicBlock());

  Node::irgen.SetBasicBlock(forBlock);
  Node::symtable->pushScope();
  body->setIsBody(true);
  body->Emit();

  if(!body->getReturnFlag())
  {
     llvm::BranchInst::Create(incBlock, Node::irgen.GetBasicBlock());
  }
  
  Node::irgen.SetBasicBlock(incBlock);
  step->getExprVal();
  llvm::BranchInst::Create(cmpBlock, Node::irgen.GetBasicBlock());


  Node::irgen.SetBasicBlock(footer);
  Node::irgen.popContinueBB();
  Node::irgen.popFooterBB();
  if(dynamic_cast<StmtBlock*>(body) == NULL)
  {
     Node::symtable->popScope();
  }

}


void WhileStmt::Emit()
{
  llvm::BasicBlock *header = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "header", Node::irgen.GetFunction());
  llvm::BasicBlock *whileBlock   = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "whileBlock"  , Node::irgen.GetFunction());
  llvm::BasicBlock *footer = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "footer", Node::irgen.GetFunction());
 
  Node::irgen.SetContinueBB(header);
  Node::irgen.SetFooterBB(footer);

  llvm::BranchInst::Create(header, Node::irgen.GetBasicBlock());
  Node::irgen.SetBasicBlock(header);
  llvm::Value* val = test->getExprVal();


  llvm::BranchInst::Create(whileBlock, footer, val, Node::irgen.GetBasicBlock());
  Node::irgen.SetBasicBlock(whileBlock);
  Node::symtable->pushScope();
  body->setIsBody(true);
  body->Emit();
  if(!body->getReturnFlag())
  {
     llvm::BranchInst::Create(header, Node::irgen.GetBasicBlock());
  }
  
  Node::irgen.SetBasicBlock(footer);
  Node::irgen.popContinueBB();
  Node::irgen.popFooterBB();
  
  if(dynamic_cast<StmtBlock*>(body) == NULL)
  {
     Node::symtable->popScope();
  }
}


IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::PrintChildren(int indentLevel) {
    if (test) test->Print(indentLevel+1, "(test) ");
    if (body) body->Print(indentLevel+1, "(then) ");
    if (elseBody) elseBody->Print(indentLevel+1, "(else) ");
}

void IfStmt::Emit()
{
   llvm::Value* val = test->getExprVal();
   llvm::BasicBlock* footer = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "footer", Node::irgen.GetFunction());
   llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "thenBB", Node::irgen.GetFunction());
   llvm::BasicBlock* elseBB = NULL;

   if(elseBody != NULL)
   {
      elseBB = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "elseBB", Node::irgen.GetFunction());
   }
   
   llvm::BranchInst::Create(thenBB, elseBody ? elseBB : footer, val, Node::irgen.GetBasicBlock());
   
   Node::irgen.SetBasicBlock(thenBB);
   Node::symtable->pushScope();
   body->setIsBody(true);
   body->Emit();
  
   if(dynamic_cast<StmtBlock*>(body) == NULL)
   {
     Node::symtable->popScope();
   }

   if(!body->getReturnFlag() && !Node::irgen.getUnreachFlag())
   {
      llvm::BranchInst::Create(footer, Node::irgen.GetBasicBlock());
   }  
   
   Node::irgen.SetBasicBlock(footer);
    
   if(elseBody != NULL)
   {
      Node::irgen.SetBasicBlock(elseBB);
      StmtBlock* currStmtBlock = dynamic_cast<StmtBlock*>(elseBody);
      Node::symtable->pushScope();
      
      Assert(elseBB != NULL);
      elseBody->setIsBody(true);
      elseBody->Emit();
      if(!elseBody->getReturnFlag()  && !Node::irgen.getUnreachFlag())
      {
         llvm::BranchInst::Create(footer, Node::irgen.GetBasicBlock());
      }
      
      Node::irgen.SetBasicBlock(footer);

      if(body->getReturnFlag()  && elseBody->getReturnFlag())
      {         
	 llvm::UnreachableInst* ui = new llvm::UnreachableInst(*Node::irgen.GetContext(), Node::irgen.GetBasicBlock());
         Node::irgen.setUnreachFlag(true);
	 return_flag = true;
      }
      if(dynamic_cast<StmtBlock*>(body) == NULL)
      {
        Node::symtable->popScope();
       }
   }
}
ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    expr = e;
    return_flag = true;
    if (e != NULL) expr->SetParent(this);
}

void ReturnStmt::PrintChildren(int indentLevel) {
    if ( expr ) 
      expr->Print(indentLevel+1);
}

void ReturnStmt::Emit()
{
    if (expr != NULL) {
      llvm::Value *val = expr->getExprVal();
      llvm::ReturnInst::Create(*Node::irgen.GetContext(), val, Node::irgen.GetBasicBlock());
    }
    else {
      llvm::ReturnInst::Create(*Node::irgen.GetContext(), Node::irgen.GetBasicBlock()); 
    }
}

void BreakStmt::Emit()
{
   llvm::BranchInst::Create(Node::irgen.GetFooterBB(), Node::irgen.GetBasicBlock());
}

SwitchLabel::SwitchLabel(Expr *l, Stmt *s) {
    Assert(l != NULL && s != NULL);
    (label=l)->SetParent(this);
    (stmt=s)->SetParent(this);
}

SwitchLabel::SwitchLabel(Stmt *s) {
    Assert(s != NULL);
    label = NULL;
    (stmt=s)->SetParent(this);
}

void ContinueStmt::Emit()
{
//   llvm::BasicBlock* contBB = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "contBB", Node::irgen.GetFunction());
//   llvm::BasicBlock* footBB = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "footBB", Node::irgen.GetFunction());
//   llvm::BranchInst::Create(contBB, Node::irgen.GetBasicBlock());
//   Node::irgen.SetBasicBlock(contBB);
   llvm::BranchInst::Create(Node::irgen.GetContinueBB(), Node::irgen.GetBasicBlock());
//   Node::irgen.SetBasicBlock(footBB);
}

void SwitchLabel::PrintChildren(int indentLevel) {
    if (label) label->Print(indentLevel+1);
    if (stmt)  stmt->Print(indentLevel+1);
}

void Case::Emit()
{
   llvm::BasicBlock* caseBB = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "case", Node::irgen.GetFunction());
   llvm::ConstantInt *caseNum = llvm::cast<llvm::ConstantInt>(label->getExprVal());
   
   Node::irgen.GetSwitchInst()->addCase(caseNum, caseBB);

   if(!return_flag)
   {
      llvm::BranchInst::Create(caseBB, Node::irgen.GetBasicBlock());    
   }

   Node::irgen.SetBasicBlock(caseBB);
   stmt->Emit();
   return_flag = stmt->getReturnFlag();
}

void Default::Emit()
{
   if(!return_flag)
   {
      llvm::BranchInst::Create(Node::irgen.GetDefaultBB(), Node::irgen.GetBasicBlock());
   }

   Node::irgen.SetBasicBlock(Node::irgen.GetDefaultBB());
   stmt->Emit();
   return_flag = stmt->getReturnFlag();
}

Expr* Case::getExpr()
{
   return label;
}

SwitchStmt::SwitchStmt(Expr *e, List<Stmt *> *c, Default *d) {
    Assert(e != NULL && c != NULL && c->NumElements() != 0 );
    (expr=e)->SetParent(this);
    (cases=c)->SetParentAll(this);
    def = d;
    if (def) def->SetParent(this);
}
void SwitchStmt::Emit()
{
   Node::symtable->pushScope();
   llvm::Value* val = expr->getExprVal();
   llvm::BasicBlock* footer = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "footer", Node::irgen.GetFunction());
   Node::irgen.SetFooterBB(footer);
   
   List<Case*> true_cases;
   List<llvm::BasicBlock*> casesBB;
   unsigned int numCases = 0;
   for(int i = 0; i < cases->NumElements(); ++i)
   {
      Case* this_case   = dynamic_cast<Case*>(cases->Nth(i));
      Default* this_def = dynamic_cast<Default*>(cases->Nth(i));

      if(this_case != NULL || this_def != NULL)
      {
         ++numCases;
      }
   }

   llvm::BasicBlock* defaultBB = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "default", Node::irgen.GetFunction());
   Node::irgen.SetDefaultBB(defaultBB);
   llvm::SwitchInst* this_switch = llvm::SwitchInst::Create(val, defaultBB, numCases, Node::irgen.GetBasicBlock()); 
   Node::irgen.SetSwitchInst(this_switch);

   int j;
   bool isBreak = true; 
   for(j = 0; j < cases->NumElements(); ++j)
   {
      Case* this_case = dynamic_cast<Case*>(cases->Nth(j));
      Default* this_def = dynamic_cast<Default*>(cases->Nth(j));

      if(this_case != NULL)
      {
         /*
	 llvm::BasicBlock* caseBB = llvm::BasicBlock::Create(*Node::irgen.GetContext(), "case", Node::irgen.GetFunction());
         llvm::ConstantInt *caseNum = llvm::cast<llvm::ConstantInt>(this_case->getExpr()->getExprVal());
         this_switch->addCase(caseNum, caseBB);
	 if(!isBreak && j != 0)
	 {
            llvm::BranchInst::Create(caseBB, Node::irgen.GetBasicBlock());
	 }

         Node::irgen.SetBasicBlock(caseBB);
         */
	 this_case->setReturnFlag(isBreak);
	 this_case->Emit();
	 isBreak = this_case->getReturnFlag();
      }
      else if (this_def != NULL)
      {   
	 this_def->setReturnFlag(isBreak);
	 this_def->Emit();
	 isBreak = this_def->getReturnFlag();
      }
      else
      {
         cases->Nth(j)->Emit();
	 isBreak = cases->Nth(j)->getReturnFlag(); 
      }
   }
   
   if(!isBreak)
   {
      llvm::BranchInst::Create(footer, Node::irgen.GetBasicBlock());
   }

   if(defaultBB->getTerminator() == NULL)
   {
      Node::irgen.SetBasicBlock(defaultBB);
      llvm::BranchInst::Create(footer, Node::irgen.GetBasicBlock());
   }
   
   Node::irgen.popSwitchInst();
   Node::irgen.popFooterBB();
   Node::irgen.SetBasicBlock(footer);
   Node::symtable->popScope();
}
void SwitchStmt::PrintChildren(int indentLevel) {
    if (expr) expr->Print(indentLevel+1);
    if (cases) cases->PrintAll(indentLevel+1);
    if (def) def->Print(indentLevel+1);
}

