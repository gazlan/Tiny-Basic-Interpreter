//////////////////////////////////////////////////////////////////////////
// James Roskind Tiny Basic Interpreter recompilesd with MSVC 6.0
// http://www.noniandjim.com/Jim/uproc/tinymsdos.zip
// (c) gazlan@yandex.ru, 2018
//////////////////////////////////////////////////////////////////////////
// Tiny Basic Interpreter
// Copyright (c) 1995 Jim Patchell
// Requires the use of Parsifal Software's Anagram LALR compiler.
// http://www.parsifalsoft.com/
//////////////////////////////////////////////////////////////////////////

/* ******************************************************************** **
**                uses pre-compiled headers
** ******************************************************************** */

#include "stdafx.h"

#include "tiny.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* ******************************************************************** **
** @@                   internal defines
** ******************************************************************** */

/*
AnaGram, A System for Syntax Directed Programming
Copyright (c) 1993-2002, Parsifal Software.
All Rights Reserved.

Version 2.01.08a   Feb 28 2002

Serial number 2P17253
Registered to:
  Parsifal Software
  AnaGram 2.01 Master
*/

#ifndef TINY_H_1528923969
#include "tiny_basic.h"
#endif

#ifndef TINY_H_1528923969
#error Mismatched header file
#endif

#include <ctype.h>
#include <stdio.h>

#define RULE_CONTEXT (&((PCB).cs[(PCB).ssx]))
#define ERROR_CONTEXT ((PCB).cs[(PCB).error_frame_ssx])
#define CONTEXT ((PCB).cs[(PCB).ssx])



int tiny_value(void);


#line 653 "C:/Samples/M$DOS/tiny.syn"
// these are the C functions for the above basic grammar
char* FindNextLine(int linenumber,char *s,int *flag);

tiny_pcb_type     tiny_pcb[2];
int               buffer_selector;

#define PCB tiny_pcb[buffer_selector]
#define COMMAND_LINE 0
#define PROGRAM         1

char              name[256];
int               name_index;
hashtab*          Symbols;
char*             ProgramSpace;
int               ProgSpaceSize;
FOR_STACK         ForStack[20]; // 20 loops deep
int               ForStackPointer = -1;
int               CurrentLineNumber;
bool              bRun = false;   // when true, parser actions really do something
SubroutineStack   RetStack[100]; // one hundred levels of subroutine
int               RetStackPointer = -1;

//////////////////////////////////////////////////////////////////////////
void iins()
{
   name_index = 0;
}
//////////////////////////////////////////////////////////////////////////
void ins(int c)
{
   name[0] = (char)c;
   name_index = 1;
}
//////////////////////////////////////////////////////////////////////////
void pcn(int c)
{
   ASSERT(name_index < 256);
   name[name_index++] = (char)c;
}
//////////////////////////////////////////////////////////////////////////
// member functions for hashtab class
//////////////////////////////////////////////////////////////////////////
DWORD hashtab::hash(BYTE* name)
{
   //--------------------------------------------------------
   // this is the hashing function.  It takes the character
   // string name and creates a value from that...sort of
   // like a CRC
   //--------------------------------------------------------
   DWORD h = 0;
   DWORD g;

   for (; *name; ++name)
   {
      h = (h << 2) + *name;

      // this line is correct...do not change
      g = h & 0xC000;

      if (g)
      {
         h = (h ^ (g >> 12)) & 0x3fff;
      }
   }

   return h;
}
//////////////////////////////////////////////////////////////////////////
hashtab::hashtab(int size)
{
   //--------------------------------------------------------
   //Constructor
   //besides creating the hash table object, we need to create
   // an array of BUCKET pointers.
   // size: number of elements in the BUCKET pointer array
   //     : It should be noted that this should be a prime
   //     : number...it is a mathematic thingy
   //--------------------------------------------------------
   table = (BUCKET**)new char[size * sizeof(BUCKET*)];

   memset(table,0,size * sizeof(BUCKET*));

   tabsize = size;
}
//////////////////////////////////////////////////////////////////////////
hashtab::~hashtab()
{
   delete[] table;
   table = NULL;
}
//////////////////////////////////////////////////////////////////////////
void* hashtab::newsym(int size)
{
   //---------------------------------------------------------
   // Create a new symbol
   // size: This is the size in bytes of the new symbol
   // Action:
   // Each symbol is contained in a BUCKET
   // Allocate enough space for a BUCKET plus the symbol
   // Increment the BUCKET * (variable s) which will now
   // point to the symbol
   // Think of this object as an array, the first element
   // being a BUCKET and the second being a symbol
   // When you increment the pointer to the BUCKET, you
   // will point to the next element in the array, which
   // happens to not be a BUCKET, but rather a symbol...
   // pretty fucked up if you ask me
   //---------------------------------------------------------

   BUCKET*  s  = (BUCKET*)new char[sizeof(BUCKET) + size];

   ++s;

   return (void*)s;
}
//////////////////////////////////////////////////////////////////////////
void* hashtab::addsym(void* isym)
{
   //----------------------------------------------------------------
   // add a symbol to the hash table
   //
   // isym: pointer to the symbol to add
   //     : the symbol should have been created with newsym (above)
   // action:
   //----------------------------------------------------------------
   BUCKET** p, *tmp;
   BUCKET*  sym = (BUCKET*)isym; // sym points to the symbol
   //----------------------------------------------------------------
   // the next line is pretty wierd...
   // the first element in the symbol is the symbol name
   // so by passing sym as a character pointer is
   // passing the name to the hash function
   // sym is then decremented, which then points us to the BUCKET
   // the hash value is the MODed with the hash table size to get
   // and index into the hash table, which we then use to get a
   // BUCKET pointer to the first element.
   //----------------------------------------------------------------
   p = &table[hash((BYTE*) sym--) % tabsize];
   tmp = *p;   // store the first element temporarily
   *p  = sym;  // put new symbol at head of BUCKET list
   sym->prev = p; //set the prev pointer
   sym->next = tmp;  //set the next pointer

   if (tmp)
   {
      tmp->prev = &sym->next;
   }
   ++numsyms;

   return (void*) (sym + 1);
}
//////////////////////////////////////////////////////////////////////////
void* hashtab::nextsym(void* ilast)
{
   BUCKET*  last  = (BUCKET*)ilast;

   for (--last; last->next; last = last->next)
   {
      if (!strcmp((char*)ilast,(char*)(last->next + 1)))
      {
         return (void*)(last->next + 1);
      }
   }

   return NULL;
}
//////////////////////////////////////////////////////////////////////////
void* hashtab::findsym(char* isym)
{
   BUCKET*  p;

   p = table[hash(((BYTE*)isym) /*--*/) % tabsize];

   while (p && strcmp((char*)isym,(char*)(p + 1)))
   {
      p = p->next;
   }

   return (void*)(p  ?  p + 1  :  NULL);
}
//////////////////////////////////////////////////////////////////////////
void hashtab::delsym(void* isym)
{
   BUCKET*  sym = (BUCKET*)isym;

   if (sym)
   {
      --numsyms;
      --sym;

      *(sym->prev) = sym->next;

      if (*(sym->prev))
      {
         sym->next->prev = sym->prev;
      }
   }
}
//////////////////////////////////////////////////////////////////////////
void hashtab::dump(FILE* o)
{
   //--------------------------------------------------------
   // dump all of the symbols
   //--------------------------------------------------------
   int      i;
   BUCKET*  b;
   symbol*  s;

   for (i = 0; i < tabsize; ++i)
   {
      if (table[i])   //anything is this one?
      {
         b = table[i];

         while (b)
         {
            s = (symbol*)(b + 1);
            s->PrintInfo(o);  //print symbol information
            b = b->next;
         }
      }
   }
}
//////////////////////////////////////////////////////////////////////////
// member functions for symbol class
//////////////////////////////////////////////////////////////////////////
symbol::symbol(char* s)
{
   strcpy(name,s);
   next = NULL;
   sv = NULL;
   value.lv = 0;
}
//////////////////////////////////////////////////////////////////////////
symbol::~symbol()
{
   if (sv)
   {
      delete[] sv;
      sv = NULL;
   }
}
//////////////////////////////////////////////////////////////////////////
void symbol::Init() //fake constructor
{
   next = NULL;
   sv = NULL;
}
//////////////////////////////////////////////////////////////////////////
void symbol::SetName(char* s)
{
   strcpy(name,s);
}
//////////////////////////////////////////////////////////////////////////
void symbol::SetVal(int v)
{
   value.lv = v;
}
//////////////////////////////////////////////////////////////////////////
int symbol::GetIntVal()
{
   int  val = 0;

   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         val = value.lv;
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         val = (int) value.dv;
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         val = 0;
         break;
   }

   return val;
}
//////////////////////////////////////////////////////////////////////////
void symbol::SetVal(double v)
{
   value.dv = v;
}
//////////////////////////////////////////////////////////////////////////
double symbol::GetDoubleVal()
{
   double   val   = 0.0;

   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         val = (double) value.lv;
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         val = value.dv;
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         val = 0.0;
         break;
   }

   return val;
}
//////////////////////////////////////////////////////////////////////////
void symbol::SetType(int t)
{
   type = t;
}
//////////////////////////////////////////////////////////////////////////
int symbol::GetType()
{
   return type;
}
//////////////////////////////////////////////////////////////////////////
void symbol::SetVal(symbol* v)
{
   int   vtype = v->GetType();
   type = vtype;

   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
      {
         value.lv = v->GetIntVal();
         break;
      }
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
      {
         value.dv = v->GetDoubleVal();
         break;
      }
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
      {
         sv = new char[v->GetStringLen() + 1];
         strcpy(sv,v->GetStringVal());
         break;
      }
   }
}
//////////////////////////////////////////////////////////////////////////
char* symbol::GetStringVal()
{
   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         if (!sv)
         {
            sv = new char[256];
         }
         sprintf(sv,"%ld",value.lv);
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         if (!sv)
         {
            sv = new char[256];
         }
         sprintf(sv,"%lf",value.dv);
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         break;
   }
   return sv;
}
//////////////////////////////////////////////////////////////////////////
int symbol::GetStringLen()
{
   int   l  = 0;
   if (sv)
   {
      l = strlen(sv);
   }
   return l;
}
//////////////////////////////////////////////////////////////////////////
void symbol::SetMaxStringLen(int d)
{
   sv = new char[d];
   value.lv = d;
   memset(sv,0,d);   //zero new memory
}
//////////////////////////////////////////////////////////////////////////
void symbol::SetVal(char* s)
{
   sv = new char[strlen(s) + 1];
   strcpy(sv,s);
}
//////////////////////////////////////////////////////////////////////////
static const char*   symtypes[] =
{
   "No type",
   "Int",
   "Int",
   "Double",
   "Double",
   "String",
   "String"
};
//////////////////////////////////////////////////////////////////////////
void symbol::PrintInfo(FILE* o)
{
   fprintf(o,"---------------\"%s\"----------------\n",name);
   fprintf(o,"Type = %s\n",symtypes[type]);
   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         fprintf(o,"%ld\n",value.lv);
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         fprintf(o,"%lf\n",value.dv);
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         if (sv)
         {
            fprintf(o,"MaxSize:%ld->%s\n",value.lv,sv);
         }
         else
         {
            fprintf(o,"No String Yet\n");
         }
         break;
   }

   fprintf(o,"--------------------------------------------\n");
}
//////////////////////////////////////////////////////////////////////////
// Misc Program Utility Routines
//////////////////////////////////////////////////////////////////////////
symbol* MakeIntValue(int v)
{
   symbol*  s  = new symbol("temp");
   s->SetVal(v);
   s->SetType(SYMBOLTYPE_TEMPINT);
   return s;
}
//////////////////////////////////////////////////////////////////////////
int ConvertToInt(symbol* s,int deleteflag)
{
   int   type  = s->GetType();
   int  v     = 0;

   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         v = s->GetIntVal();
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         v = (int) s->GetDoubleVal();
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         printf("Big Time Error!\n");
         break;
   }

   if (!((type & 1)) && deleteflag)   // delete temporary symbol if desired
   {
      delete s;
      s = NULL;
   }

   return v;
}
//////////////////////////////////////////////////////////////////////////
void DoForLoop(symbol* s,symbol* limit,symbol* step)
{
   int   flag;

   ++ForStackPointer;      //increment stack pointer
   ForStack[ForStackPointer].start = FindNextLine((int) CurrentLineNumber,ProgramSpace,&flag);
   ForStack[ForStackPointer].count = ConvertToInt(limit,1);
   ForStack[ForStackPointer].step = ConvertToInt(step,1);
   ForStack[ForStackPointer].loopvar = s;
}
//////////////////////////////////////////////////////////////////////////
void DoAssign(symbol* s,symbol* v)
{
   int   type1 = v->GetType();
   switch (s->GetType())
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         s->SetVal(v->GetIntVal());
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         s->SetVal(v->GetDoubleVal());
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         s->SetVal(v->GetStringVal());
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v;
      v = NULL;
   }
}
//////////////////////////////////////////////////////////////////////////
void DoPrint(symbol* v)
{
   int   type  = v->GetType();

   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         printf("%ld",v->GetIntVal());
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         printf("%lf",v->GetDoubleVal());
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         printf("%s",v->GetStringVal());
         break;
   }

   if (!(type & 1))  // check to see if input symbol is a temp
   {
      delete v;
      v = NULL;
   }
}
//////////////////////////////////////////////////////////////////////////
void DoPoke(symbol* a,symbol* v,int optype)
{
   int  address;
   int  val   = 0;
   int   type  = v->GetType();

   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         val = v->GetIntVal();
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         val = (int) v->GetDoubleVal();
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         break;
   }

   if (!(type & 1))  // check to see if input symbol is a temp
   {
      delete v;
      v = NULL;
   }

   type = a->GetType();

   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         address = v->GetIntVal();
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         address = (int) v->GetDoubleVal();
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         break;
   }

   if (!(type & 1))  //check to see if input symbol is a temp
   {
      delete a;
      a = NULL;
   }

   switch (optype)
   {
      case BYTE_POKE:
         *((char*)a) = (char)val;
         break;
      case WORD_POKE:
         *((int*)a) = (int)val;
         break;
      case INT_POKE:
         *((int*)a) = val;
         break;
   }
}
//////////////////////////////////////////////////////////////////////////
symbol* DoEquals(symbol* v1,symbol* v2)
{
   int     v     = 0;
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s     = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = v1->GetIntVal() == v2->GetIntVal();
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = (double) v1->GetIntVal() == v2->GetDoubleVal();
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = v1->GetDoubleVal() == (double) v2->GetIntVal();
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = v1->GetDoubleVal() == v2->GetDoubleVal();
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = 0;
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = 0;
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) // check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   s->SetVal(v);
   s->SetType(SYMBOLTYPE_TEMPINT);

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoNotEquals(symbol* v1,symbol* v2)
{
   int     v     = 0;
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = v1->GetIntVal() != v2->GetIntVal();
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = (double) v1->GetIntVal() != v2->GetDoubleVal();
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = v1->GetDoubleVal() != (double) v2->GetIntVal();
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = v1->GetDoubleVal() != v2->GetDoubleVal();
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = 0;
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = 0;
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) // check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   s->SetVal(v);
   s->SetType(SYMBOLTYPE_TEMPINT);

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoLogicalAnd(symbol* v1,symbol* v2)
{
   int     v     = 0;
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s     = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         v = v1->GetIntVal() && v2->GetIntVal();
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         v = 0;
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) // check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   s->SetVal(v);
   s->SetType(SYMBOLTYPE_TEMPINT);

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoLogicalOr(symbol* v1,symbol* v2)
{
   int     v     = 0;
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = v1->GetIntVal() || v2->GetIntVal();
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = v1->GetIntVal() || (int) v2->GetDoubleVal();
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = (int) v1->GetDoubleVal() || v2->GetIntVal();
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = (int) v1->GetDoubleVal() || (int) v2->GetDoubleVal();
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = 0;
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = 0;
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
   }

   if (!(type1 & 1)) //check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) //check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   s->SetVal(v);
   s->SetType(SYMBOLTYPE_TEMPINT);

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoAdd(symbol* v1,symbol* v2)
{
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();
   symbol*  s     = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         s->SetVal(v1->GetIntVal() + v2->GetIntVal());
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         s->SetVal(v1->GetDoubleVal() + v2->GetDoubleVal());
         s->SetType(SYMBOLTYPE_TEMPDOUBLE);
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         s->SetMaxStringLen(v1->GetStringLen() + v2->GetStringLen() + 1);
         s->SetType(SYMBOLTYPE_TEMPSTRING);
         strcpy(s->GetStringVal(),v1->GetStringVal());
         strcat(s->GetStringVal(),v2->GetStringVal());
         break;
   }

   if (!(type1 & 1)) //check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) //check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoSubtract(symbol* v1,symbol* v2)
{
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         s->SetVal(v1->GetIntVal() - v2->GetIntVal());
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         s->SetVal(v1->GetDoubleVal() - v2->GetDoubleVal());
         s->SetType(SYMBOLTYPE_TEMPDOUBLE);
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         s->SetVal(0l);
         s->SetType(SYMBOLTYPE_TEMPSTRING);
         break;
   }

   if (!(type1 & 1)) //check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) //check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoMult(symbol* v1,symbol* v2)
{
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         s->SetVal(v1->GetIntVal() * v2->GetIntVal());
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         s->SetVal(v1->GetDoubleVal() * v2->GetDoubleVal());
         s->SetType(SYMBOLTYPE_TEMPDOUBLE);
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         s->SetVal(0l);
         s->SetType(SYMBOLTYPE_TEMPSTRING);
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) // check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoDivide(symbol* v1,symbol* v2)
{
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         s->SetVal(v1->GetIntVal() / v2->GetIntVal());
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         s->SetVal(v1->GetDoubleVal() / v2->GetDoubleVal());
         s->SetType(SYMBOLTYPE_TEMPDOUBLE);
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         s->SetVal(0l);
         s->SetType(SYMBOLTYPE_TEMPSTRING);
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) // check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoMod(symbol* v1,symbol* v2)
{
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         s->SetVal(v1->GetIntVal() % v2->GetIntVal());
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         s->SetVal(0l);
         s->SetType(SYMBOLTYPE_TEMPSTRING);
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) // check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoGreaterThan(symbol* v1,symbol* v2)
{
   int     v = 0;
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = v1->GetIntVal() > v2->GetIntVal();
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = v1->GetDoubleVal() > v2->GetDoubleVal();
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = v1->GetDoubleVal() > v2->GetDoubleVal();
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = v1->GetDoubleVal() > v2->GetDoubleVal();
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = 0;
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = 0;
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) // check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   s->SetVal(v);
   s->SetType(SYMBOLTYPE_TEMPINT);

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoLessThan(symbol* v1,symbol* v2)
{
   int     v = 0;
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = v1->GetIntVal() < v2->GetIntVal();
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = (double) v1->GetIntVal() < v2->GetDoubleVal();
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = v1->GetDoubleVal() < (double) v2->GetIntVal();
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = v1->GetDoubleVal() < v2->GetDoubleVal();
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         switch (type2)
         {
            case SYMBOLTYPE_INT:
            case SYMBOLTYPE_TEMPINT:
               v = 0;
               break;
            case SYMBOLTYPE_DOUBLE:
            case SYMBOLTYPE_TEMPDOUBLE:
               v = 0;
               break;
            case SYMBOLTYPE_STRING:
            case SYMBOLTYPE_TEMPSTRING:
               v = 0;
               break;
         }
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) // check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   s->SetVal(v);
   s->SetType(SYMBOLTYPE_TEMPINT);

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoShiftRight(symbol* v1,symbol* v2)
{
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         s->SetVal(v1->GetIntVal() >> v2->GetIntVal());
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         s->SetType(SYMBOLTYPE_TEMPSTRING);
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) // check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoShiftLeft(symbol* v1,symbol* v2)
{
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         s->SetVal(v1->GetIntVal() << v2->GetIntVal());
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         s->SetType(SYMBOLTYPE_TEMPSTRING);
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) // check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   return s;
}

