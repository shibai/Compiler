

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"
#include <map>

extern int semant_debug;
extern char *curr_filename;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}

static SymbolTable <Symbol, Symbol> *methodTable;
static SymbolTable <Symbol, Symbol> *attrTable;
static ClassTable *classtable;

/*
 * Inheritance graph check 
 * Using map<> to map child-parent relationship
 */

ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {

    /* Fill this in */ 
 
  std::map<Symbol,Class_>::iterator itr;
  install_basic_classes();
  bool mainExist = false;
  for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
    Class_ child = classes->nth(i);
    Symbol name = child->get_name();
    Symbol parent = child->get_parent();

    if (name == parent) {
      semant_error(classes->nth(i))<<"child's name is the same as the parent's \n";
      return;
    }
    // to see if the child is already defined
    itr = graph.find(name);
    
    if (parent == Bool || parent == Str || parent == SELF_TYPE || parent == Int) {
      semant_error(child)<<"cannot inherits from "<<parent<<endl;
      return;
    }
    if (name == SELF_TYPE) {
      semant_error(child)<<"Redefinition of basci class SELF_TYPE"<<endl;
    }
    
    if (itr == graph.end()) {
      graph[name] = child;
    }else {
      semant_error(child)<<"child is already defined previously \n";
      return;
    }
    if (name == Main) {
      mainExist = true;
    }
  }
  
  if (!mainExist) {
    semant_error()<<"Class Main is not defined."<<endl;
  }
  // if(semant_errors > 0) return;
  // check for parent class and cycles
  
  std::map<Symbol,Class_>::iterator itr_p;
  for (itr = graph.begin();itr != graph.end();itr++) {
    Symbol name = itr->first;
    Symbol parent = itr->second->get_parent();
    
    itr_p = graph.find(parent);
    if (parent != No_class && itr_p == graph.end()) {
      semant_error(itr->second)<<"parent class is not defined \n";
    }
    if (itr_p != graph.end() && itr_p->second->get_parent() == name) {
      semant_error(itr->second)<<"found a cycle \n";
    }   
    
  }
  
}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);  

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);

    // add basic classes to graph
    graph[Object] = Object_class;
    graph[IO] = IO_class;
    graph[Int] = Int_class;
    graph[Bool] = Bool_class;
    graph[Str] = Str_class;
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 


// Based on the project requirements, any extra cool-tree class definitions are defined here 

// for report errers
// call sement_error(Class_) function within classtable
static void errReport (Symbol clas, char* msg) {
  std::map<Symbol,Class_> graph = classtable->getGraph();
  std::map<Symbol,Class_>::iterator itr = graph.find(clas);
  classtable->semant_error(itr->second)<<msg<<endl;
}

// class conformation check
// check if child is a subclass of parent
static bool classInheritanceCheck (Symbol parent, Symbol child) {
  if (parent == child) {
    return true;
  }
  std::map<Symbol,Class_> graph = classtable->getGraph();
  std::map<Symbol,Class_>::iterator itr;
  while (child != No_class) {
    itr = graph.find(child);
    child = itr->second->get_parent();
    if (parent == child) {
      return true;
    } 
  }
  return false;
}

method_class* findMethod (Class_ clas, Symbol name) {
  Features fs = clas->get_features();
  Feature f;
  for (int i = fs->first(); fs->more(i); i = fs->next(i)) {
    f = fs->nth(i);
    if (f->get_name() == name) {
      method_class* m = (method_class *)f;
      return m; 
    }
  }
  return NULL;
}

// find method in current and ancestors' class
method_class* findAllMethod (Class_ clas, Symbol name) {
  method_class* meth = findMethod(clas,name);
  while (meth == NULL) {
    Symbol parent = clas->get_parent();
    std::map<Symbol,Class_> graph = classtable->getGraph();
    if (graph.find(parent) == graph.end()) {
      return NULL;
    }
    clas = graph[parent];
    meth = findMethod(clas,name);
  }
  return meth;
}

