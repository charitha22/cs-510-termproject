
/*--------------------------------------------------------------------*/
/*--- Nulgrind: The minimal Valgrind tool.               dd_main.c ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of Nulgrind, the minimal Valgrind tool,
   which does no instrumentation or analysis.

   Copyright (C) 2002-2015 Nicholas Nethercote
      njn@valgrind.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_mallocfree.h"

#include "pub_tool_libcassert.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_debuginfo.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_options.h"
#include "pub_tool_machine.h" 
#include "vki/vki-scnums-x86-linux.h"
#include "pub_tool_threadstate.h"

// data structure to store address information
typedef struct AddrList_{
  Int * addr_list;
} AddrList;

// ananlysis variables
static Bool** table;
static Bool* tempshadow;
static Bool trace = False; // this is true when the analysis is started from main

// aditional instrumentation functions


static Bool get_shadow_mem(Addr addr){
    Int up = (((addr)&(0xFFFF0000))>>16);
    Bool* loopkup = table[up];
    Bool ret = False;
    if(loopkup!=NULL){
        
        Int low = (addr)&(0x0000FFFF);
        ret = loopkup[low];
    }
    return ret;
}

static void set_shadow_mem(Addr addr, Bool value){
    //VG_(printf)("setting shadow mem for %x\n",addr);
    Int up = (((addr)&(0xFFFF0000))>>16);
    Bool* loopkup = table[up];
    // on demand allocation
    if(loopkup == NULL){
        table[up] = (Bool*)VG_(malloc)("Memory shadow", 0xFFFF*sizeof(Bool));
        loopkup = table[up];
    }
    Int low = (addr)&(0x0000FFFF);
    loopkup[low] = value;
}

static Bool get_shadow_temp(IRTemp temp){
    return (temp==-1)?False:tempshadow[temp];
}

static void set_shadow_temp(IRTemp temp, Bool value){
    tempshadow[temp] = value;
}

static VG_REGPARM(2) void dd_put(Int offset, Int tmp){
    
    ThreadId tid = VG_(get_running_tid)();
    Bool shadow_reg = get_shadow_temp(tmp);
    VG_(set_shadow_regs_area)(tid, 1,offset, 1, &shadow_reg);
}

//syscall handlers
static void dd_pre_call(ThreadId tid, UInt syscallno,
                                    UWord* args, UInt nArgs){

}

static void dd_post_call(ThreadId tid, UInt syscallno,
                                    UWord* args, UInt nArgs, SysRes res){
    if(trace){
        if(syscallno==__NR_read){
            // read syscall args : fd, buffer address, size 
            //VG_(printf)("read %d %x %d\n",args[0], args[1],args[2]);
            Int i;
            for(i=0; i < args[2];i++){

                set_shadow_mem(args[1]+i, True);
            }
        }
    }
}

static void dd_post_clo_init(void)
{
}


static
IRSB* dd_instrument ( VgCallbackClosure* closure,
                      IRSB* sbIn,
                      const VexGuestLayout* layout, 
                      const VexGuestExtents* vge,
                      const VexArchInfo* archinfo_host,
                      IRType gWordTy, IRType hWordTy )
{
  
  Int        i;
  IRSB*      sbOut;
  IRDirty*   dirty;

  if (gWordTy != hWordTy) {
    /* We don't currently support this case. */
    VG_(tool_panic)("host/guest word size mismatch");
  }

  /* Set up SB */
  sbOut = deepCopyIRSBExceptStmts(sbIn);

  // Copy verbatim any IR preamble preceding the first IMark
  i = 0;
  while (i < sbIn->stmts_used && sbIn->stmts[i]->tag != Ist_IMark) {
    addStmtToIRSB( sbOut, sbIn->stmts[i] );
    i++;
  }

  for(;i < sbIn->stmts_used; i++){
    IRStmt* st = sbIn->stmts[i];
    
    if (!st || st->tag == Ist_NoOp) continue;

    const HChar* fnname;
    switch(st->tag){
        case Ist_IMark:

            if(VG_(get_fnname_if_entry)(st->Ist.IMark.addr, &fnname)){
                if(VG_(strcmp)(fnname, "main")==0){
                    trace = True;
                }
                else if(VG_(strcmp)(fnname, "exit")==0){
                    trace = False;
                }
            }
            addStmtToIRSB(sbOut, st);
            break;
        case Ist_Put:
            if(trace){
                Int offset = st->Ist.Put.offset;
                IRExpr* data = st->Ist.Put.data;

                VG_(printf)("offset = %x , data = %lu, data tag = %d\n", offset, (SizeT)data, data->tag);
                
                IRExpr** argv = mkIRExprVec_2(mkIRExpr_HWord((HWord)offset),
                        mkIRExpr_HWord( (HWord) (data->tag == Iex_RdTmp)?(data->Iex.RdTmp.tmp):-1));
                
                dirty = unsafeIRDirty_0_N(2, "dd_put", VG_(fnptr_to_fnentry)(dd_put), argv);
                
                addStmtToIRSB(sbOut,IRStmt_Dirty(dirty)); 
            
            }
            addStmtToIRSB(sbOut, st);
            break;
            

        case Ist_PutI:
        case Ist_WrTmp:
        case Ist_Store:
        case Ist_LoadG:
        case Ist_StoreG:
        case Ist_CAS:
        case Ist_LLSC:
        case Ist_Dirty:
        case Ist_MBE:
        case Ist_Exit:

        default: addStmtToIRSB( sbOut, st );    
    }

    

  }

  return sbOut;
}



static void dd_fini(Int exitcode)
{
  // free memory
  VG_(free)(tempshadow);
  for(ULong i=0; i<0xFFFF; i++){
    if(table[i]!=NULL){
      VG_(free)(table[i]);
    }
  }
  VG_(free)(table);

}

static void dd_pre_clo_init(void)
{
   VG_(details_name)            ("ddtector");
   VG_(details_version)         (NULL);
   VG_(details_description)     ("dynamic data dependance detector");
   VG_(details_copyright_author)(
      "Copyright (C) 2002-2015, and GNU GPL'd, by Charitha Saumya.");
   VG_(details_bug_reports_to)  (VG_BUGS_TO);

   VG_(details_avg_translation_sizeB) ( 275 );

   VG_(basic_tool_funcs)        (dd_post_clo_init,
                                 dd_instrument,
                                 dd_fini);

   //sys calls
   VG_(needs_syscall_wrapper)(dd_pre_call, dd_post_call);

   // We assume 32 bit programs
   table = (Bool**)VG_(malloc)("Memory shadow", 0xFFFF*sizeof(Bool*));
   tempshadow = (Bool*)VG_(malloc)("Temp shadow", 0xFFFF*sizeof(Bool));

   for(ULong i=0; i<0xFFFF;i++){
      table[i] = NULL;
      tempshadow[i] = False;
   }

}






VG_DETERMINE_INTERFACE_VERSION(dd_pre_clo_init)

/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
