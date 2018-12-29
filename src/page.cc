/* page.cc
 * separates generated assembly into pages, updating the 
 * dispatch tables */

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "cgen.h"

#define PAGE_CMDLEN 256
#define SYMTAB_MAXLEN 256 // max length of line
#define SYM_MAXLEN 128
#define ESTAT_FAILED 1
enum {
   EXITSTAT_SUCCESS = 0,
   EXITSTAT_FAILURE
};

namespace cool {
   extern DispatchTables gCgenDispatchTables;

   
   int PageEmitAssemblySymTab(const char *asm_path, const char *lib_dir) {
      char cmd[256];

      /* emit symbol table using spasm */
      sprintf(cmd, "spasm -DBREAK=\"di \\ halt \\ ei\" -L \"%s\" -I \"%s\"", asm_path, lib_dir);

      if (system(cmd)) {
         perror("system");
         return -1;
      }

      return 0;
   }

   int PageLoadMethodAddresses(const char *symtab_path) {
      std::unordered_map<std::string,unsigned int> symtab;
      FILE *symtabf;
      int retv;

      /* init */
      retv = -1;
      symtabf = NULL;
      
      /* open symbol table file */
      if ((symtabf = fopen(symtab_path, "r")) == NULL) {
         perror("fopen");
         goto cleanup;
      }

      /* load symbols from file into map */
      char sbuf[SYMTAB_MAXLEN];
      while (fgets(sbuf, SYMTAB_MAXLEN, symtabf)) {
         char sym[SYM_MAXLEN];
         unsigned int addr;
         
         if (sscanf(sbuf, "%s = $%x", sym, &addr) < 2) {
            fprintf(stderr, "PageLoadMethodAddresses: invalid symbol table format.\n");
            goto cleanup;
         }
         
         std::string sym2(sym);
         symtab[sym2] = addr;
      }

      for (auto p : gCgenDispatchTables) {
         DispatchTable disptab = p.second;
         Symbol *klass = p.first;
         
         for (auto p : disptab) {
            Symbol *method = p.first;
            DispatchEntry entry = p.second;
            AbsoluteAddress loc = entry.loc_;
         }
      }

      retv = 0;

   cleanup:
      if (symtabf && fclose(symtabf) < 0) {
         retv = -1;
         perror("fclose");
      }

      return retv;
   }

}
