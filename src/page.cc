/* page.cc
 * separates generated assembly into pages, updating the 
 * dispatch tables */

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <numeric>
#include "cgen.h"
#include "page.h"


namespace cool {
   extern DispatchTables gCgenDispatchTables;


   void CgenKlassTable::CgenSymbolTable(const char *asm_path, const char *lib_dir) {
      char cmd[256];

      /* emit symbol table using spasm */
      sprintf(cmd, "spasm -DBREAK=\"di \\ halt \\ ei\" -L \"%s\" -I \"%s\"", asm_path, lib_dir);

      if (system(cmd)) {
         perror("system");
         throw "assembler error";
      }

      /* format symbol table path */
      std::string symtab_path(asm_path);
      size_t ext_index;
      if ((ext_index = symtab_path.rfind(".", symtab_path.rfind("/"))) != std::string::npos) {
         symtab_path = symtab_path.substr(0, ext_index);
      }
      symtab_path += std::string(".lab");
      
      /* load symbol table */
      FILE *symtabf;
      /* open symbol table file */
      if ((symtabf = fopen(symtab_path.c_str(), "r")) == NULL) {
         perror("fopen");
         throw "cannot open symbol table file";
      }

      /* load symbols from file into map */
      char sbuf[SYMTAB_MAXLEN];
      while (fgets(sbuf, SYMTAB_MAXLEN, symtabf)) {
         char sym[SYM_MAXLEN];
         unsigned int addr;
         
         if (sscanf(sbuf, "%s = $%x", sym, &addr) < 2) {
            fprintf(stderr, "CgenSymbolTable: invalid symbol table format.\n");
            if (fclose(symtabf) < 0) {
               perror("fclose");
            }
            throw "symbol table parse error";
         }
         
         std::string sym2(sym);
         symtab_[sym2] = addr;
      }
      
      /* close symbol table file */
      if (fclose(symtabf) < 0) {
         perror("fclose");
         throw "could not close symbol table file";
      }
   
      /* load dispatch symbols into dispatch entries */
      root()->LoadDispatchSymbols(symtab_);
}

   void CgenNode::LoadDispatchSymbols(const AsmSymbolTable& symtab) {
      /* load this node's dispatch symbols */
      dispTab_.LoadDispatchSymbols(symtab);

      /* load children's dispatch symbols */
      for (auto child : children_) {
         child->LoadDispatchSymbols(symtab);
      }
   }

   void DispatchTable::LoadDispatchSymbols(const AsmSymbolTable& symtab) {
      for (std::pair<Symbol*,DispatchEntry&> p : *this) {
            DispatchEntry& entry = p.second;
            entry.LoadDispatchSymbol(symtab);
      }
   }

   void DispatchEntry::LoadDispatchSymbol(const AsmSymbolTable& symtab) {
      std::string full_method = klass_->value() + std::string(METHOD_SEP) 
         + method_->value();
      
      /* convert full method string to uppercase */
      for (char &c : full_method) {
         c = toupper(c);
      }
      
      /* locate method in map */
      auto it = symtab.find(full_method);
      if (it == symtab.end()) {
         std::cerr << "page: symbol " << full_method << " not found in symbol table."
                   << std::endl;
         throw "symbol not found";
      }
      
      /* get address & set in dispent */
      addr_ = it->second;
   }
   
   int PageLoadMethodAddresses(const char *symtab_path/*, DispatchTables& disptabs*/) {
      DispatchTables& disptabs = gCgenDispatchTables;
      typedef std::unordered_map<std::string,unsigned int> AsmSymbolTable;
      AsmSymbolTable symtab;
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

      /* set addresses in dispatch table entries */
      for (std::pair<Symbol*,DispatchTable&> p : disptabs) {
         DispatchTable& disptab = p.second;

         for (std::pair<Symbol*,DispatchEntry&> p : disptab) {
            DispatchEntry& entry = p.second;
            const Symbol *klass = entry.klass_;
            const Symbol *method = entry.method_;
            std::string full_method = klass->value() + std::string(METHOD_SEP) 
               + method->value();
            
            /* convert full method string to uppercase */
            for (char &c : full_method) {
               c = toupper(c);
            }

            /* locate method in map */
            auto it = symtab.find(full_method);
            if (it == symtab.end()) {
               std::cerr << "page: symbol " << full_method << " not found in symbol table."
                         << std::endl;
               throw "symbol not found";
            }

            /* get address & set in dispent */
            unsigned int addr = it->second;
            entry.addr_ = addr;
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

   void PageReassignPages(/*DispatchTables& disptabs*/) {
      DispatchTables& disptabs = gCgenDispatchTables;
      
      /* create list of all dispatch entries */
      std::vector<DispatchEntry> entry_vec;
      for (auto p : disptabs) {
         DispatchTable disptab = p.second;
         for (auto p : disptab) {
            DispatchEntry dispent = p.second;
            entry_vec.push_back(dispent);
         }
      }

      /* sort list of dispatch entries */
      std::sort(entry_vec.begin(), entry_vec.end(), 
                [](const DispatchEntry& lhs, const DispatchEntry& rhs){
                   return lhs.addr_ < rhs.addr_;
                });


      /* reassign addresses */
      unsigned int addr_off = 0;
      unsigned int page = 0;

      std::vector<DispatchEntry>::iterator dispent_current = entry_vec.begin();
      std::vector<DispatchEntry>::iterator dispent_next;
      if (dispent_current != entry_vec.end()) {
         for (dispent_next = dispent_current + 1; dispent_next != entry_vec.end();
              ++dispent_next, ++dispent_current) {
            unsigned int& cur_addr = dispent_current->addr_;
            unsigned int& next_addr = dispent_next->addr_;

            /* ensure length of current dispatch entry is no greater than one page */
            if (next_addr - cur_addr > PAGE_SIZE) {
               fprintf(stderr, "page: method %s" METHOD_SEP "%s is too long (%ud bytes).\n",
                       dispent_current->klass_->value().c_str(), 
                       dispent_current->method_->value().c_str(), next_addr - cur_addr);
               throw std::pair<std::string,DispatchEntry>(std::string("method too long"),
                                                          *dispent_current);
            }

            
            if ((next_addr + addr_off - 1) / PAGE_SIZE > page) {
               /* move current dispatch entry onto next page */
               ++page;
               addr_off += page * PAGE_SIZE - cur_addr;
            }
            
            cur_addr += addr_off;
         }
      }

      /* print result */
      disptabs.print_entries();
   }

   
   
}
