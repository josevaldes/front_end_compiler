#ifndef _H_symtable
#define _H_symtable

#include <list>
#include <map>
#include <iostream>
#include "ast.h"
#include "ast_decl.h"
#include "irgen.h"

class SymbolTable
{
   private:
      list< std::map<std::string, Decl*>* > mystack;
      FnDecl* currentCall;
      VarDecl* array;
   public:
      SymbolTable();
      void pushScope();
      void popScope();
      void setValue(Identifier* id,llvm::Value* value); 
      void addDecl(Decl* decl);
      size_t lookup(Identifier* id);
      bool lookupCurr(Identifier *id);
      int getNumScopes();
      Decl *GetDecl(Identifier* id);
      void setCurrentCall(Identifier *id);
      FnDecl* getCurrentCall();
      void setArray(Identifier *id);
      VarDecl* getArray();
};

#endif
