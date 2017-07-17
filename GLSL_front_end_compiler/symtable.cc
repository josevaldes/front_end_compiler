/*
 * Symbol table implementation
 *
 */
#include <list>
#include <map>
#include "symtable.h"
#include "ast.h"
#include "ast_decl.h"

SymbolTable::SymbolTable()
{
   map<string, Decl*>* first = new map<string, Decl*>;
   mystack.push_front(first);
   currentCall = NULL;
}

void SymbolTable::pushScope()
{
   mystack.push_front(new map<string, Decl*>);
}

void SymbolTable::popScope()
{
   mystack.pop_front();
}

void SymbolTable::addDecl(Decl* decl)
{
  string str = decl->GetIdentifier()->GetName();
  map<string, Decl*>* ite = mystack.front();
  (*ite)[str] = decl;
  
}

size_t SymbolTable::lookup(Identifier* id)
{
   string str = id->GetName();
   size_t found = 0;
   for(list<map<string, Decl*>*>::iterator it = mystack.begin(); it != mystack.end(); ++it)
   {
      found = (*it)->count(str);
      if(found == 1)
      {
         break;
      }
   }
   return found;;
}

bool SymbolTable::lookupCurr(Identifier *id)
{
   return (((mystack.front())->count(id->GetName())) == 1);
}

int SymbolTable::getNumScopes() 
{
   return mystack.size();
}

Decl *SymbolTable::GetDecl(Identifier *id) 
{
   size_t found;
   for(list<map<string, Decl*>*>::iterator it = mystack.begin(); it != mystack.end(); ++it)
   {
      found = (*it)->count(id->GetName());
      if(found == 1)
      {
         return (*it)->find(id->GetName())->second;    
      }
   }

   return NULL;
}

void SymbolTable::setValue(Identifier* id, llvm::Value* value)
{
   Decl* decl = GetDecl(id);
   VarDecl* var = dynamic_cast<VarDecl*>(decl);
   Assert(var != NULL); //Still need to consider functions
   var->setValue(value);
}

void SymbolTable::setCurrentCall(Identifier* id)
{
   currentCall = dynamic_cast<FnDecl*>(GetDecl(id));
   Assert(currentCall != NULL);
}

FnDecl* SymbolTable::getCurrentCall()
{
   Assert(currentCall != NULL);
   return currentCall;
}

void SymbolTable::setArray(Identifier* id) {
  array = dynamic_cast<VarDecl*>(GetDecl(id));
  Assert(array != NULL);
}

VarDecl* SymbolTable::getArray() {
  Assert(array != NULL);
  return array;
}