// walk towards the root - Object node
// collect all nodes that forms the path
static void getPath (Symbol s, Symbol list[],int& length) {
  std::map<Symbol,Class_> graph = classtable->getGraph();
  std::map<Symbol,Class_>::iterator itr;
  list[0] = s;
  while (s != Object) {
    s = graph[s]->get_parent();
    list[length] = s; 
    length++;
  }
  length--;
}
// find the lowerest common ancestor of two classes
static Symbol leastType (Symbol s1, Symbol s2) {
  Symbol s1_list[256];
  Symbol s2_list[256];
  int length1 = 1;
  int length2 = 1;
  getPath(s1,s1_list,length1);
  getPath(s2,s2_list,length2);
  while (length1 >= 0 && length2 >= 0) {
    if (s1_list[length1] != s2_list[length2]) {
      break;
    }
    length1--;
    length2--;
  }
  return s1_list[length1+1];
}

// object_class
Symbol object_class::exprCheck (Symbol clas) {
  if (name == self) {  // self, return current class
    type = SELF_TYPE;
    return type;
  }
  Symbol* return_t = attrTable->lookup(name);
  if (return_t == NULL) {
    char* msg = "in object: id is not declared";
    errReport(clas,msg);
    type = Object;
  }else {
    type = *return_t;
  } 
  return type;
}

// assign exprCheck()
Symbol assign_class::exprCheck (Symbol clas) {
  Symbol* type1 = attrTable->lookup(name);
  Symbol type2;
  if (attrTable->lookup(name) == NULL) {
    char* msg = "Identifier is not declared";
    errReport(clas,msg);
    type = Object;
  }else {
    type2 = expr->exprCheck(clas);
    if (classInheritanceCheck(*type1,type2)) {
       type = type2;
    }else {
      char* msg = "type inheritance issue in assign";
      errReport(clas,msg);
      type = Object;
    }
  }
  return type;
}
// int
Symbol int_const_class::exprCheck (Symbol clas) {
  type = Int;
  return type;
}
// bool
Symbol bool_const_class::exprCheck (Symbol clas) {
  type = Bool;
  return Bool;
}
// string
Symbol string_const_class::exprCheck (Symbol clas) {
  type = Str;
  return Str;
}
// no_expr
Symbol no_expr_class::exprCheck (Symbol clas) {
  type = No_type;
  return No_type;
}
// new
Symbol new__class::exprCheck (Symbol clas) {
  if (type_name == SELF_TYPE) {
    type = SELF_TYPE;
  }else {
    std::map<Symbol,Class_> graph = classtable->getGraph();
    std::map<Symbol,Class_>::iterator itr = graph.find(type_name);
    if (itr != graph.end()) {
      type = type_name;
    }else {
      char* msg = "in new: type class not defined";
      errReport(clas,msg);
      type = Object;
    }
  }
  return type;
}

/*
 * dispatch, read manual
 * Format: <expr>.id(<expr>,...,<expr>)
 * Example: e0.id(e1,...,en)
 * 1) evaluate e0, SELF_TYPE or Object ID
 * 2) evaluate f method
 *   a) number of parameters
 *   b) parameters types check
 *   c) f return type check
 * 3) the return type is the same as f method,
 *    unless it's self_type, then return e0's type
 */
Symbol dispatch_class::exprCheck (Symbol clas) {
  
  std::map<Symbol,Class_> graph = classtable->getGraph();
  std::map<Symbol,Class_>::iterator itr;
  method_class* reference;
  Symbol e0_type = expr->exprCheck(clas);
  if (e0_type == SELF_TYPE) {
    reference = findAllMethod(graph[clas],name);
    // e0_type = clas;
  }else {
    itr = graph.find(e0_type);
    if (itr == graph.end()) {
      char* msg = "in dispatch: class has not been defined";
      errReport(clas,msg);
      type = Object;
      return type;
    }else {
      reference = findAllMethod(itr->second, name);
    }
  }
  if (reference == NULL) {
    char* msg = "dispatch to undefined method";
    //cout<<name<<endl;
    errReport(clas,msg);
    type = Object;
    return type;
  }  

  int num1 = actual->len();
  int num2 = reference->get_formals()->len();
  if (num1 != num2) {
    char* msg = "in dispatch: numbers of augments not match";
    errReport(clas,msg);
  }  

  Formals formals_ref = reference->get_formals();
  for (int i = formals_ref->first(); formals_ref->more(i) ; i = formals_ref->next(i)) {
    Symbol p_Type = actual->nth(i)->exprCheck(clas);
    Symbol type_ref = formals_ref->nth(i)->get_type();
    if (p_Type == SELF_TYPE) {
      p_Type = clas;
    }
    if (!classInheritanceCheck(type_ref,p_Type)) {
      char* msg = "in dispatch: formal type not MATCH!!!";
      errReport(clas,msg);
      type =  Object;
      return type;
    }
  }
  
  type = reference->get_return();
  if (type == SELF_TYPE) {
    type = e0_type;
    return type;
  }
  return type;
}

