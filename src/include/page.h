#ifndef _PAGE_H
#define _PAGE_H

#include <unordered_map>

namespace cool {

#define PAGE_SIZE 0x4000
#define PAGE_CMDLEN 256
#define SYMTAB_MAXLEN 512 // max length of line
#define SYM_MAXLEN 512
#define ESTAT_FAILED 1
   enum {
      EXITSTAT_SUCCESS = 0,
      EXITSTAT_FAILURE
   };
   
   
   int PageEmitAssemblySymTab(const char *asm_path, const char *lib_dir);
   int PageLoadMethodAddresses(const char *symtab_path/*, DispatchTables& disptabs*/);
   void PageReassignPages(/*DispatchTables& disptabs*/);
}

#endif
