/*
*  cool.y
*              Parser definition for the COOL language.
*
*/
%{
  #include <iostream>
  #include "cool-tree.h"
  #include "stringtab.h"
  #include "utilities.h"
  
  extern char *curr_filename;
  
  
  /* Locations */
  #define YYLTYPE int              /* the type of locations */
  #define cool_yylloc curr_lineno  /* use the curr_lineno from the lexer
  for the location of tokens */
    
    extern int node_lineno;          /* set before constructing a tree node
    to whatever you want the line number
    for the tree node to be */
      
      
      #define YYLLOC_DEFAULT(Current, Rhs, N)         \
      Current = Rhs[1];                             \
      node_lineno = Current;
    
    
    #define SET_NODELOC(Current)  \
    node_lineno = Current;
    
    /* IMPORTANT NOTE ON LINE NUMBERS
    *********************************
    * The above definitions and macros cause every terminal in your grammar to 
    * have the line number supplied by the lexer. The only task you have to
    * implement for line numbers to work correctly, is to use SET_NODELOC()
    * before constructing any constructs from non-terminals in your grammar.
    * Example: Consider you are matching on the following very restrictive 
    * (fictional) construct that matches a plus between two integer constants. 
    * (SUCH A RULE SHOULD NOT BE  PART OF YOUR PARSER):
    
    plus_consts	: INT_CONST '+' INT_CONST 
    
    * where INT_CONST is a terminal for an integer constant. Now, a correct
    * action for this rule that attaches the correct line number to plus_const
    * would look like the following:
    
    plus_consts	: INT_CONST '+' INT_CONST 
    {
      // Set the line number of the current non-terminal:
      // ***********************************************
      // You can access the line numbers of the i'th item with @i, just
      // like you acess the value of the i'th exporession with $i.
      //
      // Here, we choose the line number of the last INT_CONST (@3) as the
      // line number of the resulting expression (@$). You are free to pick
      // any reasonable line as the line number of non-terminals. If you 
      // omit the statement @$=..., bison has default rules for deciding which 
      // line number to use. Check the manual for details if you are interested.
      @$ = @3;
      
      
      // Observe that we call SET_NODELOC(@3); this will set the global variable
      // node_lineno to @3. Since the constructor call "plus" uses the value of 
      // this global, the plus node will now have the correct line number.
      SET_NODELOC(@3);
      
      // construct the result node:
      $$ = plus(int_const($1), int_const($3));
    }
    
    */
    
    
    
    void yyerror(char *s);        /*  defined below; called for each parse error */
    extern int yylex();           /*  the entry point to the lexer  */
    
    /************************************************************************/
    /*                DONT CHANGE ANYTHING IN THIS SECTION                  */
    
    Program ast_root;	      /* the result of the parse  */
    Classes parse_results;        /* for use in semantic analysis */
    int omerrs = 0;               /* number of errors in lexing and parsing */
    %}
    
    /* A union of all the types that can be the result of parsing actions. */
    %union {
      Boolean boolean;
      Symbol symbol;
      Program program;
      Class_ class_;
      Classes classes;
      Feature feature;
      Features features;
      Formal formal;
      Formals formals;
      Case case_;
      Cases cases;
      Expression expression;
      Expressions expressions;
      char *error_msg;
    }
    
    /* 
    Declare the terminals; a few have types for associated lexemes.
    The token ERROR is never used in the parser; thus, it is a parse
    error when the lexer returns it.
    
    The integer following token declaration is the numeric constant used
    to represent that token internally.  Typically, Bison generates these
    on its own, but we give explicit numbers to prevent version parity
    problems (bison 1.25 and earlier start at 258, later versions -- at
    257)
    */
    %token CLASS 258 ELSE 259 FI 260 IF 261 IN 262 
    %token INHERITS 263 LET 264 LOOP 265 POOL 266 THEN 267 WHILE 268
    %token CASE 269 ESAC 270 OF 271 DARROW 272 NEW 273 ISVOID 274
    %token <symbol>  STR_CONST 275 INT_CONST 276 
    %token <boolean> BOOL_CONST 277
    %token <symbol>  TYPEID 278 OBJECTID 279 
    %token ASSIGN 280 NOT 281 LE 282 ERROR 283
    
    /*  DON'T CHANGE ANYTHING ABOVE THIS LINE, OR YOUR PARSER WONT WORK       */
    /**************************************************************************/
    
    /* Complete the nonterminal list below, giving a type for the semantic
    value of each non terminal. (See section 3.6 in the bison 
    documentation for details). */
    
    /* Declare types for the grammar's non-terminals. */
    %type <program> program
    %type <classes> class_list
    %type <class_> class

     /* my code */ 
    
    /* You will want to change the following line. */
     
    %type <features> feature_list 
    %type <feature> feature_single
    %type <formals> formal_list 
    %type <formals> formal_list2
    %type <formal> formal_single
    %type <expressions> expression_list
    %type <expressions> expr_comma_list
    %type <expression> expression_single 
    %type <expression> let_list
    %type <case_> case_single
    %type <cases> case_list

    /* Precedence declarations go here. */
    %nonassoc IN
    %right ASSIGN
    %left NOT
    %nonassoc '<' LE '='
    %left '+' '-'
    %left '*' '/'
    %left ISVOID
    %left '~' 
    %left '@'
    %left '.'

    %%
    /* 
    Save the root of the abstract syntax tree in a global variable.
    */
    program	: class_list	{ @$ = @1; ast_root = program($1); }
    ;
    
    class_list
    : class			/* single class */
    { $$ = single_Classes($1);
    parse_results = $$; }
    | class_list class	/* several classes */
    { $$ = append_Classes($1,single_Classes($2)); 
    parse_results = $$; }
    ;
    
    /* If no parent is specified, the class inherits from the Object class. */
    class	: CLASS TYPEID '{' feature_list '}' ';'
    { $$ = class_($2,idtable.add_string("Object"),$4,
    stringtable.add_string(curr_filename)); }
    | CLASS TYPEID INHERITS TYPEID '{' feature_list '}' ';'
    { $$ = class_($2,$4,$6,stringtable.add_string(curr_filename)); }
    | error
    {}
    ;

    /* Feature list may be empty, but no empty features in list. */

    feature_list : /* empty */
    { $$ = nil_Features();}
    | feature_single ';'
    { $$ = single_Features($1);}
    | feature_list feature_single ';'
    { $$ = append_Features($1,single_Features($2));}

    /* Note: by default, Bison chooses shift over reduce */
    feature_single :
    OBJECTID '(' formal_list ')' ':' TYPEID '{' expression_single '}' 
    { $$ = method($1,$3,$6,$8);}
    | OBJECTID ':' TYPEID 
    { $$ = attr($1,$3,no_expr());}
    | OBJECTID ':' TYPEID ASSIGN expression_single 
    { $$ = attr($1,$3,$5);}
    | error 
    {}
    
    formal_list : /* empty */
    { $$ = nil_Formals();}
    | formal_list2
    { $$ = $1;}

    formal_list2 :
    formal_single
    { $$ = single_Formals($1);}
    | formal_list2 ',' formal_single
    { $$ = append_Formals($1,single_Formals($3));}

    formal_single :
    OBJECTID ':' TYPEID
    { $$ = formal($1,$3);}
    | error
    {}

    expression_list :
    expression_single ';'
    { $$ = single_Expressions($1);}
    | expression_list  expression_single ';'
    { $$ = append_Expressions($1,single_Expressions($3));}
 
   
    expr_comma_list : /* empty */
    { $$ = nil_Expressions();}
    | expression_single
    { $$ = single_Expressions($1);}
    | expr_comma_list ',' expression_single
    { $$ = append_Expressions($1,single_Expressions($3));}
    ;

    expression_single :  
    OBJECTID ASSIGN expression_single
    { $$ = assign($1,$3);}
    /* dispatch */
    | expression_single '.' OBJECTID '(' expr_comma_list ')'
    { $$ = dispatch($1,$3,$5);}
    | expression_single '@' TYPEID '.' OBJECTID '(' expr_comma_list ')'
    { $$ = static_dispatch($1,$3,$5,$7);}

    /* omitted self dispatch */
    | OBJECTID '(' expr_comma_list ')'
    { $$ = dispatch(object(idtable.add_string("self")),$1,$3);}

    /* if condition */
    | IF expression_single THEN expression_single FI
    { $$ = cond($2,$4,no_expr());}
    | IF expression_single THEN expression_single ELSE expression_single FI
    { $$ = cond($2,$4,$6);}

    /* while */
    | WHILE expression_single LOOP expression_single POOL
    { $$ = loop($2,$4);}

    /* block */
    | '{' expression_list '}'
    { $$ = block($2);}

    /* let */
    | LET let_list
    { $$ = $2;}

    /* case */
    | CASE expression_single OF case_list ';' ESAC
    { $$ = typcase($2,$4);}

    | NEW TYPEID
    { $$ = new_($2);}
    | ISVOID expression_single
    { $$ = isvoid($2);}

    /* operators */
    | expression_single '+' expression_single
    { $$ = plus($1,$3);}
    | expression_single '-' expression_single
    { $$ = sub($1,$3);}
    | expression_single '*' expression_single
    { $$ = mul($1,$3);}
    | expression_single '/' expression_single
    { $$ = divide($1,$3);}
    | '~' expression_single
    { $$ = neg($2);}
    | expression_single '<' expression_single
    { $$ = lt($1,$3);}
    | expression_single LE expression_single
    { $$ = leq($1,$3);}
    | expression_single '=' expression_single
    { $$ = eq($1,$3);}
    | NOT expression_single
    { $$ = comp($2);}
    | '(' expression_single ')'
    { $$ = $2 ;}
    | OBJECTID
    { $$ = object($1);}
    | INT_CONST
    { $$ = int_const($1);}
    | STR_CONST
    { $$ = string_const($1);}
    | BOOL_CONST
    { $$ = bool_const($1);}
    | error
    {}

    let_list :
    OBJECTID ':' TYPEID IN expression_single
    { $$ = let($1,$3,no_expr(),$5);}
    | OBJECTID ':' TYPEID ASSIGN expression_single IN expression_single
    { $$ = let($1,$3,$5,$7);}
    | OBJECTID ':' TYPEID ',' let_list
    { $$ = let($1,$3,no_expr(),$5);}
    | OBJECTID ':' TYPEID ASSIGN expression_single ',' let_list
    { $$ = let($1,$3,$5,$7);}

    case_list :
    case_single
    { $$ = single_Cases($1);}
    |case_list ';' case_single
    { $$ = append_Cases($1,single_Cases($3));}
//   | error
//    {}
    case_single :
    OBJECTID ':' TYPEID DARROW expression_single
    { $$ = branch($1,$3,$5);}
   




    /* end of grammar */
    %%
    
    /* This function is called automatically when Bison detects a parse error. */
    void yyerror(char *s)
    {
      extern int curr_lineno;
      
      cerr << "\"" << curr_filename << "\", line " << curr_lineno << ": " \
      << s << " at or near ";
      print_cool_token(yychar);
      cerr << endl;
      omerrs++;
      
      if(omerrs>50) {fprintf(stdout, "More than 50 errors\n"); exit(1);}
    }
    
    