/* 
 * static_dispatch expreCheck()
 * ----------------------------------------------
 * cool manual - page 10.
 * Format: <expr>@<type>.id(<expr>,...,<expr>)
 * Example: e0@T.f(e1,...,en)
 * 1) method f(e1,...,en) check 
 *   a) number of parameters
 *   b) parameters types check
 *   c) f return type check
 * 2) evalue e0, e0 must conform to the type T 
 * 3) the return type is the same as f method,
 *    unless it's self_type, then return e0's type
 */ 
Symbol static_dispatch_class::exprCheck (Symbol clas) {
  std::map<Symbol,Class_> graph = classtable->getGraph();
  std::map<Symbol,Class_>::iterator itr = graph.find(type_name);
  method_class* reference;

  if (itr == graph.end()) {
    char* msg = "in static_dispatch: class has not been defined";
    errReport(clas,msg);
    type = Object;
    return type;
  }else {
    reference = findAllMethod(itr->second, name);
  }
  
  int num1 = actual->len();
  int num2 = reference->get_formals()->len();
  if (num1 != num2) {
    char* msg = "in static_dispatch: numbers of augments not match";
    errReport(clas,msg);
    type = Object;
    return type;
  }  
 
  Formals formals_ref = reference->get_formals();
  for (int i = formals_ref->first(); formals_ref->more(i) ; i = formals_ref->next(i)) {
    Symbol p_Type = actual->nth(i)->exprCheck(clas);
    Symbol type_ref = formals_ref->nth(i)->get_type();
    if (!classInheritanceCheck(type_ref,p_Type)) {
      char* msg = "in static_dispatch: formal type not MATCH!!!";
      errReport(clas,msg);
      type = Object;
      return type;
    }
  }

  // e0 type check
  Symbol e0_type = expr->exprCheck(clas);
  if (e0_type == SELF_TYPE) {
    e0_type = clas;
  }
  if (!classInheritanceCheck(type_name,e0_type)) {
    char* msg = "type inheritance issue";
    errReport(clas,msg);
    type = Object;
    return type;
  }
 
  type = reference->get_expr()->exprCheck(type_name);
  if (type == SELF_TYPE) {
    type = e0_type;
  }
  
  return type;  
}

/* if-then condition
 * -------------------------------
 * Read manual page 10
 * 1) Check predicate
 * 2) Check both then_branch and else_branch 
 * The return type of if_cond is the least type of those two branchs
 */
Symbol cond_class::exprCheck (Symbol clas) {
  Symbol condi = pred->exprCheck(clas);
  if (condi != Bool) {
    char* msg = "it's not a boolean";
    errReport(clas,msg);
    type = Object;
    return type;
  }
  Symbol then_branch = then_exp->exprCheck(clas);
  Symbol else_branch = else_exp->exprCheck(clas);
  type = leastType(then_branch,else_branch);
  return type;
}

// block_class
Symbol block_class::exprCheck (Symbol clas) {
  int i;
  for (i = body->first(); body->more(i); i = body->next(i)) {
    body->nth(i)->exprCheck(clas);
  }
  type = body->nth(i-1)->get_type();
  return type;
}

/* let_class, manual page 11
 * -------------------------------------
 * 1) check each initialization without identifier
 * 2) check body
 * Note: self is not allowed as identifiers,
 *       but SELF_TYPE is allowed and not translated to actual type
 */
