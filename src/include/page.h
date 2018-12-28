#ifndef _PAGE_H
#define _PAGE_H

#include <unordered_map>

namespace cool {
   
   /* classes & types */




   int PageEmitAssemblySymTab(const char *asm_path, const char *lib_dir);
   int PageLoadMethodAddresses(const char *symtab_path);
}

#endif
