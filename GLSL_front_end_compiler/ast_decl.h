/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp3: You will need to extend the Decl classes to implement
 * semantic processing including detection of declaration conflicts
 * and managing scoping issues.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "list.h"
#include "ast_expr.h"
#include "irgen.h"

class Type;
class TypeQualifier;
class NamedType;
class Identifier;
class Stmt;

void yyerror(const char *msg);

class Decl : public Node 
{
  protected:
    Identifier *id;
    llvm::Value *val;
  
  public:
    Decl() : id(NULL) {}
    Decl(Identifier *name);
    Identifier *GetIdentifier() const { return id; }
    friend ostream& operator<<(ostream& out, Decl *d) { return out << d->id; }

};

class VarDecl : public Decl 
{
  protected:
    Type *type;
    TypeQualifier *typeq;
    Expr *assignTo;
    bool global_flag;    
    bool param_flag;
    bool load_flag;
 
  public:
    VarDecl() : type(NULL), typeq(NULL), assignTo(NULL) {}
    VarDecl(Identifier *name, Type *type, Expr *assignTo = NULL);
    VarDecl(Identifier *name, TypeQualifier *typeq, Expr *assignTo = NULL);
    VarDecl(Identifier *name, Type *type, TypeQualifier *typeq, Expr *assignTo = NULL);
    const char *GetPrintNameForNode() { return "VarDecl"; }
    void PrintChildren(int indentLevel);
    Type *GetType() const { return type; }
    void Emit();
    void setGlobalFlag(bool tf);
    llvm::Value* getValue();
    void setValue(llvm::Value* value);
    void setParamFlag(bool tf);
    bool getParamFlag();
    llvm::Value* getVal() {return val;} 
};

class VarDeclError : public VarDecl
{
  public:
    VarDeclError() : VarDecl() { yyerror(this->GetPrintNameForNode()); };
    const char *GetPrintNameForNode() { return "VarDeclError"; }
};

class FnDecl : public Decl 
{
  protected:
    List<VarDecl*> *formals;
    Type *returnType;
    TypeQualifier *returnTypeq;
    Stmt *body;
    llvm::Function* llvm_fn;
    
  public:
    FnDecl() : Decl(), formals(NULL), returnType(NULL), returnTypeq(NULL), body(NULL) {}
    FnDecl(Identifier *name, Type *returnType, List<VarDecl*> *formals);
    FnDecl(Identifier *name, Type *returnType, TypeQualifier *returnTypeq, List<VarDecl*> *formals);
    void SetFunctionBody(Stmt *b);
    const char *GetPrintNameForNode() { return "FnDecl"; }
    void PrintChildren(int indentLevel);
    
    void Emit();
    Type *GetType() const { return returnType; }
    List<VarDecl*> *GetFormals() {return formals;}
    llvm::Function* GetFun() { Assert (llvm_fn != NULL); return llvm_fn; }
};

class FormalsError : public FnDecl
{
  public:
    FormalsError() : FnDecl() { yyerror(this->GetPrintNameForNode()); }
    const char *GetPrintNameForNode() { return "FormalsError"; }
};

#endif