Symbol let_class::exprCheck (Symbol clas) {
  if (identifier == self) {
    char* msg = "cannot bind self in let";
    errReport(clas,msg);
    type = Object;
    return type;
  }
  Symbol initial = init->exprCheck(clas);
  attrTable->enterscope();
  attrTable->addid(identifier,new Symbol(type_decl));
  if ( initial != No_type && type_decl != SELF_TYPE) {
    if (!classInheritanceCheck(type_decl,initial)) {
      char* msg = "type conformation issue in let";
      errReport(clas,msg);
      type = Object;
      return type;
    }
  }
  //evaluate the body
  type = body->exprCheck(clas);
  attrTable->exitscope();
  return type;
}

/* typcase_class
 * -------------------------------
 * 1) denote the case_expr
 * 2) among all branches, find the least type <exprk>
 *   - in each branch, check conformation between expr type and type_decl
 * 3) the results of case is the value of <exprk>
 * Note:  each branch shall have distinct type
 * using map<type_name,id_name>
 */
Symbol typcase_class::exprCheck (Symbol clas) {
  expr->exprCheck(clas);
  type = NULL;
  std::map<Symbol,bool> mapping; 
  std::map<Symbol,bool>::iterator itr;
  std::map<Symbol,Class_> graph = classtable->getGraph();
  for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
    Case c = cases->nth(i);
    Symbol branch_t = c->get_type_decl();
    // check declared type
    if (graph.find(branch_t) == graph.end()) {
      char* msg = "class type not defined";
      errReport(clas,msg);
      type = Object;
      return type;
    }
    // check for duplicates in branchse
    itr = mapping.find(branch_t);
    if (mapping.find(branch_t) != mapping.end()) {
      char* msg = "in typcase: duplicate branch type";
      errReport(clas,msg);
      type = Object;
      return type;
    }
    mapping[branch_t] = true; // doesn't matter what the value is
    attrTable->addid(c->get_name(),new Symbol(branch_t)); // store static type
    Symbol expr_t = c->get_expr()->exprCheck(clas);
    if (!classInheritanceCheck(branch_t,expr_t)) {
      char* msg = "in branch: type not match";
      errReport(clas,msg);
      type = Object;
      return type;
    }

    if (type != NULL) {
      type = leastType(branch_t,type);
    }else {
      type = branch_t;
    }
  }
  return type;
}

// loop_class
Symbol loop_class::exprCheck (Symbol clas) {
  if (pred->exprCheck(clas) != Bool) {
    char* msg = "in loop: predicate is not type of boolean";
    errReport(clas,msg);
  }
  body->exprCheck(clas);
  type = Object;
  return type;
}

// isvoid_class
Symbol isvoid_class::exprCheck (Symbol clas) {
  e1->exprCheck(clas);
  type = Bool;
  return type;
}

// comp_class,  Symbol: 'Not'
Symbol comp_class::exprCheck (Symbol clas) {
  if (e1->exprCheck(clas) != Bool) {
    char* msg = "in neg: expression is not type of boolean";
    errReport(clas,msg);
    type = Object;
    return type;
  }
  type = Bool;
  return type;
}

// lt_class
// both e1 and e2 have to be Int
Symbol lt_class::exprCheck (Symbol clas) {
  if (e1->exprCheck(clas) != Int || e2->exprCheck(clas) != Int) {
    char* msg = "in lt: must be Int";
    errReport(clas,msg);
    type = Object;
  }else {
    type = Bool;
  }
  return type;
}

// leq_class
// both e1 and e2 have to be Int
Symbol leq_class::exprCheck (Symbol clas) {
  if (e1->exprCheck(clas) != Int || e2->exprCheck(clas) != Int) {
    char* msg = "in leq: must be Int";
    errReport(clas,msg);
    type = Object;
  }else {
    type = Bool;
  }
  return type;
}

// neg_class, Symbol: '~'
// e1 has to be Int
Symbol neg_class::exprCheck (Symbol clas) {
  if (e1->exprCheck(clas) != Int) {
    char* msg = "in neg: must be Int";
    errReport(clas,msg);
    type = Object;
  }else {
    type = Int;
  }
  return type;
}

