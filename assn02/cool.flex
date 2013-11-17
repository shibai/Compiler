/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */
int count = 0;

%}

/*
 * Define names for regular expressions here.
 */

%x str comm skip

DARROW          =>

delim		[ \t\r\f\v]
ws		{delim}+
lower		[a-z]
upper		[A-Z]
letter		[a-zA-Z]
digit		[0-9]
typeId		{upper}({letter}|{digit}|_)*
objectId	{lower}({letter}|{digit}|_)*
digits		{digit}+

%%
 /*
  *  Nested comments
  */


 /*
  *  The multiple-character operators.
  */
{DARROW}		{ return (DARROW); }

[iI][fF]		return IF;
[tT][hH][eE][nN]			return THEN;
[eE][lL][sS][eE]	return ELSE;
[cC][aA][sS][eE]	return CASE;
[fF][iI]		return FI;
[wW][hH][iI][lL][eE]	return WHILE;
[iI][nN][hH][eE][rR][iI][tT][sS]	return INHERITS;
[lL][eE][tT]		return LET;
[lL][oO][oO][pP]	return LOOP;
[pP][oO][oO][lL]	return POOL;
[nN][eE][wW]		return NEW;
[eE][sS][aA][cC]	return ESAC;
[oO][fF]		return OF;
[nN][oO][tT]		return NOT;
[iI][nN]		return IN;
[iI][sS][vV][oO][iI][dD]	return ISVOID;
[cC][lL][aA][sS][sS]	return CLASS;

"+"|"-"|"*"|"/"|"="|"("|")"|"{"|"}"|";"|":"|"."|","|"<"|"~"|"@"	return yytext[0];	
"_"|"["|"]"|"!"|"#"|"$"|"%"|"^"|"&"|">"|"?"|"`"|"|"|[\\]	{
			cool_yylval.error_msg = yytext;
			return ERROR; // error
			}


[\x01-\x08]		{
			cool_yylval.error_msg = yytext;
			return ERROR;
			}

"<="			return LE;
"<-"			return ASSIGN;


(t[rR][uU][eE])		{
			cool_yylval.boolean = true;
			return BOOL_CONST;
			}
(f[Aa][lL][sS][eE])	{
			cool_yylval.boolean = false;
			return BOOL_CONST;
			}

{typeId}		{
	   		cool_yylval.symbol = idtable.add_string(yytext);
			return TYPEID;
			}

{objectId}|(self)	{
			cool_yylval.symbol = idtable.add_string(yytext);
			return OBJECTID;
			}

{digits}		{
			cool_yylval.symbol = inttable.add_string(yytext);
			return INT_CONST;
			}
"--"(.)*

"*)"			{
			cool_yylval.error_msg = "unmatched comments";
			return ERROR;
			}
"(*"			BEGIN(comm);count++;


<comm>"*)"		{
			count--;
			if (count == 0) BEGIN 0;
			else if (count < 0) {
			cool_yylval.error_msg = "unmatched comments";
			count = 0;
			return ERROR;
			}
			}
<comm>"(*"		{
			count++;
			}
<comm>\\(.|\n)
<comm>.
<comm>{ws}
<comm>\n		curr_lineno++;
<comm><<EOF>>		{
			BEGIN 0;
			if (count > 0) {
			cool_yylval.error_msg = "EOF in comment";
			count = 0;
			return ERROR;
			}
			}			

\"			{
			string_buf_ptr = string_buf;
			BEGIN(str);
			}

<str>{

\"			{
			// check string length
			if (string_buf_ptr - string_buf > 1024) {
			*string_buf = '\0';
			cool_yylval.error_msg = "String constant too long";
			BEGIN(skip);
			return ERROR;
			}
			*string_buf_ptr = '\0';
			cool_yylval.symbol = stringtable.add_string(string_buf);
			BEGIN 0;
			return 	STR_CONST;
			}

<<EOF>>			{
			cool_yylval.error_msg = "EOF in string constant";
			BEGIN 0;
			return ERROR;
			}

\\n			*string_buf_ptr++ = '\n'; 
\\t			*string_buf_ptr++ = '\t';
\\b			*string_buf_ptr++ = '\b';
\\f			*string_buf_ptr++ = '\f';
\\[^\0]			*string_buf_ptr++ = yytext[1]; // any characters but null

\n			{
			// error
			*string_buf = '\0';
			BEGIN(0);
			cool_yylval.error_msg = "Unterminated string constant";
			return ERROR;
			}

\0			{
			*string_buf = '\0';
			cool_yylval.error_msg = "String contains null character";
			BEGIN(skip);
			return ERROR;
			}

.			{
			*string_buf_ptr++ = *yytext;
			
			}

}

<skip>[\n|"]		BEGIN 0; // after null character in string is found
<skip>[^\n|"]			


\n			curr_lineno++;
{ws}			

 /*
  * KEYWORDS ARE CASE-INSENSITIVE EXCEPT FOR THE VALUES TRUE AND FALSE,
  * WHICH MUST BEGIN WITH A LOWER-CASE LETTER.
  */


 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */


%%