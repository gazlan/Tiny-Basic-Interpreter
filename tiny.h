#ifndef _TINY_HPP_
#define _TINY_HPP_

#define BYTE_POKE                   (0)
#define WORD_POKE                   (1)
#define INT_POKE                    (2)

typedef struct BUCKET 
{
   struct BUCKET*    next;
   struct BUCKET**   prev;
}
BUCKET;

class hashtab 
{
   int         tabsize;
   int         numsyms;
   BUCKET**    table;
   DWORD       hash(BYTE* name);

public:

   hashtab(int size);
   ~hashtab();

   void*    newsym(int size);
   void*    addsym(void* isym);
   void*    findsym(char* isym);
   void     delsym(void* isym);
   void*    nextsym(void* ilast);
   void     dump(FILE* o);
};

struct string 
{
   int      size;
   char*    s;
};

#define SYMBOLTYPE_INT              (1)
#define SYMBOLTYPE_TEMPINT          (2)
#define SYMBOLTYPE_DOUBLE           (3)
#define SYMBOLTYPE_TEMPDOUBLE       (4)
#define SYMBOLTYPE_STRING           (5)
#define SYMBOLTYPE_TEMPSTRING       (6)

class symbol 
{
   char name[32];
   int type;

public:

   union
   {
      int      lv;
      double   dv;
   }   
   value;

   char*    sv;
   symbol*  next;

    symbol(char *s);  //constructor
   ~symbol();        //destructor

   void     Init();  //fake constructor
   void     SetVal(int v);
   int      GetIntVal();
   void     SetVal(double v);
   double   GetDoubleVal();
   void     SetName(char* s);
   void     PrintInfo(FILE* o);
   void     SetType(int t);
   int      GetType();
   void     SetVal(symbol* v);
   char*    GetStringVal();
   int      GetStringLen();
   void     SetMaxStringLen(int d);
   void     SetVal(char* s);
};

struct FOR_STACK 
{
   char*    start;   // start of loop 
   int      count;
   int      step;
   symbol*  loopvar;
};

struct SubroutineStack 
{
   char*    return_line;
   char*    laptr;
   int      ssx; 
   int      sn;
};

class ExpressionList 
{
   public:

      symbol*              v;
      ExpressionList*      next;

   public:

      ExpressionList() 
      { 
         next = NULL; 
      }
};

#endif