// plus_class
Symbol plus_class::exprCheck (Symbol clas) {
  if (e1->exprCheck(clas) != Int || e2->exprCheck(clas) != Int) {
    char* msg = "in neg: must be Int";
    errReport(clas,msg);
    type = Object;
  }else {
    type = Int;
  }
  return type;
}

// sub_class
Symbol sub_class::exprCheck (Symbol clas) {
  if (e1->exprCheck(clas) != Int || e2->exprCheck(clas) != Int) {
    char* msg = "in neg: must be Int";
    errReport(clas,msg);
    type = Object;
  }else {
    type = Int;
  }
  return type;
}
// mul_class
Symbol mul_class::exprCheck (Symbol clas) {
  if (e1->exprCheck(clas) != Int || e2->exprCheck(clas) != Int) {
    char* msg = "in neg: must be Int";
    errReport(clas,msg);
    type = Object;
  }else {
    type = Int;
  }
  return type;
}
// divide_class
Symbol divide_class::exprCheck (Symbol clas) {
  if (e1->exprCheck(clas) != Int || e2->exprCheck(clas) != Int) {
    char* msg = "in neg: must be Int";
    errReport(clas,msg);
    type = Object;
  }else {
    type = Int;
  }
  return type;
}

/* eq_class
 * ------------------
 * if either e1 or e2 has static type Int, Bool or String,
 * then the other must have the same
 */ 
Symbol eq_class::exprCheck (Symbol clas) {
  Symbol e1_t = e1->exprCheck(clas);
  Symbol e2_t = e2->exprCheck(clas);
  if (((e1_t == Int || e2_t == Int)
      || (e1_t == Bool || e2_t == Bool)
       || (e1_t == Str || e2_t == Str)) && e1_t != e2_t) {
    char* msg = "in eq: type not match";
    errReport(clas,msg);
    type = Object;
  }else {
    type = Bool;
  }
  return type;
}

// method_class::addToTable
// methods can be re-defined in the sub-classes, read manual page 8
void method_class::addToTable (Symbol clas) {
  bool flag = false; // for re-definition check
  if (methodTable->probe(name) != NULL) {
    char* msg = "method has been declared in current scope";
    errReport(clas,msg);
  }else if (methodTable->lookup(name) != NULL) { // method overriding
    Symbol name_inher = *(methodTable->lookup(name));
    std::map<Symbol,Class_> graph = classtable->getGraph();
    Class_ clas_inher = graph[name_inher];
    method_class* reference = findMethod(clas_inher,name);

    int num1 = formals->len();
    int num2 = reference->get_formals()->len();
    if (num1 != num2) {
      char* msg = "method re-definition: numbers of augments not match";
      errReport(clas,msg);
    }else {  
      Formals formals_ref = reference->get_formals();
      for (int i = formals_ref->first(); formals_ref->more(i) ; i = formals_ref->next(i)) {
	Symbol p_Type = formals->nth(i)->get_type();
	Symbol type_ref = formals_ref->nth(i)->get_type();
	if (type_ref != p_Type) {
	  char* msg = "method re-definition: formal type not MATCH!!!";
	  errReport(clas,msg);
	  flag = true;
	}
      }
      if (return_type != reference->get_return()) {
	char* msg = "method re-definition: return type not match";
	errReport(clas,msg);
	flag = true;
      }
      if (!flag) methodTable->addid(name,new Symbol(clas));
    }
  }else { // new method
    methodTable->addid(name,new Symbol(clas));
  }    
}