symbol* DoBitAnd(symbol* v1,symbol* v2)
{
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         s->SetVal(v1->GetIntVal() & v2->GetIntVal());
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         s->SetType(SYMBOLTYPE_TEMPSTRING);
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) // check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoBitOr(symbol* v1,symbol* v2)
{
   int      type1 = v1->GetType();
   int      type2 = v2->GetType();

   symbol*  s = new symbol("temp");

   switch (type1)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         s->SetVal(v1->GetIntVal() | v2->GetIntVal());
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         s->SetType(SYMBOLTYPE_TEMPSTRING);
         break;
   }

   if (!(type1 & 1)) // check to see if input symbol is a temp
   {
      delete v1;
      v1 = NULL;
   }

   if (!(type2 & 1)) // check to see if input symbol is a temp
   {
      delete v2;
      v2 = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoNeg(symbol* v)
{
   int      type  = v->GetType();

   symbol*  s = new symbol("temp");

   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         s->SetVal(-v->GetIntVal());
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         s->SetVal(-v->GetDoubleVal());
         s->SetType(SYMBOLTYPE_TEMPDOUBLE);
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         break;
   }

   if (!(type & 1))  //check to see if input symbol is a temp
   {
      delete v;
      v = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoPeekI(symbol* v)
{
   int      type  = v->GetType();
   symbol*  s     = new symbol("temp");

   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         s->SetVal(*((int*)(v->GetIntVal())));
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         break;
   }

   if (!(type & 1))  //check to see if input symbol is a temp
   {
      delete v;
      v = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoPeekW(symbol* v)
{
   int      type  = v->GetType();
   symbol*  s     = new symbol("temp");

   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         s->SetVal(int(*((int*)(v->GetIntVal()))));
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         break;
   }

   if (!(type & 1))  //check to see if input symbol is a temp
   {
      delete v;
      v = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoPeekB(symbol* v)
{
   int      type  = v->GetType();
   symbol*  s     = new symbol("temp");
   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         s->SetVal(int(*((char*)(v->GetIntVal()))));
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         break;
   }

   if (!(type & 1))  // check to see if input symbol is a temp
   {
      delete v;
      v = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoRandom(symbol* v)
{
   int      type  = v->GetType();
   symbol*  s     = new symbol("temp");

   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
      {
         // s->SetVal((int)rand((int)(v->GetIntVal())));
         s->SetVal((int)rand());
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      }
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
      {
         break;
      }
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
      {
         break;
      }
   }

   if (!(type & 1))  //check to see if input symbol is a temp
   {
      delete v;
      v = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
symbol* DoAbs(symbol* v)
{
   int      type  = v->GetType();
   symbol*  s     = new symbol("temp");
   switch (type)
   {
      case SYMBOLTYPE_INT:
      case SYMBOLTYPE_TEMPINT:
         s->SetVal(labs(v->GetIntVal()));
         s->SetType(SYMBOLTYPE_TEMPINT);
         break;
      case SYMBOLTYPE_DOUBLE:
      case SYMBOLTYPE_TEMPDOUBLE:
         break;
      case SYMBOLTYPE_STRING:
      case SYMBOLTYPE_TEMPSTRING:
         break;
   }

   if (!(type & 1))  //check to see if input symbol is a temp
   {
      delete v;
      v = NULL;
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
char* getline(int prompt)
{
   static char buff[80];

   int         c = 0;
   int         index = 0;

   putchar(prompt);

   while ((c = getchar()) != '\n')
   {
      buff[index++] = (char)c;
   }

   buff[index++] = (char)c;
   buff[index]   = 0;

   return buff;
}
//////////////////////////////////////////////////////////////////////////
int LineNumber(char* s)
{
   /* figure out if there is a line number or not  */
   /* returns 0 if no line number   */
   return atoi(s);
}
//////////////////////////////////////////////////////////////////////////
char* FindLine(int linenumber,char* s,int* flag)
{
   int   ln;

   while (*s)
   {
      if ((ln = LineNumber(s)) == linenumber)
      {
         *flag = 0;
         return(s);
      }

      if (ln > linenumber)
      {
         *flag = 1;
         return s;
      }

      /* not right line number   */
      s = strchr(s,'\n') ; /* find next new line   */
      ++s;           /* increment to beginning of line   */
   }

   return s;
}
//////////////////////////////////////////////////////////////////////////
char* FindNextLine(int linenumber,char* s,int* flag)
{
   char* p  = FindLine(linenumber,s,flag);
   char* q  = strchr(p,'\n');
   ++q;
   return q;      /* well, there should be a line number here  */
}
//////////////////////////////////////////////////////////////////////////
int InsertLine
(
   char*       s,
   char*       newline,
   char*       space,
   DWORD       size
)
{
   // s is pointer of place to put new line
   // newline is pointer to line to insert
   // space is the area where it is to be put
   // size is number of bytes in space
   int   iLen     = strlen(newline);
   int   iMemSize = strlen(s);

   if (s + iLen + iMemSize > space + size)
   {
      printf("Not Enough Memory\n");
      exit(1);
   }

   if (iLen)
   {
      memmove(s + iLen,s,iMemSize);
   }
   else
   {
      printf("Did not have to move memory\n");
   }

   memcpy(s,newline,iLen); // copy in new line

   return 1;            // yep, we could copy 
}
//////////////////////////////////////////////////////////////////////////
void DelLine(char* s,char* space,DWORD size)
{
   char*    p = strchr(s,'\n');

   if (p)
   {
      memmove(s,p + 1,size - (p - space) + 1);
   }
   else
   {
      *s = 0;
   }
}
//////////////////////////////////////////////////////////////////////////
// entry into main
//////////////////////////////////////////////////////////////////////////
int tinybasic(int workspace)
{
   int   loop = 1;

   Symbols = (hashtab*)new hashtab(47);
   ProgramSpace = new char[workspace];
   ProgSpaceSize = workspace;
   memset(ProgramSpace,0,workspace);   /* zero workspace */

   while (loop)
   {
      int   flag;

      char*    s = getline('>');

      int   ln = LineNumber(s);

      if (ln)
      {
         // PARSE THE LINE
         buffer_selector = COMMAND_LINE;
         PCB.pointer = (BYTE*)s;
         tiny();

         if (PCB.exit_flag == AG_SYNTAX_ERROR_CODE)
         {
            printf("Syntax Error\n");
         }
         
         char*    p = FindLine(ln,ProgramSpace,&flag);
         
         if (p)
         {
            if (!flag)
            {
               DelLine(p,ProgramSpace,workspace);
            }
            InsertLine(p,s,ProgramSpace,workspace);
         }
         else
         {
            printf("Could Not Insert that line\n");
         }
      }
      else
      {
         bRun = true;
         buffer_selector = COMMAND_LINE;

         PCB.pointer = (BYTE*)s;

         tiny();

         if (tiny_value())
         {
            loop = 0;
         }

         bRun = false;
      }
   }

   delete[] ProgramSpace;
   ProgramSpace = NULL;

   delete Symbols;
   Symbols = NULL;
   
   return 0;
}
//////////////////////////////////////////////////////////////////////////
int main()
{
   setbuf(stdout,NULL);
   tinybasic(10000);
   return 0;
}
//////////////////////////////////////////////////////////////////////////
#line 1856 "tiny_basic.cpp"

#ifndef CONVERT_CASE

static const char agCaseTable[31] = {
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,    0,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

static int agConvertCase(int c) {
  if (c >= 'a' && c <= 'z') return c ^= 0x20;
  if (c >= 0xe0 && c < 0xff) c ^= agCaseTable[c-0xe0];
  return c;
}

#define CONVERT_CASE(c) agConvertCase(c)

#endif


#ifndef TAB_SPACING
#define TAB_SPACING 8
#endif
int tiny_value(void) {
  int returnValue;
  returnValue = (*(int *) &(PCB).vs[(PCB).ssx]);
  return returnValue;
}

static int ag_rp_1(int v) {
#line 72 "C:/Samples/M$DOS/tiny.syn"
  return v;
#line 1890 "tiny_basic.cpp"
}

static int ag_rp_2(int v) {
#line 75 "C:/Samples/M$DOS/tiny.syn"
  return v;
#line 1896 "tiny_basic.cpp"
}

static int ag_rp_3(int v) {
#line 76 "C:/Samples/M$DOS/tiny.syn"
  return v;
#line 1902 "tiny_basic.cpp"
}

static int ag_rp_4(int v) {
#line 79 "C:/Samples/M$DOS/tiny.syn"
  return v;
#line 1908 "tiny_basic.cpp"
}

static void ag_rp_5(int ln) {
#line 85 "C:/Samples/M$DOS/tiny.syn"
 
                                                               CurrentLineNumber = ln; 
                                                            
#line 1916 "tiny_basic.cpp"
}

static void ag_rp_6(symbol * l) {
#line 92 "C:/Samples/M$DOS/tiny.syn"
                                                               if (bRun)
                                                               {
                                                                  symbol*  s = l;
                                                               
                                                                  while (s)
                                                                  {
                                                                     s->SetType(SYMBOLTYPE_INT);
                                                                     s = s->next;
                                                                  }
                                                               }
                                                            
#line 1932 "tiny_basic.cpp"
}

static void ag_rp_7(symbol * l) {
#line 106 "C:/Samples/M$DOS/tiny.syn"
                                                               if (bRun)
                                                               {
                                                                  symbol*  s = l;
                                                               
                                                                  while (s)
                                                                  {
                                                                     s->SetType(SYMBOLTYPE_DOUBLE);
                                                                     s = s->next;
                                                                  }
                                                               }
                                                            
#line 1948 "tiny_basic.cpp"
}

static symbol * ag_rp_8(symbol * s) {
#line 121 "C:/Samples/M$DOS/tiny.syn"
  return s;
#line 1954 "tiny_basic.cpp"
}

static symbol * ag_rp_9(symbol * l, symbol * s) {
#line 123 "C:/Samples/M$DOS/tiny.syn"
  
                                                               if (bRun)
                                                               {
                                                                  s->next = l;
                                                               }
                                                            
                                                               return s;
                                                            
#line 1967 "tiny_basic.cpp"
}

static symbol * ag_rp_10(symbol * s, int d) {
#line 134 "C:/Samples/M$DOS/tiny.syn"
  
                                                               if (bRun)
                                                               {
                                                                  s->SetMaxStringLen(d);
                                                                  s->SetType(SYMBOLTYPE_STRING);
                                                               }

                                                               return s;
                                                            
#line 1981 "tiny_basic.cpp"
}

static symbol * ag_rp_11(symbol * l, symbol * s, int d) {
#line 145 "C:/Samples/M$DOS/tiny.syn"
  
                                                               if (bRun)
                                                               {
                                                                  s->next = l;
                                                                  s->SetMaxStringLen(d);
                                                                  s->SetType(SYMBOLTYPE_STRING);
                                                               }

                                                               return s;
                                                            
#line 1996 "tiny_basic.cpp"
}

static int ag_rp_12(void) {
#line 158 "C:/Samples/M$DOS/tiny.syn"
 
                                                               Symbols->dump(stdout);
                                                               return 0; 
                                                            
#line 2005 "tiny_basic.cpp"
}

static int ag_rp_13(void) {
#line 164 "C:/Samples/M$DOS/tiny.syn"
 
                                                               printf("%s\n",ProgramSpace);
                                                               return 0; 
                                                            
#line 2014 "tiny_basic.cpp"
}

static int ag_rp_14(void) {
#line 170 "C:/Samples/M$DOS/tiny.syn"
 
                                                               memset(ProgramSpace,0,ProgSpaceSize);
                                                               return 0; 
                                                            
#line 2023 "tiny_basic.cpp"
}

static int ag_rp_15(void) {
#line 176 "C:/Samples/M$DOS/tiny.syn"
  
                                                               buffer_selector = PROGRAM;
                                                               PCB.pointer = (BYTE*)ProgramSpace;
                                                               tiny();
                                                               buffer_selector = COMMAND_LINE;
                                                               return 0;
                                                            
#line 2035 "tiny_basic.cpp"
}

static int ag_rp_16(void) {
#line 185 "C:/Samples/M$DOS/tiny.syn"
  
                                                               name[name_index] = 0;

                                                               char*    s = ProgramSpace;

                                                               FILE*    out = fopen(name,"wt");

                                                               if (!out)
                                                               {
                                                                  fprintf(stderr,"Could not open %s for save\n",name);
                                                               }
                                                               else
                                                               {
                                                                  while (*s)
                                                                  {
                                                                     fputc(*s++,out);
                                                                  }

                                                                  fclose(out);
                                                               }

                                                               return 0;
                                                            
#line 2063 "tiny_basic.cpp"
}

static int ag_rp_17(void) {
#line 210 "C:/Samples/M$DOS/tiny.syn"
  
                                                               name[name_index] = 0;

                                                               char*    s = ProgramSpace;

                                                               FILE*    in = fopen(name,"rt");

                                                               int      c = 0;

                                                               if (!in)
                                                               {
                                                                  fprintf(stderr,"Could not open %s for load\n",name);
                                                               }
                                                               else
                                                               {
                                                                  while ((c = fgetc(in)) != EOF)
                                                                  {
                                                                     *s++ = (char)c;
                                                                  }
                                                               }

                                                               *s = 0;

                                                               fclose(in);
                                                               return 0;
                                                            
#line 2094 "tiny_basic.cpp"
}

static int ag_rp_18(void) {
#line 238 "C:/Samples/M$DOS/tiny.syn"
 
                                                               return 0; 
                                                            
#line 2102 "tiny_basic.cpp"
}

static int ag_rp_19(void) {
#line 243 "C:/Samples/M$DOS/tiny.syn"
 
                                                               return 0; 
                                                            
#line 2110 "tiny_basic.cpp"
}

static int ag_rp_20(int l) {
#line 248 "C:/Samples/M$DOS/tiny.syn"
  
                                                               char* s;
                                                               int   flag;

                                                               if (bRun)
                                                               {
                                                                  s = FindLine(l,ProgramSpace,&flag);

                                                                  if (flag)
                                                                  {
                                                                     fprintf(stderr,"Could not find line %d\n",l);
                                                                     PCB.exit_flag = AG_SEMANTIC_ERROR_CODE;
                                                                  }
                                                                  else
                                                                  {
                                                                     PCB.pointer = (BYTE*)s;
                                                                     PCB.ssx = 0;
                                                                     PCB.la_ptr = (BYTE*)s;
                                                                     PCB.sn = 0; /* do the goto */
                                                                  }
                                                               }

                                                               return 0;
                                                            
#line 2139 "tiny_basic.cpp"
}

static int ag_rp_21(int l) {
#line 275 "C:/Samples/M$DOS/tiny.syn"
                                                               // Save State 
                                                               if (bRun)
                                                               {
                                                                  RetStack[++RetStackPointer].return_line = (char*)PCB.pointer;

                                                                  RetStack[RetStackPointer].laptr = (char*)PCB.la_ptr;
                                                                  RetStack[RetStackPointer].sn    = PCB.sn;
                                                                  RetStack[RetStackPointer].ssx   = PCB.ssx;

                                                                  int   flag = 0;

                                                                  char*    s = FindLine(l,ProgramSpace,&flag);

                                                                  if (flag)
                                                                  {
                                                                     fprintf(stderr,"Could not find line %d\n",l);
                                                                     PCB.exit_flag = AG_SEMANTIC_ERROR_CODE;
                                                                  }
                                                                  else
                                                                  {
                                                                     PCB.pointer = (BYTE*)s;
                                                                     PCB.ssx = 0;
                                                                     PCB.la_ptr = (BYTE*)s;
                                                                     PCB.sn = 0; /* do the goto */
                                                                  }
                                                               }

                                                               return 0;
                                                            
#line 2173 "tiny_basic.cpp"
}

static int ag_rp_22(void) {
#line 307 "C:/Samples/M$DOS/tiny.syn"
                                                               if (bRun)
                                                               {
                                                                  PCB.pointer = (BYTE*)RetStack[RetStackPointer].return_line;
                                                                  PCB.la_ptr  = (BYTE*)RetStack[RetStackPointer].laptr;
                                                                  PCB.sn      = (int)RetStack[RetStackPointer].sn;
                                                                  PCB.ssx     = RetStack[RetStackPointer--].ssx;
                                                               }

                                                               return 0;
                                                            
#line 2188 "tiny_basic.cpp"
}

static int ag_rp_23(void) {
#line 319 "C:/Samples/M$DOS/tiny.syn"
 
                                                               return 0; 
                                                            
#line 2196 "tiny_basic.cpp"
}

static int ag_rp_24(void) {
#line 325 "C:/Samples/M$DOS/tiny.syn"
                                                               if (bRun)
                                                               {
                                                                  int   v = ForStack[ForStackPointer].loopvar->GetIntVal();
                                                                  v += ForStack[ForStackPointer].step;

                                                                  ForStack[ForStackPointer].loopvar->SetVal(v);

                                                                  if (v <= ForStack[ForStackPointer].count)
                                                                  {
                                                                     // mess with input stream
                                                                     PCB.pointer = (BYTE*)ForStack[ForStackPointer].start;
                                                                     PCB.ssx = 0;
                                                                     PCB.la_ptr = PCB.pointer;
                                                                     PCB.sn = 0;
                                                                  }
                                                                  else
                                                                  {
                                                                     ForStackPointer--;
                                                                  }
                                                               }

                                                               return 0;
                                                            
#line 2224 "tiny_basic.cpp"
}

static int ag_rp_25(void) {
#line 350 "C:/Samples/M$DOS/tiny.syn"
 
                                                               return 0; 
                                                            
#line 2232 "tiny_basic.cpp"
}

static int ag_rp_26(void) {
#line 355 "C:/Samples/M$DOS/tiny.syn"
 
                                                               return 0; 
                                                            
#line 2240 "tiny_basic.cpp"
}

static int ag_rp_27(symbol * p, symbol * v) {
#line 360 "C:/Samples/M$DOS/tiny.syn"
 
                                                               DoPoke(p,v,BYTE_POKE);
                                                               return 0; 
                                                            
#line 2249 "tiny_basic.cpp"
}

static int ag_rp_28(symbol * p, symbol * v) {
#line 366 "C:/Samples/M$DOS/tiny.syn"
 
                                                               DoPoke(p,v,WORD_POKE);
                                                               return 0; 
                                                            
#line 2258 "tiny_basic.cpp"
}

static int ag_rp_29(symbol * p, symbol * v) {
#line 372 "C:/Samples/M$DOS/tiny.syn"
 
                                                               DoPoke(p,v,INT_POKE);
                                                               return 0; 
                                                            
#line 2267 "tiny_basic.cpp"
}

static int ag_rp_30(void) {
#line 378 "C:/Samples/M$DOS/tiny.syn"
 
                                                               return 1; 
                                                            
#line 2275 "tiny_basic.cpp"
}

static void ag_rp_31(symbol * s) {
#line 385 "C:/Samples/M$DOS/tiny.syn"
                                                               if (bRun)
                                                               {
                                                                  while (s)
                                                                  {
                                                                     char*    d = getline('?');
                                                                       
                                                                       s->SetVal(strtol(d,NULL,10));
                                                                       s = s->next;
                                                                  }    
                                                               }       
                                                            
#line 2291 "tiny_basic.cpp"
}

static symbol * ag_rp_32(symbol * s) {
#line 398 "C:/Samples/M$DOS/tiny.syn"
  return s;
#line 2297 "tiny_basic.cpp"
}

static symbol * ag_rp_33(symbol * l, symbol * s) {
#line 400 "C:/Samples/M$DOS/tiny.syn"
 
                                                               if (bRun)
                                                               {
                                                                  s->next = l; 
                                                               }

                                                               return s; 
                                                            
#line 2310 "tiny_basic.cpp"
}

static void ag_rp_34(ExpressionList * el) {
#line 411 "C:/Samples/M$DOS/tiny.syn"
  
                                                               ExpressionList*   l = el;

                                                               if (bRun)
                                                               {
                                                                  while (l)
                                                                  {
                                                                     DoPrint(l->v);

                                                                     l = l->next;

                                                                     delete el;
                                                                     el = l;
                                                                  }

                                                                  printf("\n");
                                                               }
                                                            
#line 2333 "tiny_basic.cpp"
}

static ExpressionList * ag_rp_35(symbol * v) {
#line 432 "C:/Samples/M$DOS/tiny.syn"
  
                                                               ExpressionList*   el = new ExpressionList;
                                                               el->v = v;
                                                               return el;
                                                            
#line 2343 "tiny_basic.cpp"
}

static ExpressionList * ag_rp_36(ExpressionList * list, symbol * v) {
#line 440 "C:/Samples/M$DOS/tiny.syn"
                                                               ExpressionList*   el = new ExpressionList;

                                                               ExpressionList*   l = list;

                                                               while(l->next)
                                                               {
                                                                  l = l->next;
                                                               }

                                                               el->v = v;
                                                               l->next = el;
                                                               return list;
                                                            
#line 2361 "tiny_basic.cpp"
}

static symbol * ag_rp_37(symbol * s, symbol * v) {
#line 456 "C:/Samples/M$DOS/tiny.syn"
 
                                                               if (bRun)
                                                               {
                                                                  DoAssign(s,v); 
                                                               }

                                                               return s; 
                                                            
#line 2374 "tiny_basic.cpp"
}

static void ag_rp_38(void) {
#line 471 "C:/Samples/M$DOS/tiny.syn"
 
                                                               bRun = 1; 
                                                            
#line 2382 "tiny_basic.cpp"
}

static void ag_rp_39(symbol * v) {
#line 477 "C:/Samples/M$DOS/tiny.syn"
 
                                                               if (!ConvertToInt(v,1))
                                                               {
                                                                  bRun = 0; 
                                                               }
                                                            
#line 2393 "tiny_basic.cpp"
}

static void ag_rp_40(void) {
#line 486 "C:/Samples/M$DOS/tiny.syn"
 
                                                               if (bRun)
                                                               {
                                                                  bRun = false; 
                                                               }
                                                               else 
                                                               {
                                                                  bRun=true; 
                                                               }
                                                            
#line 2408 "tiny_basic.cpp"
}

static void ag_rp_41(void) {
#line 500 "C:/Samples/M$DOS/tiny.syn"
                                                               bRun = true; 
                                                            
#line 2415 "tiny_basic.cpp"
}

static void ag_rp_42(symbol * s, symbol * limit, symbol * step) {
#line 506 "C:/Samples/M$DOS/tiny.syn"
                                                               if (bRun)
                                                               {
                                                                  DoForLoop(s,limit,step);
                                                               }
                                                            
#line 2425 "tiny_basic.cpp"
}

static symbol * ag_rp_43(void) {
#line 513 "C:/Samples/M$DOS/tiny.syn"
  return MakeIntValue(1);
#line 2431 "tiny_basic.cpp"
}

static symbol * ag_rp_44(symbol * v) {
#line 514 "C:/Samples/M$DOS/tiny.syn"
  return v;
#line 2437 "tiny_basic.cpp"
}

static symbol * ag_rp_45(symbol * v1, symbol * v2) {
#line 518 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoEquals(v1,v2); return s; 
#line 2443 "tiny_basic.cpp"
}

static symbol * ag_rp_46(symbol * v1, symbol * v2) {
#line 519 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoNotEquals(v1,v2); return s; 
#line 2449 "tiny_basic.cpp"
}

static symbol * ag_rp_47(symbol * v1, symbol * v2) {
#line 520 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoLogicalAnd(v1,v2); return s; 
#line 2455 "tiny_basic.cpp"
}

static symbol * ag_rp_48(symbol * v1, symbol * v2) {
#line 521 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoLogicalOr(v1,v2); return s; 
#line 2461 "tiny_basic.cpp"
}

static symbol * ag_rp_49(symbol * v1, symbol * v2) {
#line 522 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoAdd(v1,v2); return s; 
#line 2467 "tiny_basic.cpp"
}

static symbol * ag_rp_50(symbol * v1, symbol * v2) {
#line 523 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoSubtract(v1,v2); return s; 
#line 2473 "tiny_basic.cpp"
}

static symbol * ag_rp_51(symbol * v1, symbol * v2) {
#line 524 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoMult(v1,v2); return s; 
#line 2479 "tiny_basic.cpp"
}

static symbol * ag_rp_52(symbol * v1, symbol * v2) {
#line 525 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoDivide(v1,v2); return s; 
#line 2485 "tiny_basic.cpp"
}

static symbol * ag_rp_53(symbol * v1, symbol * v2) {
#line 526 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoMod(v1,v2); return s; 
#line 2491 "tiny_basic.cpp"
}

static symbol * ag_rp_54(symbol * v1, symbol * v2) {
#line 527 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoGreaterThan(v1,v2); return s; 
#line 2497 "tiny_basic.cpp"
}

static symbol * ag_rp_55(symbol * v1, symbol * v2) {
#line 528 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoLessThan(v1,v2); return s; 
#line 2503 "tiny_basic.cpp"
}

static symbol * ag_rp_56(symbol * v1, symbol * v2) {
#line 529 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoShiftRight(v1,v2); return s; 
#line 2509 "tiny_basic.cpp"
}

static symbol * ag_rp_57(symbol * v1, symbol * v2) {
#line 530 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoShiftLeft(v1,v2); return s; 
#line 2515 "tiny_basic.cpp"
}

static symbol * ag_rp_58(symbol * v1, symbol * v2) {
#line 531 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoBitAnd(v1,v2); return s; 
#line 2521 "tiny_basic.cpp"
}

static symbol * ag_rp_59(symbol * v1, symbol * v2) {
#line 532 "C:/Samples/M$DOS/tiny.syn"
 symbol* s = DoBitOr(v1,v2); return s; 
#line 2527 "tiny_basic.cpp"
}

static symbol * ag_rp_60(symbol * v) {
#line 537 "C:/Samples/M$DOS/tiny.syn"
  
                                                               symbol*  s;
                                                               s = DoNeg(v);
                                                               return s;
                                                            
#line 2537 "tiny_basic.cpp"
}

static symbol * ag_rp_61(symbol * v) {
#line 547 "C:/Samples/M$DOS/tiny.syn"
                                                               symbol* s = NULL;

                                                               if (bRun)
                                                               {
                                                                  s = DoAbs(v);
                                                               }

                                                               return s;
                                                            
#line 2551 "tiny_basic.cpp"
}

static symbol * ag_rp_62(symbol * v) {
#line 558 "C:/Samples/M$DOS/tiny.syn"
                                                               symbol* s = NULL;

                                                               if (bRun)
                                                               {
                                                                  s = DoRandom(v);
                                                               }

                                                               return s;
                                                            
#line 2565 "tiny_basic.cpp"
}

static symbol * ag_rp_63(symbol * v) {
#line 570 "C:/Samples/M$DOS/tiny.syn"
                                                               symbol*  s = DoPeekB(v);
                                                               return s;
                                                            
#line 2573 "tiny_basic.cpp"
}

static symbol * ag_rp_64(symbol * v) {
#line 576 "C:/Samples/M$DOS/tiny.syn"
                                                               symbol*  s = DoPeekW(v);
                                                               return s;
                                                            
#line 2581 "tiny_basic.cpp"
}

static symbol * ag_rp_65(symbol * v) {
#line 582 "C:/Samples/M$DOS/tiny.syn"
                                                               symbol*  s = DoPeekI(v);
                                                               return s;
                                                            
#line 2589 "tiny_basic.cpp"
}

static symbol * ag_rp_66(symbol * s) {
#line 587 "C:/Samples/M$DOS/tiny.syn"
  return s;
#line 2595 "tiny_basic.cpp"
}

static symbol * ag_rp_67(int v) {
#line 590 "C:/Samples/M$DOS/tiny.syn"
                                                               //create temporary symbol for integer value
                                                               symbol* s = new symbol("temp");
                                                               s->SetType(SYMBOLTYPE_TEMPINT);
                                                               s->SetVal(v);
                                                               return s;
                                                            
#line 2606 "tiny_basic.cpp"
}

static symbol * ag_rp_68(symbol * s) {
#line 597 "C:/Samples/M$DOS/tiny.syn"
  return s;
#line 2612 "tiny_basic.cpp"
}

static symbol * ag_rp_69(symbol * v) {
#line 598 "C:/Samples/M$DOS/tiny.syn"
  return v;
#line 2618 "tiny_basic.cpp"
}

static symbol * ag_rp_70(void) {
#line 603 "C:/Samples/M$DOS/tiny.syn"
                                                               symbol*  s = NULL;

                                                               if (bRun)
                                                               {
                                                                  name[name_index] = 0;

                                                                  s = (symbol*)Symbols->findsym(name);

                                                                  if (!s)
                                                                  {
                                                                     s = (symbol*)Symbols->newsym(sizeof(symbol));

                                                                     s->Init();        // fake constructor
                                                                     s->SetName(name);
                                                                     s->SetVal(0l);    //defualt value
                                                                     s->SetType(SYMBOLTYPE_INT);
                                                                     Symbols->addsym(s);
                                                                  }
                                                               }

                                                               return s;
                                                            
#line 2645 "tiny_basic.cpp"
}

static symbol * ag_rp_71(void) {
#line 629 "C:/Samples/M$DOS/tiny.syn"
                                                               symbol*  s = new symbol("temp");

                                                               s->SetType(SYMBOLTYPE_TEMPSTRING);
                                                               name[name_index] = 0;
                                                               s->SetVal(name);
                                                               return s;
                                                            
#line 2657 "tiny_basic.cpp"
}

static void ag_rp_72(void) {
#line 638 "C:/Samples/M$DOS/tiny.syn"
  iins();
#line 2663 "tiny_basic.cpp"
}

static void ag_rp_73(int c) {
#line 639 "C:/Samples/M$DOS/tiny.syn"
  pcn(c);
#line 2669 "tiny_basic.cpp"
}

static void ag_rp_74(int c) {
#line 642 "C:/Samples/M$DOS/tiny.syn"
  ins(c);
#line 2675 "tiny_basic.cpp"
}

static void ag_rp_75(int c) {
#line 643 "C:/Samples/M$DOS/tiny.syn"
  pcn(c);
#line 2681 "tiny_basic.cpp"
}

static int ag_rp_76(int d) {
#line 646 "C:/Samples/M$DOS/tiny.syn"
  return d - '0';
#line 2687 "tiny_basic.cpp"
}

static int ag_rp_77(int n, int d) {
#line 647 "C:/Samples/M$DOS/tiny.syn"
  return 10*n + d - '0';
#line 2693 "tiny_basic.cpp"
}

static int ag_rp_78(int d) {
#line 650 "C:/Samples/M$DOS/tiny.syn"
  return d;
#line 2699 "tiny_basic.cpp"
}


#define READ_COUNTS 
#define WRITE_COUNTS 
#undef V
#define V(i,t) (*t (&(PCB).vs[(PCB).ssx + i]))
#undef VS
#define VS(i) (PCB).vs[(PCB).ssx + i]

#ifndef GET_CONTEXT
#define GET_CONTEXT CONTEXT = (PCB).input_context
#endif

typedef enum {
  ag_action_1,
  ag_action_2,
  ag_action_3,
  ag_action_4,
  ag_action_5,
  ag_action_6,
  ag_action_7,
  ag_action_8,
  ag_action_9,
  ag_action_10,
  ag_action_11,
  ag_action_12
} ag_parser_action;


#ifndef NULL_VALUE_INITIALIZER
#define NULL_VALUE_INITIALIZER = { 0 }
#endif

static tiny_vs_type const ag_null_value NULL_VALUE_INITIALIZER;

static const unsigned char ag_rpx[] = {
    0,  0,  0,  0,  1,  2,  3,  4,  0,  0,  5,  6,  7,  0,  8,  9, 10, 11,
   12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
   30, 31, 32, 33, 34, 35, 36, 37,  0,  0, 38, 39, 40, 41, 42, 43, 44,  0,
   45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,  0, 60,  0,
   61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78
};

static const unsigned char ag_key_itt[] = {
 0
};

static const unsigned short ag_key_pt[] = {
0
};

static const unsigned char ag_key_ch[] = {
    0, 79, 85,255, 83, 84,255, 79,255, 80, 84,255, 70, 78,255, 73, 79,255,
   87, 88,255, 69,255, 66, 73, 87,255, 69,255, 75,255, 79, 82,255, 69, 85,
  255, 65, 84,255, 66, 68, 70, 71, 73, 76, 78, 80, 82, 83,255, 79, 85,255,
   83, 84,255, 79,255, 80, 84,255, 70, 78,255, 73, 79,255, 87, 88,255, 69,
  255, 66, 73, 87,255, 75,255, 69,255, 66, 73, 87,255, 69,255, 75,255, 69,
   79, 82,255, 69, 78, 85,255, 69, 82,255, 65, 84,255, 72, 79,255, 33, 38,
   60, 62, 65, 66, 68, 69, 70, 71, 73, 76, 78, 80, 82, 83, 84,124,255, 79,
   85,255, 83, 84,255, 79,255, 80, 84,255, 70, 78,255, 73, 79,255, 87, 88,
  255, 69,255, 66, 73, 87,255, 69,255, 75,255, 79, 82,255, 69, 85,255, 69,
   82,255, 65, 84,255, 72, 79,255, 33, 38, 60, 62, 66, 68, 69, 70, 71, 73,
   76, 78, 80, 82, 83, 84,124,255, 66, 73, 87,255, 75,255, 69,255, 69,255,
   65, 80, 82,255, 33, 38, 60, 62,124,255, 69,255, 84,255, 72, 79,255, 33,
   38, 60, 62, 69, 83, 84,124,255, 33, 38, 60, 62, 84,124,255, 33, 38, 60,
   62, 69,124,255, 84,255, 83, 84,255, 79,255, 70, 78,255, 73, 79,255, 87,
   88,255, 69,255, 66, 73, 87,255, 69,255, 75,255, 79, 82,255, 69, 85,255,
   66, 68, 70, 71, 73, 76, 78, 80, 82, 83,255, 33, 38, 60, 62, 69, 84,124,
  255, 33, 38, 60, 62, 69, 83,124,255
};

static const unsigned char ag_key_act[] = {
  0,3,3,4,3,3,4,2,4,3,0,4,0,2,4,3,3,4,0,3,4,2,4,0,0,0,4,2,4,2,4,2,3,4,3,
  3,4,3,3,4,3,2,3,2,2,2,2,2,2,2,4,3,3,4,3,3,4,2,4,3,0,4,0,2,4,3,3,4,0,3,
  4,2,4,0,0,0,4,2,4,2,4,0,0,0,4,2,4,2,4,2,2,3,4,3,3,3,4,3,3,4,3,2,4,3,0,
  4,3,3,3,3,3,3,2,3,3,2,2,2,2,2,2,2,2,3,4,3,3,4,3,3,4,2,4,3,0,4,0,2,4,3,
  3,4,0,3,4,2,4,0,0,0,4,2,4,2,4,2,3,4,3,3,4,3,3,4,3,2,4,3,0,4,3,3,3,3,3,
  2,3,3,2,2,2,2,2,2,2,2,3,4,0,0,0,4,2,4,2,4,2,4,3,2,3,4,3,3,3,3,3,4,3,4,
  3,4,3,0,4,3,3,3,3,3,3,2,3,4,3,3,3,3,3,3,4,3,3,3,3,3,3,4,3,4,3,3,4,2,4,
  0,3,4,3,3,4,0,3,4,2,4,0,0,0,4,2,4,2,4,2,3,4,3,3,4,3,3,3,2,2,2,2,2,2,3,
  4,3,3,3,3,3,3,3,4,3,3,3,3,3,3,3,4
};

static const unsigned char ag_key_parm[] = {
    0, 97,103,  0,111,110,  0,  0,  0,120, 96,  0,123,  0,  0,104,109,  0,
  105,113,  0,  0,  0,116,118,117,  0,  0,  0,  0,  0,  0,121,  0,112,106,
    0,108, 98,  0,119,  0,127,  0,  0,  0,  0,  0,  0,  0,  0, 97,103,  0,
  111,110,  0,  0,  0,120, 96,  0,123,  0,  0,104,109,  0,105,113,  0,  0,
    0,145,147,146,  0,  0,  0,  0,  0,116,118,117,  0,  0,  0,  0,  0,  0,
    0,121,  0,112,144,106,  0,128, 98,  0,108,  0,  0,124,126,  0,129,130,
  140,139,143,119,  0,125,127,  0,  0,  0,  0,  0,  0,  0,  0,131,  0, 97,
  103,  0,111,110,  0,  0,  0,120, 96,  0,123,  0,  0,104,109,  0,105,113,
    0,  0,  0,116,118,117,  0,  0,  0,  0,  0,  0,121,  0,112,106,  0,128,
   98,  0,108,  0,  0,124,126,  0,129,130,140,139,119,  0,125,127,  0,  0,
    0,  0,  0,  0,  0,  0,131,  0,145,147,146,  0,  0,  0,  0,  0,  0,  0,
  143,  0,144,  0,129,130,140,139,131,  0,125,  0,124,  0,124,126,  0,129,
  130,140,139,125,128,  0,131,  0,129,130,140,139,124,131,  0,129,130,140,
  139,125,131,  0,126,  0,111,110,  0,  0,  0,123,120,  0,104,109,  0,105,
  113,  0,  0,  0,116,118,117,  0,  0,  0,  0,  0,  0,121,  0,112,106,  0,
  119,103,127,  0,  0,  0,  0,  0,  0,108,  0,129,130,140,139,125,126,131,
    0,129,130,140,139,125,128,131,  0
};

static const unsigned short ag_key_jmp[] = {
    0,  3,  8,  0, 14, 17,  0,  4,  0, 19,  0,  0,  0,  9,  0, 22, 25,  0,
    0, 28,  0, 18,  0,  0,  0,  0,  0, 23,  0, 27,  0, 29, 30,  0, 34, 39,
    0, 41, 44,  0,  0,  1, 11,  7, 12, 15, 21, 31, 34, 37,  0, 63, 68,  0,
   78, 81,  0, 54,  0, 83,  0,  0,  0, 59,  0, 86, 89,  0,  0, 92,  0, 68,
    0,  0,  0,  0,  0, 73,  0, 77,  0,  0,  0,  0,  0, 81,  0, 85,  0, 79,
   87, 94,  0, 98,103,105,  0,110,112,  0,107, 97,  0,116,  0,  0, 49, 51,
   53, 55, 57, 60, 51, 71, 75, 57, 62, 65, 71, 89, 93,100,103,119,  0,132,
  137,  0,147,150,  0,128,  0,152,  0,  0,  0,133,  0,155,158,  0,  0,161,
    0,142,  0,  0,  0,  0,  0,147,  0,151,  0,153,163,  0,167,172,  0,177,
  179,  0,174,161,  0,183,  0,  0,121,123,125,127,129,125,140,144,131,136,
  139,145,155,158,164,167,186,  0,  0,  0,  0,  0,188,  0,192,  0,194,  0,
  188,196,191,  0,194,196,198,200,202,  0,204,  0,208,  0,228,  0,  0,212,
  214,216,218,220,224,212,231,  0,233,235,237,239,241,245,  0,247,249,251,
  253,255,259,  0,261,  0,273,276,  0,240,  0,  0,278,  0,282,285,  0,  0,
  288,  0,251,  0,  0,  0,  0,  0,256,  0,260,  0,262,290,  0,294,299,  0,
  263,266,270,243,245,248,254,264,267,301,  0,305,307,309,311,313,317,319,
    0,321,323,325,327,329,333,337,  0
};

static const unsigned short ag_key_index[] = {
   40,106, 40,170, 40, 40,  0,  0,  0,  0,  0,  0,  0,198,202,198,  0,  0,
  198,208,  0,  0,  0,198,  0,  0,  0,208,  0,  0,210,  0,  0,  0,208,208,
  208,208,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 40,  0,198,
  215,  0,  0,  0,  0,  0,198,198,  0,  0,  0,  0,  0,198,224,231,208,208,
  238,270,270,208,198,198,198,198,198,  0,  0,198,  0,  0,202,198,198,198,
  198,198,215,198,198,198,198,198,198,198,198,198,198,198,198,198,198,198,
  198,198,198,198,198,198,198,198,198,198,198,198,198,198,  0,198,198,270,
  270,281,202,202,202,  0,  0,215,202,202,202,202,202,215,215,215,215,215,
  215,215,215,215,215,215,215,215,215,215,231,289,198,198,198,  0,  0,198,
  198,202,202,202,  0,231
};

static const unsigned char ag_key_ends[] = {
89,69,0, 85,66,76,69,0, 77,80,0, 79,82,0, 85,66,0, 79,0, 
85,84,0, 83,84,0, 65,68,0, 84,0, 73,78,84,0, 84,85,82,78,0, 
78,0, 86,69,0, 82,73,78,71,0, 61,0, 38,0, 60,0, 62,0, 66,83,0, 
89,69,0, 85,66,76,69,0, 77,80,0, 76,83,69,0, 79,82,0, 85,66,0, 
79,0, 85,84,0, 83,84,0, 65,68,0, 84,0, 73,78,84,0, 
84,85,82,78,0, 68,0, 78,0, 86,69,0, 80,0, 73,78,71,0, 69,78,0, 
124,0, 61,0, 38,0, 60,0, 62,0, 89,69,0, 85,66,76,69,0, 
77,80,0, 76,83,69,0, 79,82,0, 85,66,0, 79,0, 85,84,0, 83,84,0, 
65,68,0, 84,0, 73,78,84,0, 84,85,82,78,0, 78,0, 86,69,0, 80,0, 
73,78,71,0, 69,78,0, 124,0, 66,83,0, 78,68,0, 61,0, 38,0, 
60,0, 62,0, 124,0, 76,83,69,0, 72,69,78,0, 61,0, 38,0, 60,0, 
62,0, 76,83,69,0, 84,69,80,0, 69,78,0, 124,0, 61,0, 38,0, 
60,0, 62,0, 72,69,78,0, 124,0, 61,0, 38,0, 60,0, 62,0, 
76,83,69,0, 124,0, 79,0, 89,69,0, 85,77,80,0, 79,82,0, 85,66,0, 
79,0, 80,85,84,0, 83,84,0, 65,68,0, 84,0, 73,78,84,0, 
84,85,82,78,0, 78,0, 65,86,69,0, 61,0, 38,0, 60,0, 62,0, 
76,83,69,0, 79,0, 124,0, 61,0, 38,0, 60,0, 62,0, 76,83,69,0, 
84,69,80,0, 124,0, 
};

#define AG_TCV(x) ag_tcv[(x)]

static const unsigned char ag_tcv[] = {
   28, 92, 92, 92, 92, 92, 92, 92, 92, 26, 24, 92, 92, 25, 92, 92, 92, 92,
   92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 26, 92, 87, 92,
   92,136,141, 92,115,114,134,132, 99,133, 89,135, 91, 91, 91, 91, 91, 91,
   91, 91, 91, 91, 92, 92,138,122,137, 92, 92, 89, 89, 89, 89, 89, 89, 89,
   89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
   89,102, 92,100, 92, 89, 92, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89,
   89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 89, 92,142, 92,
   92,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0
};

#ifndef SYNTAX_ERROR
#define SYNTAX_ERROR fprintf(stderr,"%s, line %d, column %d\n", \
  (PCB).error_message, (PCB).line, (PCB).column)
#endif

#ifndef FIRST_LINE
#define FIRST_LINE 1
#endif

#ifndef FIRST_COLUMN
#define FIRST_COLUMN 1
#endif

#ifndef PARSER_STACK_OVERFLOW
#define PARSER_STACK_OVERFLOW {fprintf(stderr, \
   "\nParser stack overflow, line %d, column %d\n",\
   (PCB).line, (PCB).column);}
#endif

#ifndef REDUCTION_TOKEN_ERROR
#define REDUCTION_TOKEN_ERROR {fprintf(stderr, \
    "\nReduction token error, line %d, column %d\n", \
    (PCB).line, (PCB).column);}
#endif


#ifndef INPUT_CODE
#define INPUT_CODE(T) (T)
#endif

typedef enum
  {ag_accept_key, ag_set_key, ag_jmp_key, ag_end_key, ag_no_match_key,
   ag_cf_accept_key, ag_cf_set_key, ag_cf_end_key} key_words;

static void ag_get_key_word(int ag_k) {
  int ag_save = (int) ((PCB).la_ptr - (PCB).pointer);
  const  unsigned char *ag_p;
  int ag_ch;
  while (1) {
    switch (ag_key_act[ag_k]) {
    case ag_cf_end_key: {
      const  unsigned char *sp = ag_key_ends + ag_key_jmp[ag_k];
      do {
        if ((ag_ch = *sp++) == 0) {
          int ag_k1 = ag_key_parm[ag_k];
          int ag_k2 = ag_key_pt[ag_k1];
          if (ag_key_itt[ag_k2 + CONVERT_CASE(*(PCB).la_ptr)]) goto ag_fail;
          (PCB).token_number = (tiny_token_type) ag_key_pt[ag_k1 + 1];
          return;
        }
      } while (CONVERT_CASE(*(PCB).la_ptr++) == ag_ch);
      goto ag_fail;
    }
    case ag_end_key: {
      const  unsigned char *sp = ag_key_ends + ag_key_jmp[ag_k];
      do {
        if ((ag_ch = *sp++) == 0) {
          (PCB).token_number = (tiny_token_type) ag_key_parm[ag_k];
          return;
        }
      } while (CONVERT_CASE(*(PCB).la_ptr++) == ag_ch);
    }
    case ag_no_match_key:
ag_fail:
      (PCB).la_ptr = (PCB).pointer + ag_save;
      return;
    case ag_cf_set_key: {
      int ag_k1 = ag_key_parm[ag_k];
      int ag_k2 = ag_key_pt[ag_k1];
      ag_k = ag_key_jmp[ag_k];
      if (ag_key_itt[ag_k2 + CONVERT_CASE(*(PCB).la_ptr)]) break;
      ag_save = (int) ((PCB).la_ptr - (PCB).pointer);
      (PCB).token_number = (tiny_token_type) ag_key_pt[ag_k1+1];
      break;
    }
    case ag_set_key:
      ag_save = (int) ((PCB).la_ptr - (PCB).pointer);
      (PCB).token_number = (tiny_token_type) ag_key_parm[ag_k];
    case ag_jmp_key:
      ag_k = ag_key_jmp[ag_k];
      break;
    case ag_accept_key:
      (PCB).token_number = (tiny_token_type) ag_key_parm[ag_k];
      return;
    case ag_cf_accept_key: {
      int ag_k1 = ag_key_parm[ag_k];
      int ag_k2 = ag_key_pt[ag_k1];
      if (ag_key_itt[ag_k2 + CONVERT_CASE(*(PCB).la_ptr)])
        (PCB).la_ptr = (PCB).pointer + ag_save;
      else (PCB).token_number = (tiny_token_type) ag_key_pt[ag_k1+1];
      return;
    }
    }
    ag_ch = CONVERT_CASE(*(PCB).la_ptr++);
    ag_p = &ag_key_ch[ag_k];
    if (ag_ch <= 255) while (*ag_p < ag_ch) ag_p++;
    if (ag_ch > 255 || *ag_p != ag_ch) {
      (PCB).la_ptr = (PCB).pointer + ag_save;
      return;
    }
    ag_k = (int) (ag_p - ag_key_ch);
  }
}


#ifndef AG_NEWLINE
#define AG_NEWLINE 10
#endif

#ifndef AG_RETURN
#define AG_RETURN 13
#endif

#ifndef AG_FORMFEED
#define AG_FORMFEED 12
#endif

#ifndef AG_TABCHAR
#define AG_TABCHAR 9
#endif

static void ag_track(void) {
  int ag_k = (int) ((PCB).la_ptr - (PCB).pointer);
  while (ag_k--) {
    switch (*(PCB).pointer++) {
    case AG_NEWLINE:
      (PCB).column = 1, (PCB).line++;
    case AG_RETURN:
    case AG_FORMFEED:
      break;
    case AG_TABCHAR:
      (PCB).column += (TAB_SPACING) - ((PCB).column - 1) % (TAB_SPACING);
      break;
    default:
      (PCB).column++;
    }
  }
}


static void ag_prot(void) {
  int ag_k;
  ag_k = 128 - ++(PCB).btsx;
  if (ag_k <= (PCB).ssx) {
    (PCB).exit_flag = AG_STACK_ERROR_CODE;
    PARSER_STACK_OVERFLOW;
    return;
  }
  (PCB).bts[(PCB).btsx] = (PCB).sn;
  (PCB).bts[ag_k] = (PCB).ssx;
  (PCB).vs[ag_k] = (PCB).vs[(PCB).ssx];
  (PCB).ss[ag_k] = (PCB).ss[(PCB).ssx];
}

static void ag_undo(void) {
  if ((PCB).drt == -1) return;
  while ((PCB).btsx) {
    int ag_k = 128 - (PCB).btsx;
    (PCB).sn = (PCB).bts[(PCB).btsx--];
    (PCB).ssx = (PCB).bts[ag_k];
    (PCB).vs[(PCB).ssx] = (PCB).vs[ag_k];
    (PCB).ss[(PCB).ssx] = (PCB).ss[ag_k];
  }
  (PCB).token_number = (tiny_token_type) (PCB).drt;
  (PCB).ssx = (PCB).dssx;
  (PCB).sn = (PCB).dsn;
  (PCB).drt = -1;
}


static const unsigned char ag_tstt[] = {
127,123,121,120,119,118,117,116,113,112,111,110,109,108,106,105,104,103,98,
  97,96,91,89,26,0,2,93,94,
26,0,2,
127,123,121,120,119,118,117,116,113,112,111,110,109,108,106,105,104,103,98,
  97,96,91,89,0,1,4,27,29,30,33,101,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,128,127,126,125,124,
  123,122,121,120,119,118,117,116,114,113,112,111,110,109,108,106,105,104,
  103,100,99,98,97,96,91,89,26,25,24,0,2,93,94,
127,123,121,120,119,118,117,116,113,112,111,110,109,108,106,105,104,103,98,
  97,96,89,0,5,31,32,34,36,37,39,43,44,45,46,47,48,49,50,51,52,53,54,55,
  56,57,58,62,63,64,65,67,69,72,75,107,
127,123,121,120,119,118,117,116,113,112,111,110,109,108,106,105,104,103,98,
  97,96,91,89,28,0,4,29,30,33,101,
89,26,0,2,93,94,
89,26,0,2,93,94,
89,26,0,2,93,94,
89,0,5,38,39,107,
89,0,5,35,39,107,
89,0,5,35,39,107,
25,24,0,3,95,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,114,102,99,91,
  89,26,25,24,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
89,26,0,2,93,94,
89,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
125,26,25,24,0,2,93,94,
115,26,0,2,93,94,
115,26,0,2,93,94,
115,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,68,79,80,81,82,
  83,84,85,86,101,107,148,
89,0,5,39,66,107,
89,26,0,2,93,94,
89,0,5,39,49,107,
125,26,25,24,0,2,93,94,
91,26,0,2,93,94,
91,26,0,2,93,94,
124,0,7,73,
122,0,13,
89,26,0,2,93,94,
89,26,0,2,93,94,
125,26,25,24,0,2,93,94,
125,26,25,24,0,2,93,94,
125,26,25,24,0,2,93,94,
125,26,25,24,0,2,93,94,
115,0,59,
115,0,59,
115,0,59,
89,0,5,39,107,
91,0,4,33,101,
91,0,4,33,101,
89,0,5,107,
89,0,5,107,
25,24,0,3,95,
102,0,41,
99,0,40,
99,0,40,
99,0,40,
127,123,121,120,119,118,117,116,113,112,111,110,109,108,106,105,104,103,98,
  97,96,91,89,28,26,0,2,93,94,
142,141,138,137,136,135,134,133,132,122,115,114,102,100,99,92,91,89,87,26,
  25,24,0,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,128,126,125,124,122,
  114,99,26,25,24,0,2,93,94,
115,26,0,2,93,94,
115,26,0,2,93,94,
115,26,0,2,93,94,
115,26,0,2,93,94,
115,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
115,0,59,
115,0,59,
115,0,59,
115,0,59,
115,0,59,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,0,9,10,11,12,13,
  14,15,16,17,18,19,20,21,22,23,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,0,9,10,11,12,13,
  14,15,16,17,18,19,20,21,22,23,
99,0,40,
99,0,40,
126,0,76,
127,123,121,120,119,118,117,116,113,112,111,110,109,108,106,105,104,103,89,
  26,0,2,93,94,
127,123,121,120,119,118,117,116,113,112,111,110,109,108,106,105,104,103,89,
  0,5,31,39,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,62,63,64,65,
  67,69,72,75,107,
125,25,24,0,70,71,74,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
91,26,0,2,93,94,
91,0,4,101,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
89,0,5,39,107,
89,0,5,39,107,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,114,0,9,10,11,
  12,13,14,15,16,17,18,19,20,21,22,23,61,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
136,135,134,0,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
89,0,5,39,107,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
127,123,121,120,119,118,117,116,113,112,111,110,109,108,106,105,104,103,89,
  26,0,2,93,94,
127,123,121,120,119,118,117,116,113,112,111,110,109,108,106,105,104,103,89,
  0,5,31,39,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,62,63,64,65,
  67,69,72,75,107,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,0,9,10,11,12,13,
  14,15,16,17,18,19,20,21,22,23,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,99,0,9,10,11,12,
  13,14,15,16,17,18,19,20,21,22,23,40,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,99,0,9,10,11,12,
  13,14,15,16,17,18,19,20,21,22,23,40,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,99,0,9,10,11,12,
  13,14,15,16,17,18,19,20,21,22,23,40,
100,0,42,
102,0,41,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,128,126,125,124,122,
  114,99,26,25,24,0,2,93,94,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,114,0,9,10,11,
  12,13,14,15,16,17,18,19,20,21,22,23,61,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,114,0,9,10,11,
  12,13,14,15,16,17,18,19,20,21,22,23,61,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,114,0,9,10,11,
  12,13,14,15,16,17,18,19,20,21,22,23,61,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,114,0,9,10,11,
  12,13,14,15,16,17,18,19,20,21,22,23,61,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,114,0,9,10,11,
  12,13,14,15,16,17,18,19,20,21,22,23,61,
141,140,139,136,135,134,133,132,0,9,10,11,12,13,14,15,16,17,18,19,20,21,22,
  23,
140,139,136,135,134,133,132,0,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
136,135,134,133,132,0,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
136,135,134,133,132,0,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
142,141,140,139,136,135,134,133,132,129,122,0,9,10,11,12,13,14,15,16,17,18,
  19,20,21,22,23,
142,141,140,139,136,135,134,133,132,129,122,0,9,10,11,12,13,14,15,16,17,18,
  19,20,21,22,23,
0,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
0,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
0,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
136,135,134,0,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
136,135,134,0,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
142,141,140,139,138,137,136,135,134,133,132,130,129,122,0,9,10,11,12,13,14,
  15,16,17,18,19,20,21,22,23,
142,141,140,139,138,137,136,135,134,133,132,129,122,0,9,10,11,12,13,14,15,
  16,17,18,19,20,21,22,23,
142,141,140,139,136,135,134,133,132,0,9,10,11,12,13,14,15,16,17,18,19,20,21,
  22,23,
142,141,140,139,136,135,134,133,132,0,9,10,11,12,13,14,15,16,17,18,19,20,21,
  22,23,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,0,9,10,11,12,13,
  14,15,16,17,18,19,20,21,22,23,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,128,125,122,25,24,0,
  9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,77,78,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
99,26,25,24,0,2,93,94,
91,0,4,101,
147,146,145,144,143,133,115,91,89,87,26,0,2,93,94,
147,146,145,144,143,133,115,91,89,87,0,4,5,6,8,20,39,59,60,79,80,81,82,83,
  84,85,86,101,107,148,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,114,0,9,10,11,
  12,13,14,15,16,17,18,19,20,21,22,23,61,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,114,0,9,10,11,
  12,13,14,15,16,17,18,19,20,21,22,23,61,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,114,0,9,10,11,
  12,13,14,15,16,17,18,19,20,21,22,23,61,
100,0,42,
142,141,140,139,138,137,136,135,134,133,132,131,130,129,122,0,9,10,11,12,13,
  14,15,16,17,18,19,20,21,22,23,

};


static unsigned const char ag_astt[3083] = {
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,7,1,1,1,9,5,3,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,2,8,7,0,2,1,2,1,2,1,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,10,5,1,5,5,7,
  1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,7,2,1,1,1,1,1,1,2,2,2,2,
  1,1,2,2,1,1,2,2,1,2,2,1,1,1,2,1,1,1,1,1,1,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,2,8,2,7,2,2,1,2,1,5,1,7,1,1,3,5,1,7,1,1,3,5,1,7,1,1,3,2,7,2,1,1,
  1,2,7,2,1,2,1,2,7,2,1,2,1,1,1,7,3,1,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,10,10,1,5,5,7,1,1,3,5,5,5,5,5,5,5,5,5,5,1,7,1,
  1,3,5,1,7,1,1,3,5,1,7,1,1,3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,
  1,1,1,1,1,1,1,5,1,5,5,7,1,1,3,5,1,7,1,1,3,5,1,7,1,1,3,5,1,7,1,1,3,1,1,1,1,
  1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,7,2,2,1,1,5,1,7,1,
  1,3,2,7,2,1,1,1,5,1,5,5,7,1,1,3,5,1,7,1,1,3,5,1,7,1,1,3,1,7,1,1,1,7,1,5,1,
  7,1,1,3,5,1,7,1,1,3,5,1,5,5,7,1,1,3,5,1,5,5,7,1,1,3,5,1,5,5,7,1,1,3,5,1,5,
  5,7,1,1,3,1,7,1,1,7,1,1,7,1,2,7,2,2,1,2,7,2,2,1,2,7,2,2,1,2,7,2,1,2,7,2,1,
  1,1,7,2,1,1,7,1,1,5,1,1,4,1,1,4,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,7,1,1,3,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,2,
  10,10,10,7,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,1,5,5,7,1,1,3,5,1,7,1,1,3,5,1,7,1,1,3,5,1,7,1,1,3,5,1,7,1,1,3,5,1,
  7,1,1,3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,
  5,5,5,5,5,5,5,1,7,1,1,3,1,7,1,1,7,1,1,7,1,1,7,1,1,7,1,1,1,1,1,1,1,1,2,2,2,
  7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,4,1,1,4,1,1,7,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,
  7,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,7,2,2,1,2,2,2,2,1,1,2,2,1,1,
  2,2,1,2,2,1,1,1,2,1,1,1,1,1,1,1,4,4,7,3,3,1,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,
  1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,2,2,
  1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,5,1,7,1,1,3,2,7,1,1,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,2,7,2,
  1,1,2,7,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,
  2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,1,7,1,
  1,3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,
  5,5,5,5,5,1,7,1,1,3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,
  1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,1,1,1,1,1,1,1,2,2,2,7,
  2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,1,1,1,
  1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,
  1,7,1,1,3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,
  5,5,5,5,5,5,5,5,1,7,1,1,3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,1,1,1,1,1,1,1,2,
  2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,
  1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,1,1,1,1,1,1,1,
  2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,1,7,1,1,
  3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,
  5,5,5,5,1,7,1,1,3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
  1,1,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,7,2,2,
  1,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,
  1,1,1,1,1,1,1,1,1,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,7,1,1,3,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,7,2,2,1,2,2,2,2,1,1,2,2,1,1,2,2,1,2,2,1,1,1,
  2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,2,1,7,1,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,5,5,7,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1,4,4,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,1,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,7,2,
  2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,5,1,5,5,7,1,1,3,2,7,1,1,5,5,5,5,5,5,5,
  5,5,5,1,7,1,1,3,1,1,1,1,1,1,1,2,2,2,7,2,2,2,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,7,2,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};


static const unsigned char ag_pstt[] = {
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,1,1,2,
114,116,114,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,87,4,2,0,89,5,5,4,10,3,
115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,
  115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,
  115,115,115,115,115,115,88,115,1,115,115,3,1,1,123,
17,13,15,16,19,20,21,22,25,27,28,29,32,33,34,35,36,37,6,7,8,85,4,81,46,12,
  11,10,9,31,18,19,20,21,45,44,24,25,43,42,28,29,41,31,32,40,39,38,36,24,
  23,30,18,26,14,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,87,4,4,5,89,6,4,10,3,
115,1,6,1,1,120,
115,1,7,1,1,119,
115,1,8,1,1,118,
85,9,81,48,47,14,
85,10,81,49,14,14,
85,11,81,50,14,14,
51,51,12,8,51,
115,115,115,115,115,115,115,115,115,115,1,13,1,1,145,
115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,86,
  86,1,115,115,14,1,1,129,
115,115,115,115,115,115,115,115,115,115,1,15,1,1,143,
115,1,16,1,1,142,
115,1,17,1,1,149,
55,56,57,58,59,61,53,87,85,83,18,78,81,79,52,67,77,60,68,68,68,68,66,65,64,
  63,62,3,14,54,
115,1,115,115,19,1,1,141,
115,1,20,1,1,140,
115,1,21,1,1,139,
115,1,22,1,1,138,
55,56,57,58,59,61,53,87,85,83,23,78,81,79,52,67,77,60,69,70,69,69,69,66,65,
  64,63,62,3,14,54,
85,24,81,38,71,14,
115,1,25,1,1,135,
85,26,81,31,72,14,
115,1,115,115,27,1,1,134,
115,1,28,1,1,133,
115,1,29,1,1,132,
73,30,75,74,
76,31,77,
115,1,32,1,1,131,
115,1,33,1,1,130,
115,1,115,115,34,1,1,128,
115,1,115,115,35,1,1,127,
115,1,115,115,36,1,1,126,
115,1,115,115,37,1,1,125,
53,38,78,
53,39,79,
53,40,80,
85,41,81,30,14,
87,42,89,27,3,
87,43,89,26,3,
85,44,23,14,
85,45,22,14,
51,51,46,7,51,
81,47,82,
83,13,84,
83,12,85,
83,11,85,
115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,
  115,115,115,115,115,1,51,1,1,117,
84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,82,84,84,84,52,
115,115,115,115,115,115,115,115,115,115,1,53,1,1,137,
115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,
  115,115,1,115,115,54,1,1,170,
115,1,55,1,1,169,
115,1,56,1,1,168,
115,1,57,1,1,167,
115,1,58,1,1,166,
115,1,59,1,1,165,
55,56,57,58,59,61,53,87,85,83,60,78,81,79,52,67,77,60,86,86,86,86,66,65,64,
  63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,61,1,1,155,
53,62,87,
53,63,88,
53,64,89,
53,65,90,
53,66,91,
55,56,57,58,59,61,53,87,85,83,67,78,81,79,52,67,77,60,92,92,92,92,66,65,64,
  63,62,3,14,54,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,47,115,117,104,102,
  120,119,94,96,98,100,113,111,110,108,106,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,41,115,117,104,102,
  120,119,94,96,98,100,113,111,110,108,106,
83,40,121,
83,37,122,
123,72,124,
115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,
  1,73,1,1,146,
17,13,15,16,19,20,21,22,25,27,28,29,32,33,34,35,36,37,85,74,81,48,31,18,19,
  20,21,45,44,24,25,43,42,28,29,41,31,32,40,39,38,36,24,23,30,18,26,14,
125,46,46,75,44,45,126,
115,115,115,115,115,115,115,115,115,115,1,76,1,1,144,
55,56,57,58,59,61,53,87,85,83,77,78,81,79,52,67,77,60,127,127,127,127,66,65,
  64,63,62,3,14,54,
55,56,57,58,59,61,53,87,85,83,78,78,81,79,52,67,77,60,128,128,128,128,66,65,
  64,63,62,3,14,54,
55,56,57,58,59,61,53,87,85,83,79,78,81,79,52,67,77,60,129,129,129,129,66,65,
  64,63,62,3,14,54,
55,56,57,58,59,61,53,87,85,83,80,78,81,79,52,67,77,60,130,130,130,130,66,65,
  64,63,62,3,14,54,
115,1,81,1,1,124,
87,82,131,3,
115,115,115,115,115,115,115,115,115,115,1,83,1,1,121,
85,84,81,132,14,
85,85,81,15,14,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,133,86,115,117,104,
  102,120,119,94,96,98,100,113,111,110,108,106,80,
55,56,57,58,59,61,53,87,85,83,87,78,81,79,52,67,77,60,134,134,134,134,66,65,
  64,63,62,3,14,54,
55,56,57,58,59,61,53,87,85,83,88,78,81,79,52,67,77,60,135,135,135,135,66,65,
  64,63,62,3,14,54,
55,56,57,58,59,61,53,87,85,83,89,78,81,79,52,67,77,60,136,136,136,136,66,65,
  64,63,62,3,14,54,
55,56,57,58,59,61,53,87,85,83,90,78,81,79,52,67,77,60,137,137,137,137,66,65,
  64,63,62,3,14,54,
55,56,57,58,59,61,53,87,85,83,91,78,81,79,52,67,77,60,138,138,138,138,66,65,
  64,63,62,3,14,54,
105,107,109,70,115,117,104,102,120,119,94,96,98,100,113,111,110,108,106,
115,115,115,115,115,115,115,115,115,115,1,93,1,1,164,
55,56,57,58,59,61,53,87,85,83,94,78,81,79,52,67,77,60,139,139,139,139,66,65,
  64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,95,1,1,163,
55,56,57,58,59,61,53,87,85,83,96,78,81,79,52,67,77,60,140,140,140,140,66,65,
  64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,97,1,1,162,
55,56,57,58,59,61,53,87,85,83,98,78,81,79,52,67,77,60,141,141,141,141,66,65,
  64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,99,1,1,161,
55,56,57,58,59,61,53,87,85,83,100,78,81,79,52,67,77,60,142,142,142,142,66,
  65,64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,101,1,1,160,
55,56,57,58,59,61,53,87,85,83,102,78,81,79,52,67,77,60,143,143,143,143,66,
  65,64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,103,1,1,159,
55,56,57,58,59,61,53,87,85,83,104,78,81,79,52,67,77,60,144,144,144,144,66,
  65,64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,105,1,1,158,
55,56,57,58,59,61,53,87,85,83,106,78,81,79,52,67,77,60,145,145,145,145,66,
  65,64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,107,1,1,157,
55,56,57,58,59,61,53,87,85,83,108,78,81,79,52,67,77,60,146,146,146,146,66,
  65,64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,109,1,1,156,
55,56,57,58,59,61,53,87,85,83,110,78,81,79,52,67,77,60,147,147,147,147,66,
  65,64,63,62,3,14,54,
55,56,57,58,59,61,53,87,85,83,111,78,81,79,52,67,77,60,148,148,148,148,66,
  65,64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,112,1,1,154,
55,56,57,58,59,61,53,87,85,83,113,78,81,79,52,67,77,60,149,149,149,149,66,
  65,64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,114,1,1,153,
55,56,57,58,59,61,53,87,85,83,115,78,81,79,52,67,77,60,150,150,150,150,66,
  65,64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,116,1,1,152,
55,56,57,58,59,61,53,87,85,83,117,78,81,79,52,67,77,60,151,151,151,151,66,
  65,64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,1,118,1,1,151,
55,56,57,58,59,61,53,87,85,83,119,78,81,79,52,67,77,60,152,152,152,152,66,
  65,64,63,62,3,14,54,
55,56,57,58,59,61,53,87,85,83,120,78,81,79,52,67,77,60,153,153,153,153,66,
  65,64,63,62,3,14,54,
55,56,57,58,59,61,53,87,85,83,121,78,81,79,52,67,77,60,154,154,154,154,66,
  65,64,63,62,3,14,54,
85,122,81,39,14,
115,115,115,115,115,115,115,115,115,115,1,123,1,1,148,
55,56,57,58,59,61,53,87,85,83,124,78,81,79,52,67,77,60,155,155,155,155,66,
  65,64,63,62,3,14,54,
115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,
  1,125,1,1,147,
17,13,15,16,19,20,21,22,25,27,28,29,32,33,34,35,36,37,85,126,81,49,31,18,19,
  20,21,45,44,24,25,43,42,28,29,41,31,32,40,39,38,36,24,23,30,18,26,14,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,43,115,117,104,102,
  120,119,94,96,98,100,113,111,110,108,106,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,83,128,115,117,104,
  102,120,119,94,96,98,100,113,111,110,108,106,156,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,83,129,115,117,104,
  102,120,119,94,96,98,100,113,111,110,108,106,157,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,83,130,115,117,104,
  102,120,119,94,96,98,100,113,111,110,108,106,158,
159,131,16,
81,132,160,
115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,
  115,115,1,115,115,133,1,1,136,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,133,134,115,117,104,
  102,120,119,94,96,98,100,113,111,110,108,106,76,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,133,135,115,117,104,
  102,120,119,94,96,98,100,113,111,110,108,106,75,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,133,136,115,117,104,
  102,120,119,94,96,98,100,113,111,110,108,106,74,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,133,137,115,117,104,
  102,120,119,94,96,98,100,113,111,110,108,106,73,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,133,138,115,117,104,
  102,120,119,94,96,98,100,113,111,110,108,106,72,
95,97,99,105,107,109,61,112,68,115,117,104,102,120,119,94,96,98,100,113,111,
  110,108,106,
97,99,105,107,109,61,112,67,115,117,104,102,120,119,94,96,98,100,113,111,
  110,108,106,
105,107,109,61,112,66,115,117,104,102,120,119,94,96,98,100,113,111,110,108,
  106,
105,107,109,61,112,65,115,117,104,102,120,119,94,96,98,100,113,111,110,108,
  106,
93,95,97,99,105,107,109,61,112,118,76,64,115,117,104,102,120,119,94,96,98,
  100,113,111,110,108,106,
93,95,97,99,105,107,109,61,112,118,76,63,115,117,104,102,120,119,94,96,98,
  100,113,111,110,108,106,
62,115,117,104,102,120,119,94,96,98,100,113,111,110,108,106,
61,115,117,104,102,120,119,94,96,98,100,113,111,110,108,106,
60,115,117,104,102,120,119,94,96,98,100,113,111,110,108,106,
105,107,109,59,115,117,104,102,120,119,94,96,98,100,113,111,110,108,106,
105,107,109,58,115,117,104,102,120,119,94,96,98,100,113,111,110,108,106,
93,95,97,99,101,103,105,107,109,61,112,116,118,76,57,115,117,104,102,120,
  119,94,96,98,100,113,111,110,108,106,
93,95,97,99,101,103,105,107,109,61,112,118,76,56,115,117,104,102,120,119,94,
  96,98,100,113,111,110,108,106,
93,95,97,99,105,107,109,61,112,55,115,117,104,102,120,119,94,96,98,100,113,
  111,110,108,106,
93,95,97,99,105,107,109,61,112,54,115,117,104,102,120,119,94,96,98,100,113,
  111,110,108,106,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,42,115,117,104,102,
  120,119,94,96,98,100,113,111,110,108,106,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,161,51,76,51,51,155,115,
  117,104,102,120,119,94,96,98,100,113,111,110,108,106,50,162,
55,56,57,58,59,61,53,87,85,83,156,78,81,79,52,67,77,60,163,163,163,163,66,
  65,64,63,62,3,14,54,
55,56,57,58,59,61,53,87,85,83,157,78,81,79,52,67,77,60,164,164,164,164,66,
  65,64,63,62,3,14,54,
55,56,57,58,59,61,53,87,85,83,158,78,81,79,52,67,77,60,165,165,165,165,66,
  65,64,63,62,3,14,54,
115,1,115,115,159,1,1,122,
87,160,166,3,
115,115,115,115,115,115,115,115,115,115,1,161,1,1,150,
55,56,57,58,59,61,53,87,85,83,162,78,81,79,52,67,77,60,167,167,167,167,66,
  65,64,63,62,3,14,54,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,133,163,115,117,104,
  102,120,119,94,96,98,100,113,111,110,108,106,35,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,133,164,115,117,104,
  102,120,119,94,96,98,100,113,111,110,108,106,34,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,133,165,115,117,104,
  102,120,119,94,96,98,100,113,111,110,108,106,33,
159,166,17,
93,95,97,99,101,103,105,107,109,61,112,114,116,118,76,52,115,117,104,102,
  120,119,94,96,98,100,113,111,110,108,106,

};


static const unsigned short ag_sbt[] = {
     0,  28,  31,  62, 114, 169, 199, 205, 211, 217, 223, 229, 235, 240,
   255, 282, 297, 303, 309, 339, 347, 353, 359, 365, 396, 402, 408, 414,
   422, 428, 434, 438, 441, 447, 453, 461, 469, 477, 485, 488, 491, 494,
   499, 504, 509, 513, 517, 522, 525, 528, 531, 534, 563, 586, 601, 629,
   635, 641, 647, 653, 659, 689, 704, 707, 710, 713, 716, 719, 749, 780,
   811, 814, 817, 820, 844, 892, 899, 914, 944, 974,1004,1034,1040,1044,
  1059,1064,1069,1102,1132,1162,1192,1222,1252,1271,1286,1316,1331,1361,
  1376,1406,1421,1451,1466,1496,1511,1541,1556,1586,1601,1631,1646,1676,
  1706,1721,1751,1766,1796,1811,1841,1856,1886,1916,1946,1951,1966,1996,
  2020,2068,2099,2132,2165,2198,2201,2204,2232,2265,2298,2331,2364,2397,
  2421,2444,2465,2486,2513,2540,2556,2572,2588,2607,2626,2656,2685,2710,
  2735,2766,2803,2833,2863,2893,2901,2905,2920,2950,2983,3016,3049,3052,
  3083
};


static const unsigned short ag_sbe[] = {
    24,  29,  54, 110, 136, 193, 201, 207, 213, 218, 224, 230, 237, 251,
   278, 293, 299, 305, 319, 343, 349, 355, 361, 375, 397, 404, 409, 418,
   424, 430, 435, 439, 443, 449, 457, 465, 473, 481, 486, 489, 492, 495,
   500, 505, 510, 514, 519, 523, 526, 529, 532, 559, 585, 597, 625, 631,
   637, 643, 649, 655, 669, 700, 705, 708, 711, 714, 717, 729, 764, 795,
   812, 815, 818, 840, 863, 895, 910, 924, 954, 984,1014,1036,1041,1055,
  1060,1065,1085,1112,1142,1172,1202,1232,1255,1282,1296,1327,1341,1372,
  1386,1417,1431,1462,1476,1507,1521,1552,1566,1597,1611,1642,1656,1686,
  1717,1731,1762,1776,1807,1821,1852,1866,1896,1926,1947,1962,1976,2016,
  2039,2083,2115,2148,2181,2199,2202,2228,2248,2281,2314,2347,2380,2405,
  2428,2449,2470,2497,2524,2540,2556,2572,2591,2610,2640,2669,2694,2719,
  2750,2785,2813,2843,2873,2897,2902,2916,2930,2966,2999,3032,3050,3067,3083
};


static const unsigned char ag_fl[] = {
  2,1,1,1,2,1,2,3,3,0,1,2,2,2,1,3,4,6,1,1,1,1,2,2,1,1,2,2,1,1,2,1,1,6,6,
  6,1,2,1,3,2,1,3,3,3,3,0,2,2,2,5,0,2,1,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,
  2,1,4,4,4,4,4,1,1,1,3,1,2,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};

static const unsigned char ag_ptt[] = {
    0, 95, 95,  2,  1, 27, 27, 29, 29, 30, 30, 32, 32, 32, 35, 35, 38, 38,
   31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
   31, 56, 66, 66, 57, 68, 68, 49, 50, 50, 70, 69,  7, 71, 54, 77, 77, 60,
   60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 79, 79, 80,
   80, 80, 80, 80, 80, 81, 81, 81, 81, 39,148,  8,  8,107,107,101,101, 33,
   88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88,
   88, 88, 88, 90, 90, 93, 93, 94, 94,  3, 34, 36, 37, 40, 42,  4, 41, 43,
   44, 45, 46,  5, 47, 48, 51, 52, 53, 55, 61, 59, 58, 62, 63, 64, 65, 67,
   13, 72, 73, 74, 76, 75, 78, 14, 10,  9, 19, 20, 21, 22, 23, 11, 12, 18,
   17, 16, 15, 82, 83, 84, 85, 86,  6
};


static void ag_ra(void)
{
  switch(ag_rpx[(PCB).ag_ap]) {
    case 1: V(0,(int *)) = ag_rp_1(V(0,(int *))); break;
    case 2: V(0,(int *)) = ag_rp_2(V(0,(int *))); break;
    case 3: V(0,(int *)) = ag_rp_3(V(1,(int *))); break;
    case 4: V(0,(int *)) = ag_rp_4(V(1,(int *))); break;
    case 5: ag_rp_5(V(0,(int *))); break;
    case 6: ag_rp_6(V(1,(symbol * *))); break;
    case 7: ag_rp_7(V(1,(symbol * *))); break;
    case 8: V(0,(symbol * *)) = ag_rp_8(V(0,(symbol * *))); break;
    case 9: V(0,(symbol * *)) = ag_rp_9(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 10: V(0,(symbol * *)) = ag_rp_10(V(0,(symbol * *)), V(2,(int *))); break;
    case 11: V(0,(symbol * *)) = ag_rp_11(V(0,(symbol * *)), V(2,(symbol * *)), V(4,(int *))); break;
    case 12: V(0,(int *)) = ag_rp_12(); break;
    case 13: V(0,(int *)) = ag_rp_13(); break;
    case 14: V(0,(int *)) = ag_rp_14(); break;
    case 15: V(0,(int *)) = ag_rp_15(); break;
    case 16: V(0,(int *)) = ag_rp_16(); break;
    case 17: V(0,(int *)) = ag_rp_17(); break;
    case 18: V(0,(int *)) = ag_rp_18(); break;
    case 19: V(0,(int *)) = ag_rp_19(); break;
    case 20: V(0,(int *)) = ag_rp_20(V(1,(int *))); break;
    case 21: V(0,(int *)) = ag_rp_21(V(1,(int *))); break;
    case 22: V(0,(int *)) = ag_rp_22(); break;
    case 23: V(0,(int *)) = ag_rp_23(); break;
    case 24: V(0,(int *)) = ag_rp_24(); break;
    case 25: V(0,(int *)) = ag_rp_25(); break;
    case 26: V(0,(int *)) = ag_rp_26(); break;
    case 27: V(0,(int *)) = ag_rp_27(V(2,(symbol * *)), V(4,(symbol * *))); break;
    case 28: V(0,(int *)) = ag_rp_28(V(2,(symbol * *)), V(4,(symbol * *))); break;
    case 29: V(0,(int *)) = ag_rp_29(V(2,(symbol * *)), V(4,(symbol * *))); break;
    case 30: V(0,(int *)) = ag_rp_30(); break;
    case 31: ag_rp_31(V(1,(symbol * *))); break;
    case 32: V(0,(symbol * *)) = ag_rp_32(V(0,(symbol * *))); break;
    case 33: V(0,(symbol * *)) = ag_rp_33(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 34: ag_rp_34(V(1,(ExpressionList * *))); break;
    case 35: V(0,(ExpressionList * *)) = ag_rp_35(V(0,(symbol * *))); break;
    case 36: V(0,(ExpressionList * *)) = ag_rp_36(V(0,(ExpressionList * *)), V(2,(symbol * *))); break;
    case 37: V(0,(symbol * *)) = ag_rp_37(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 38: ag_rp_38(); break;
    case 39: ag_rp_39(V(1,(symbol * *))); break;
    case 40: ag_rp_40(); break;
    case 41: ag_rp_41(); break;
    case 42: ag_rp_42(V(1,(symbol * *)), V(3,(symbol * *)), V(4,(symbol * *))); break;
    case 43: V(0,(symbol * *)) = ag_rp_43(); break;
    case 44: V(0,(symbol * *)) = ag_rp_44(V(1,(symbol * *))); break;
    case 45: V(0,(symbol * *)) = ag_rp_45(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 46: V(0,(symbol * *)) = ag_rp_46(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 47: V(0,(symbol * *)) = ag_rp_47(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 48: V(0,(symbol * *)) = ag_rp_48(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 49: V(0,(symbol * *)) = ag_rp_49(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 50: V(0,(symbol * *)) = ag_rp_50(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 51: V(0,(symbol * *)) = ag_rp_51(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 52: V(0,(symbol * *)) = ag_rp_52(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 53: V(0,(symbol * *)) = ag_rp_53(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 54: V(0,(symbol * *)) = ag_rp_54(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 55: V(0,(symbol * *)) = ag_rp_55(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 56: V(0,(symbol * *)) = ag_rp_56(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 57: V(0,(symbol * *)) = ag_rp_57(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 58: V(0,(symbol * *)) = ag_rp_58(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 59: V(0,(symbol * *)) = ag_rp_59(V(0,(symbol * *)), V(2,(symbol * *))); break;
    case 60: V(0,(symbol * *)) = ag_rp_60(V(1,(symbol * *))); break;
    case 61: V(0,(symbol * *)) = ag_rp_61(V(2,(symbol * *))); break;
    case 62: V(0,(symbol * *)) = ag_rp_62(V(2,(symbol * *))); break;
    case 63: V(0,(symbol * *)) = ag_rp_63(V(2,(symbol * *))); break;
    case 64: V(0,(symbol * *)) = ag_rp_64(V(2,(symbol * *))); break;
    case 65: V(0,(symbol * *)) = ag_rp_65(V(2,(symbol * *))); break;
    case 66: V(0,(symbol * *)) = ag_rp_66(V(0,(symbol * *))); break;
    case 67: V(0,(symbol * *)) = ag_rp_67(V(0,(int *))); break;
    case 68: V(0,(symbol * *)) = ag_rp_68(V(0,(symbol * *))); break;
    case 69: V(0,(symbol * *)) = ag_rp_69(V(1,(symbol * *))); break;
    case 70: V(0,(symbol * *)) = ag_rp_70(); break;
    case 71: V(0,(symbol * *)) = ag_rp_71(); break;
    case 72: ag_rp_72(); break;
    case 73: ag_rp_73(V(1,(int *))); break;
    case 74: ag_rp_74(V(0,(int *))); break;
    case 75: ag_rp_75(V(1,(int *))); break;
    case 76: V(0,(int *)) = ag_rp_76(V(0,(int *))); break;
    case 77: V(0,(int *)) = ag_rp_77(V(0,(int *)), V(1,(int *))); break;
    case 78: V(0,(int *)) = ag_rp_78(V(0,(int *))); break;
  }
  (PCB).la_ptr = (PCB).pointer;
}

#define TOKEN_NAMES tiny_token_names
const char *const tiny_token_names[149] = {
  "basic",
  "basic",
  "white space",
  "eol",
  "integer",
  "name string",
  "String Value",
  "then part",
  "all character string",
  "\"||\"",
  "\"&&\"",
  "'>'",
  "'<'",
  "'='",
  "\"!=\"",
  "'|'",
  "'&'",
  "\"<<\"",
  "\">>\"",
  "'+'",
  "'-'",
  "'*'",
  "'/'",
  "'%'",
  "'\\n'",
  "'\\r'",
  "",
  "basic statement list",
  "eof",
  "basic line",
  "opt line number",
  "basic statement",
  "declaration",
  "line number",
  "\"INT\"",
  "decl var list",
  "\"DOUBLE\"",
  "\"STRING\"",
  "string decl var list",
  "variable name",
  "','",
  "'['",
  "']'",
  "\"DUMP\"",
  "\"LIST\"",
  "\"NEW\"",
  "\"RUN\"",
  "\"SAVE\"",
  "\"LOAD\"",
  "assignment",
  "conditional",
  "\"GOTO\"",
  "\"GOSUB\"",
  "\"RETURN\"",
  "forloop",
  "\"NEXT\"",
  "input",
  "output",
  "\"POKEB\"",
  "'('",
  "expression",
  "')'",
  "\"POKEW\"",
  "\"POKEI\"",
  "\"BYE\"",
  "\"INPUT\"",
  "variable name list",
  "\"PRINT\"",
  "expression list",
  "if part",
  "nothing",
  "else part",
  "\"IF\"",
  "\"THEN\"",
  "\"ELSE\"",
  "\"FOR\"",
  "\"TO\"",
  "optional step",
  "\"STEP\"",
  "unary expression",
  "postfix expression",
  "primary expression",
  "\"ABS\"",
  "\"RND\"",
  "\"PEEKB\"",
  "\"PEEKW\"",
  "\"PEEKI\"",
  "'\\\"'",
  "all letters",
  "letter",
  "",
  "digit",
  "",
  "",
  "",
  "eol",
  "\"INT\"",
  "\"DOUBLE\"",
  "\"STRING\"",
  "','",
  "']'",
  "integer",
  "'['",
  "\"DUMP\"",
  "\"LIST\"",
  "\"NEW\"",
  "\"RUN\"",
  "name string",
  "\"SAVE\"",
  "\"LOAD\"",
  "\"GOTO\"",
  "\"GOSUB\"",
  "\"RETURN\"",
  "\"NEXT\"",
  "')'",
  "'('",
  "\"POKEB\"",
  "\"POKEW\"",
  "\"POKEI\"",
  "\"BYE\"",
  "\"INPUT\"",
  "\"PRINT\"",
  "'='",
  "\"IF\"",
  "\"THEN\"",
  "\"ELSE\"",
  "\"TO\"",
  "\"FOR\"",
  "\"STEP\"",
  "\"!=\"",
  "\"&&\"",
  "\"||\"",
  "'+'",
  "'-'",
  "'*'",
  "'/'",
  "'%'",
  "'>'",
  "'<'",
  "\">>\"",
  "\"<<\"",
  "'&'",
  "'|'",
  "\"ABS\"",
  "\"RND\"",
  "\"PEEKB\"",
  "\"PEEKW\"",
  "\"PEEKI\"",
  "String Value",

};

#ifndef MISSING_FORMAT
#define MISSING_FORMAT "Missing %s"
#endif
#ifndef UNEXPECTED_FORMAT
#define UNEXPECTED_FORMAT "Unexpected %s"
#endif
#ifndef UNNAMED_TOKEN
#define UNNAMED_TOKEN "input"
#endif


static void ag_diagnose(void) {
  int ag_snd = (PCB).sn;
  int ag_k = ag_sbt[ag_snd];

  if (*TOKEN_NAMES[ag_tstt[ag_k]] && ag_astt[ag_k + 1] == ag_action_8) {
    sprintf((PCB).ag_msg, MISSING_FORMAT, TOKEN_NAMES[ag_tstt[ag_k]]);
  }
  else if (ag_astt[ag_sbe[(PCB).sn]] == ag_action_8
          && (ag_k = (int) ag_sbe[(PCB).sn] + 1) == (int) ag_sbt[(PCB).sn+1] - 1
          && *TOKEN_NAMES[ag_tstt[ag_k]]) {
    sprintf((PCB).ag_msg, MISSING_FORMAT, TOKEN_NAMES[ag_tstt[ag_k]]);
  }
  else if ((PCB).token_number && *TOKEN_NAMES[(PCB).token_number]) {
    sprintf((PCB).ag_msg, UNEXPECTED_FORMAT, TOKEN_NAMES[(PCB).token_number]);
  }
  else if (isprint(INPUT_CODE((*(PCB).pointer))) && INPUT_CODE((*(PCB).pointer)) != '\\') {
    char buf[20];
    sprintf(buf, "\'%c\'", (char) INPUT_CODE((*(PCB).pointer)));
    sprintf((PCB).ag_msg, UNEXPECTED_FORMAT, buf);
  }
  else sprintf((PCB).ag_msg, UNEXPECTED_FORMAT, UNNAMED_TOKEN);
  (PCB).error_message = (PCB).ag_msg;


}
static int ag_action_1_r_proc(void);
static int ag_action_2_r_proc(void);
static int ag_action_3_r_proc(void);
static int ag_action_4_r_proc(void);
static int ag_action_1_s_proc(void);
static int ag_action_3_s_proc(void);
static int ag_action_1_proc(void);
static int ag_action_2_proc(void);
static int ag_action_3_proc(void);
static int ag_action_4_proc(void);
static int ag_action_5_proc(void);
static int ag_action_6_proc(void);
static int ag_action_7_proc(void);
static int ag_action_8_proc(void);
static int ag_action_9_proc(void);
static int ag_action_10_proc(void);
static int ag_action_11_proc(void);
static int ag_action_8_proc(void);


static int (*const  ag_r_procs_scan[])(void) = {
  ag_action_1_r_proc,
  ag_action_2_r_proc,
  ag_action_3_r_proc,
  ag_action_4_r_proc
};

static int (*const  ag_s_procs_scan[])(void) = {
  ag_action_1_s_proc,
  ag_action_2_r_proc,
  ag_action_3_s_proc,
  ag_action_4_r_proc
};

static int (*const  ag_gt_procs_scan[])(void) = {
  ag_action_1_proc,
  ag_action_2_proc,
  ag_action_3_proc,
  ag_action_4_proc,
  ag_action_5_proc,
  ag_action_6_proc,
  ag_action_7_proc,
  ag_action_8_proc,
  ag_action_9_proc,
  ag_action_10_proc,
  ag_action_11_proc,
  ag_action_8_proc
};


static int ag_action_10_proc(void) {
  int ag_t = (PCB).token_number;
  (PCB).btsx = 0, (PCB).drt = -1;
  do {
    ag_track();
    (PCB).token_number = (tiny_token_type) AG_TCV(INPUT_CODE(*(PCB).la_ptr));
    (PCB).la_ptr++;
    if (ag_key_index[(PCB).sn]) {
      unsigned ag_k = ag_key_index[(PCB).sn];
      int ag_ch = CONVERT_CASE(INPUT_CODE(*(PCB).pointer));
      if (ag_ch <= 255) {
        while (ag_key_ch[ag_k] < ag_ch) ag_k++;
        if (ag_key_ch[ag_k] == ag_ch) ag_get_key_word(ag_k);
      }
    }
  } while ((PCB).token_number == (tiny_token_type) ag_t);
  (PCB).la_ptr =  (PCB).pointer;
  return 1;
}

static int ag_action_11_proc(void) {
  int ag_t = (PCB).token_number;

  (PCB).btsx = 0, (PCB).drt = -1;
  do {
    (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).pointer;
    (PCB).ssx--;
    ag_track();
    ag_ra();
    if ((PCB).exit_flag != AG_RUNNING_CODE) return 0;
    (PCB).ssx++;
    (PCB).token_number = (tiny_token_type) AG_TCV(INPUT_CODE(*(PCB).la_ptr));
    (PCB).la_ptr++;
    if (ag_key_index[(PCB).sn]) {
      unsigned ag_k = ag_key_index[(PCB).sn];
      int ag_ch = CONVERT_CASE(INPUT_CODE(*(PCB).pointer));
      if (ag_ch <= 255) {
        while (ag_key_ch[ag_k] < ag_ch) ag_k++;
        if (ag_key_ch[ag_k] == ag_ch) ag_get_key_word(ag_k);
      }
    }
  }
  while ((PCB).token_number == (tiny_token_type) ag_t);
  (PCB).la_ptr =  (PCB).pointer;
  return 1;
}

static int ag_action_3_r_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap] - 1;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  (PCB).btsx = 0, (PCB).drt = -1;
  (PCB).reduction_token = (tiny_token_type) ag_ptt[(PCB).ag_ap];
  ag_ra();
  return (PCB).exit_flag == AG_RUNNING_CODE;
}

static int ag_action_3_s_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap] - 1;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  (PCB).btsx = 0, (PCB).drt = -1;
  (PCB).reduction_token = (tiny_token_type) ag_ptt[(PCB).ag_ap];
  ag_ra();
  return (PCB).exit_flag == AG_RUNNING_CODE;;
}

static int ag_action_4_r_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap] - 1;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  (PCB).reduction_token = (tiny_token_type) ag_ptt[(PCB).ag_ap];
  return 1;
}

static int ag_action_2_proc(void) {
  (PCB).btsx = 0, (PCB).drt = -1;
  if ((PCB).ssx >= 128) {
    (PCB).exit_flag = AG_STACK_ERROR_CODE;
    PARSER_STACK_OVERFLOW;
  }
  (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).pointer;
  (PCB).ss[(PCB).ssx] = (PCB).sn;
  (PCB).ssx++;
  (PCB).sn = (PCB).ag_ap;
  ag_track();
  return 0;
}

static int ag_action_9_proc(void) {
  if ((PCB).drt == -1) {
    (PCB).drt=(PCB).token_number;
    (PCB).dssx=(PCB).ssx;
    (PCB).dsn=(PCB).sn;
  }
  ag_prot();
  (PCB).vs[(PCB).ssx] = ag_null_value;
  (PCB).ss[(PCB).ssx] = (PCB).sn;
  (PCB).ssx++;
  (PCB).sn = (PCB).ag_ap;
  (PCB).la_ptr =  (PCB).pointer;
  return (PCB).exit_flag == AG_RUNNING_CODE;
}

static int ag_action_2_r_proc(void) {
  (PCB).ssx++;
  (PCB).sn = (PCB).ag_ap;
  return 0;
}

static int ag_action_7_proc(void) {
  --(PCB).ssx;
  (PCB).la_ptr =  (PCB).pointer;
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_1_proc(void) {
  ag_track();
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_1_r_proc(void) {
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_1_s_proc(void) {
  (PCB).exit_flag = AG_SUCCESS_CODE;
  return 0;
}

static int ag_action_4_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap] - 1;
  (PCB).reduction_token = (tiny_token_type) ag_ptt[(PCB).ag_ap];
  (PCB).btsx = 0, (PCB).drt = -1;
  (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).pointer;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  else (PCB).ss[(PCB).ssx] = (PCB).sn;
  ag_track();
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (unsigned char)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    (PCB).ag_ap = ag_pstt[ag_t1];
    if ((ag_s_procs_scan[ag_astt[ag_t1]])() == 0) break;
  }
  return 0;
}

static int ag_action_3_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap] - 1;
  (PCB).btsx = 0, (PCB).drt = -1;
  (*(int *) &(PCB).vs[(PCB).ssx]) = *(PCB).pointer;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  else (PCB).ss[(PCB).ssx] = (PCB).sn;
  ag_track();
  (PCB).reduction_token = (tiny_token_type) ag_ptt[(PCB).ag_ap];
  ag_ra();
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (unsigned char)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    (PCB).ag_ap = ag_pstt[ag_t1];
    if ((ag_s_procs_scan[ag_astt[ag_t1]])() == 0) break;
  }
  return 0;
}

static int ag_action_8_proc(void) {
  ag_undo();
  (PCB).la_ptr =  (PCB).pointer;
  (PCB).exit_flag = AG_SYNTAX_ERROR_CODE;
  ag_diagnose();
  SYNTAX_ERROR;
  {(PCB).la_ptr = (PCB).pointer + 1; ag_track();}
  return (PCB).exit_flag == AG_RUNNING_CODE;
}

static int ag_action_5_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap];
  (PCB).btsx = 0, (PCB).drt = -1;
  if (ag_sd) (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  else {
    (PCB).ss[(PCB).ssx] = (PCB).sn;
  }
  (PCB).la_ptr =  (PCB).pointer;
  (PCB).reduction_token = (tiny_token_type) ag_ptt[(PCB).ag_ap];
  ag_ra();
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (unsigned char)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    (PCB).ag_ap = ag_pstt[ag_t1];
    if ((ag_r_procs_scan[ag_astt[ag_t1]])() == 0) break;
  }
  return (PCB).exit_flag == AG_RUNNING_CODE;
}

static int ag_action_6_proc(void) {
  int ag_sd = ag_fl[(PCB).ag_ap];
  (PCB).reduction_token = (tiny_token_type) ag_ptt[(PCB).ag_ap];
  if ((PCB).drt == -1) {
    (PCB).drt=(PCB).token_number;
    (PCB).dssx=(PCB).ssx;
    (PCB).dsn=(PCB).sn;
  }
  if (ag_sd) {
    (PCB).sn = (PCB).ss[(PCB).ssx -= ag_sd];
  }
  else {
    ag_prot();
    (PCB).vs[(PCB).ssx] = ag_null_value;
    (PCB).ss[(PCB).ssx] = (PCB).sn;
  }
  (PCB).la_ptr =  (PCB).pointer;
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbe[(PCB).sn] + 1;
    unsigned ag_t2 = ag_sbt[(PCB).sn+1] - 1;
    do {
      unsigned ag_tx = (ag_t1 + ag_t2)/2;
      if (ag_tstt[ag_tx] < (unsigned char)(PCB).reduction_token) ag_t1 = ag_tx + 1;
      else ag_t2 = ag_tx;
    } while (ag_t1 < ag_t2);
    (PCB).ag_ap = ag_pstt[ag_t1];
    if ((ag_r_procs_scan[ag_astt[ag_t1]])() == 0) break;
  }
  return (PCB).exit_flag == AG_RUNNING_CODE;
}


void init_tiny(void) {
  (PCB).la_ptr = (PCB).pointer;
  (PCB).ss[0] = (PCB).sn = (PCB).ssx = 0;
  (PCB).exit_flag = AG_RUNNING_CODE;
  (PCB).line = FIRST_LINE;
  (PCB).column = FIRST_COLUMN;
  (PCB).btsx = 0, (PCB).drt = -1;
}

void tiny(void) {
  init_tiny();
  (PCB).exit_flag = AG_RUNNING_CODE;
  while ((PCB).exit_flag == AG_RUNNING_CODE) {
    unsigned ag_t1 = ag_sbt[(PCB).sn];
    if (ag_tstt[ag_t1]) {
      unsigned ag_t2 = ag_sbe[(PCB).sn] - 1;
      (PCB).token_number = (tiny_token_type) AG_TCV(INPUT_CODE(*(PCB).la_ptr));
      (PCB).la_ptr++;
      if (ag_key_index[(PCB).sn]) {
        unsigned ag_k = ag_key_index[(PCB).sn];
        int ag_ch = CONVERT_CASE(INPUT_CODE(*(PCB).pointer));
        if (ag_ch <= 255) {
          while (ag_key_ch[ag_k] < ag_ch) ag_k++;
          if (ag_key_ch[ag_k] == ag_ch) ag_get_key_word(ag_k);
        }
      }
      do {
        unsigned ag_tx = (ag_t1 + ag_t2)/2;
        if (ag_tstt[ag_tx] > (unsigned char)(PCB).token_number)
          ag_t1 = ag_tx + 1;
        else ag_t2 = ag_tx;
      } while (ag_t1 < ag_t2);
      if (ag_tstt[ag_t1] != (unsigned char)(PCB).token_number)
        ag_t1 = ag_sbe[(PCB).sn];
    }
    (PCB).ag_ap = ag_pstt[ag_t1];
    (ag_gt_procs_scan[ag_astt[ag_t1]])();
  }
}