// attr_class::addToTable
// attribute cannot be re-defined
void attr_class::addToTable (Symbol clas) {
  if (name == self) {
    char* msg = "self cannot be the type of attribute";
    errReport(clas,msg);
  }else if (attrTable->probe(name) != NULL) {
    char* msg = "attribute has already been declared in current scope";
    errReport(clas,msg);
  }else if (attrTable->lookup(name)) {
    char* msg = "redefined attribute";
    errReport(clas,msg);
  }else if (type_decl == SELF_TYPE) {
    attrTable->addid(name,new Symbol(clas));
  }else {
    attrTable->addid(name,new Symbol(type_decl));
  }
  
}
/* method_class
 * ----------------------------
 * 1) check the formals
 * 2) check return type, if it's SELF_TYPE, assign the current class
 * 3) the type of the method body must conform to the return type
 * Note: self is not allowed as a formal name,
 *       SELF_TYPE is not allowed in a formal type
 *
 */
void method_class::featureCheck (Symbol clas) {
  for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
    // formal check
    Formal f = formals->nth(i);
    if (f->get_name() == self) {
      char* msg = "self is not allowed as a formal name";
      errReport(clas,msg);
    }else if (f->get_type() == SELF_TYPE) {
      char* msg = "Formal parameter cannot have type SELF_TYPE";
      errReport(clas,msg);
    }else if (attrTable->probe(f->get_name()) != NULL) { // check current scope
      char* msg = "formal multiply defined";
      errReport(clas,msg);
    }else {
      attrTable->addid(f->get_name(),new Symbol(f->get_type()));
    }
  }
  Symbol dynamic_t = return_type;
  Symbol expr_t = expr->exprCheck(clas);

  // self only conforms to SELF_TYPE
  if (return_type == SELF_TYPE && expr_t != return_type) {
    char* msg = "self should conforms to SELF_TYPE";
    errReport(clas,msg);
  }

  if (return_type == SELF_TYPE) {
    dynamic_t = clas;
  }
  if (expr_t == SELF_TYPE) {
    expr_t = clas;
  }
  if (!classInheritanceCheck(dynamic_t,expr_t)) {
    char* msg = "method: types not match";
    errReport(clas,msg);
  } 
}

void attr_class::featureCheck (Symbol clas) {
  // No need to check attribute multi-declaration since it's been taken care of in the 1st pass 

  // expr check  
  Symbol return_t = init->exprCheck(clas);
  std::map<Symbol,Class_> graph = classtable->getGraph();
  Symbol tmp;
  if (type_decl == SELF_TYPE) {
    tmp = clas;
  }else {
    tmp = type_decl;
  }
  if (graph.find(tmp) == graph.end()) {
    char* msg = "in attr: type is not declared";
    errReport(clas,msg);
  }
  if (return_t == SELF_TYPE) {
    return_t = clas;
  }
  if (return_t != No_type && !classInheritanceCheck(tmp,return_t)) {
    char *msg = "in attr: type not match";
    errReport(clas,msg);
  }
}

void climb (Class_ cla) {
  Symbol parent = cla->get_parent();
  if (parent != No_class) {
    std::map<Symbol,Class_> graph = classtable->getGraph();
    std::map<Symbol,Class_>::iterator itr = graph.find(parent); 
    climb(itr->second); 
  }
  methodTable->enterscope();
  attrTable->enterscope();
  Features fs = cla->get_features();
  for (int i = fs->first(); fs->more(i); i = fs->next(i)) {
    fs->nth(i)->addToTable(cla->get_name());
  }
}


/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */
    classtable = new ClassTable(classes);

    if (classtable->errors()) {
	cerr << "Compilation halted due to static semantic errors." << endl;
	exit(1);
    }
    /* some semantic analysis code may go here */
    
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
      // for each class, set up a new pair of symbol tables
      // 1st pass: add methods and attributes from current and parent classes
      methodTable = new SymbolTable <Symbol, Symbol>();
      attrTable = new SymbolTable<Symbol,Symbol>();
      Class_ cla = classes->nth(i);
      Features fs = cla->get_features();
      climb(cla);
      // 2nd pass
      for (int j = fs->first(); fs->more(j); j = fs->next(j)) {
	methodTable->enterscope(); // enter a new method scope
	attrTable->enterscope();
	fs->nth(j)->featureCheck(cla->get_name());
	methodTable->exitscope();
	attrTable->exitscope();
      }
    }
 
    if (classtable->errors()) {
	cerr << "Compilation halted due to static semantic errors." << endl;
	exit(1);
    }
}



