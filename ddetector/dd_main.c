
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
typedef struct AddrNode_{
  Int addr;
  struct AddrNode_ * next;
} AddrNode;

typedef struct AddrList_ {
  AddrNode* head;
} AddrList;

// add a new address to the list
static void update_addr_list(AddrList* list, Int addr){
  AddrNode* new_node = VG_(malloc)("addr node", sizeof(AddrNode));
  new_node->addr = addr;
  new_node->next = NULL;

  VG_(printf)("created new addr node : addr %x value %x\n", addr, *((HChar*)addr));

  if(list->head == NULL){
    VG_(printf)("setting head for addr %lx\n", addr);
    list->head = new_node;

  }
  else{
    VG_(printf)("head is already set\n");
    AddrNode* curr = list->head;
    while(curr->next != NULL){
      curr = curr->next;
    }

    curr->next = new_node;

  }

}

// free a node
static void free_addr_node(AddrNode* node){

  if(node->next!=NULL){
    free_addr_node(node->next);
  }
  VG_(free)(node);
}

// free the complete list
static void free_addr_list(AddrList* list){
  if(list->head!=NULL){
    VG_(printf)("head is not null\n");
    free_addr_node(list->head);
  }
  
  //VG_(free)(list);
}

//add all the nodes of l2 list to l1
static void merge_addr_lists(AddrList* l1, AddrList* l2){
  
  if(l2->head != NULL){

    AddrNode* curr = l2->head;

    while(curr != NULL){

      update_addr_list(l1, curr->addr);

      curr = curr->next;
    }
  }
}



// ananlysis variables
static AddrList** table;
static AddrList* tempshadow;
static Bool trace = False; // this is true when the analysis is started from main

// aditional instrumentation functions


// get the list of addresses which taints 'addr'
static AddrList* get_shadow_mem(Addr addr){
    Int up = (((addr)&(0xFFFF0000))>>16);
    AddrList* loopkup = table[up];
    AddrList* ret = NULL;
    if(loopkup!=NULL){
        
        Int low = (addr)&(0x0000FFFF);
        ret = &loopkup[low];
    }
    return ret;
}

// set 'addr' as tainted by 'tainted_by'
static void set_shadow_mem(Addr addr, Addr tainted_by){
    VG_(printf)("setting shadow mem for %lx as tainted by %lx\n",addr, tainted_by);
    Int up = (((addr)&(0xFFFF0000))>>16);
    VG_(printf)("up value = %x\n", up);
    AddrList* lookup = table[up];
    // on demand allocation
    if(lookup == NULL){
        VG_(printf)("lookup is NULL\n");
        table[up] = (AddrList*)VG_(malloc)("Memory shadow", 0xFFFF*sizeof(AddrList));
        for(ULong i = 0; i < 0xFFFF; i++){
          table[up][i].head = NULL;
        }
        lookup = table[up];
    }
    Int low = (addr)&(0x0000FFFF);
    update_addr_list(&lookup[low], tainted_by);
}

static AddrList* get_shadow_temp(IRTemp temp){
    return (temp==-1)?NULL:&tempshadow[temp];
}

static void set_shadow_temp(IRTemp temp, AddrList tainted_by){
    tempshadow[temp] = tainted_by;
}

static VG_REGPARM(2) void dd_put_reg(Int offset, Int tmp){

    //VG_(printf)("dd_put call ==> offset = %x, tmp = %x\n", offset, tmp);
    
    ThreadId tid = VG_(get_running_tid)();
    AddrList* shadow_reg = get_shadow_temp(tmp);
    // TODO : verify the sizeof here
    if(shadow_reg!=NULL){
      VG_(set_shadow_regs_area)(tid, 1,offset, sizeof(AddrList*), (const UChar*)(&shadow_reg));
    }
    
}

static VG_REGPARM(2) void dd_get_reg(Int offset, Int tmp){

  //VG_(printf)("dd_get call ==> offset = %x, tmp = %x, type = %x\n", offset, tmp, ty);
  ThreadId tid = VG_(get_running_tid)();
  Int addr_of_addr_list = 0;

  // copy the address of addr_list object to dst
  VG_(get_shadow_regs_area)(tid, (UChar*)&addr_of_addr_list, 1, offset,sizeof(AddrList*));

  // if there is a list update set the shadow temp
  if(addr_of_addr_list != 0){
    //VG_(printf)("setting shadow temp ===========\n");
    set_shadow_temp(tmp, *(AddrList*)addr_of_addr_list);
  }
  
}

static VG_REGPARM(2) void dd_tmp_to_tmp(IRTemp rdtmp, IRTemp wrtmp){
  // rdtmp can not be invalid
  tl_assert(rdtmp!=-1);

  AddrList* addr_list = get_shadow_temp(rdtmp);

  if(addr_list!=NULL){
    set_shadow_temp(wrtmp, *addr_list);
  }

}

// funtions for handling ALU operations
// Qop
static VG_REGPARM(3) void dd_qop_to_tmp(IRTemp arg1, IRTemp arg2, IRTemp arg3, IRTemp arg4, IRTemp wrtmp){
  //VG_(printf)("arg1 = %x, arg2 %x, arg3 = %x, arg4 = %x\n", arg1, arg2, arg3, arg4);
}

// Binop
static VG_REGPARM(3) void dd_binop_to_tmp(IRTemp arg1, IRTemp arg2, IRTemp wrtmp){

  //VG_(printf)("arg1 = %x , arg2 = %x\n", arg1, arg2); 
  AddrList* addr_list_arg1 = get_shadow_temp(arg1);
  AddrList* addr_list_arg2 = get_shadow_temp(arg2);

  AddrList* addr_list_wrtmp = get_shadow_temp(wrtmp);

  if(addr_list_arg1!=NULL){
    //VG_(printf)("MERGING1\n");
    merge_addr_lists(/* list1 = */addr_list_wrtmp, /*list2 = */addr_list_arg1);
  }

  if(addr_list_arg2 != NULL){
    //VG_(printf)("MERGING2\n");
    merge_addr_lists(/* list1 = */addr_list_wrtmp, /*list2 = */addr_list_arg2);
  }

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
            VG_(printf)("read %d %x %d\n",args[0], args[1],args[2]);
            Int i;
            for(i=0; i < args[2];i++){
              VG_(printf)("setting addr %lx as tainted by addr %lx\n", args[1]+i, args[1]+i);
              set_shadow_mem(args[1]+i, args[1]+i);
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

            // put some value in to guest register
            if(trace){
              VG_(printf)("Ist_Put\n");
              Int offset = st->Ist.Put.offset;
              IRExpr* data = st->Ist.Put.data;

              VG_(printf)("offset = %x , data = %lx, data tag = %x\n", offset, (SizeT)data, data->tag);
              
              IRExpr** argv = mkIRExprVec_2(mkIRExpr_HWord((HWord)offset),
                      mkIRExpr_HWord( (HWord) (data->tag == Iex_RdTmp)?(data->Iex.RdTmp.tmp):-1));
              
              dirty = unsafeIRDirty_0_N(2, "dd_put_reg", VG_(fnptr_to_fnentry)(dd_put_reg), argv);
              
              addStmtToIRSB(sbOut,IRStmt_Dirty(dirty)); 

            
            }
            addStmtToIRSB(sbOut, st);
            break;
            

        case Ist_PutI:
            if(trace){
              VG_(printf)("Ist_PutI\n");
            }
            addStmtToIRSB(sbOut, st);
            break;
        case Ist_WrTmp:
            // writes a value to a temp variable
            if(trace){
              VG_(printf)("Ist_WrTmp\n");
              IRTemp wrtmp = st->Ist.WrTmp.tmp;
              IRExpr* data = st->Ist.WrTmp.data;
              // handle different types of IR expressions
              switch(data->tag){
                case Iex_Get:
                {
                  // guest registers must be 16 bits or 32 bits
                  // IRType ty = data->Iex.Get.ty;
                  // tl_assert(ty == Ity_I32 || ty == Ity_I16);
                  
                  
                  Int offset = data->Iex.Get.offset;
                  //VG_(printf)("data = %lx, type = %x, offet = %x\n", (SizeT)data, data->Iex.Get.ty, offset);

                  IRExpr** argv = mkIRExprVec_2(mkIRExpr_HWord((HWord)offset),
                    mkIRExpr_HWord((HWord)wrtmp));

                  dirty = unsafeIRDirty_0_N(2, "dd_get_reg", VG_(fnptr_to_fnentry)(dd_get_reg), argv);
                  addStmtToIRSB(sbOut, IRStmt_Dirty(dirty));
                  break;

                }
                case Iex_GetI:
                case Iex_RdTmp:
                {
                  //VG_(printf)("Temp to temp assignment\n");
                  IRTemp rdtmp = data->Iex.RdTmp.tmp;
                  IRExpr** argv = mkIRExprVec_2(mkIRExpr_HWord((HWord)rdtmp), mkIRExpr_HWord((HWord)wrtmp));
                  dirty = unsafeIRDirty_0_N(2, "dd_tmp_to_tmp", VG_(fnptr_to_fnentry)(dd_tmp_to_tmp), argv);
                  addStmtToIRSB(sbOut, IRStmt_Dirty(dirty));
                  break;
                }
                case Iex_Qop:
                {
                  //VG_(printf)("Q operation\n");
                  IRExpr* arg1 = data->Iex.Qop.details->arg1;
                  IRExpr* arg2 = data->Iex.Qop.details->arg2;
                  IRExpr* arg3 = data->Iex.Qop.details->arg3;
                  IRExpr* arg4 = data->Iex.Qop.details->arg4;

                  IRTemp tmp1, tmp2, tmp3, tmp4;

                  tmp1 = (arg1!=NULL)? arg1->Iex.RdTmp.tmp: -1;
                  tmp2 = (arg2!=NULL)? arg2->Iex.RdTmp.tmp: -1;
                  tmp3 = (arg3!=NULL)? arg3->Iex.RdTmp.tmp: -1;
                  tmp4 = (arg4!=NULL)? arg4->Iex.RdTmp.tmp: -1;



                  IRExpr** argv = mkIRExprVec_5(
                    mkIRExpr_HWord((HWord)tmp1),
                    mkIRExpr_HWord((HWord)tmp2),
                    mkIRExpr_HWord((HWord)tmp3),
                    mkIRExpr_HWord((HWord)tmp4),
                    mkIRExpr_HWord((HWord)wrtmp));

                  dirty = unsafeIRDirty_0_N(3, "dd_qop_to_tmp", VG_(fnptr_to_fnentry)(dd_qop_to_tmp), argv);
                  addStmtToIRSB(sbOut, IRStmt_Dirty(dirty));
                  break;


                }
                case Iex_Triop:
                {
                  VG_(printf)("Tri operation\n");
                  break;
                }
                case Iex_Binop:
                {
                  VG_(printf)("Binary operation\n");
                  IRExpr* arg1 = data->Iex.Binop.arg1;
                  IRExpr* arg2 = data->Iex.Binop.arg2;

                  //VG_(printf)("arg1 tag %x\n", arg1->tag);
                  //VG_(printf)("arg2 tag %x\n", arg2->tag);

                  IRTemp tmp1, tmp2;

                  tmp1 = (arg1->tag == Iex_RdTmp)? arg1->Iex.RdTmp.tmp: -1;
                  tmp2 = (arg2->tag == Iex_RdTmp)? arg2->Iex.RdTmp.tmp: -1;

                  IRExpr** argv = mkIRExprVec_3(
                    mkIRExpr_HWord((HWord)tmp1),
                    mkIRExpr_HWord((HWord)tmp2),
                    mkIRExpr_HWord((HWord)wrtmp));

                  dirty = unsafeIRDirty_0_N(3, "dd_binop_to_tmp", VG_(fnptr_to_fnentry)(dd_binop_to_tmp), argv);
                  addStmtToIRSB(sbOut, IRStmt_Dirty(dirty));

                  break;
                }
                case Iex_Unop:
                {
                  VG_(printf)("Unary operation\n");
                  //TODO:
                  break;
                }
                case Iex_Load:
                {
                  VG_(printf)("Load operation\n");
                  break;
                }
                case Iex_Const:
                case Iex_ITE:
                case Iex_CCall:
                case Iex_VECRET:
                case Iex_BBPTR:
                default: break;
              }
            }
            addStmtToIRSB(sbOut, st);
            break;
        case Ist_Store:
            if(trace){
              VG_(printf)("Ist_Store\n");
            }
            addStmtToIRSB(sbOut, st);
            break;
        case Ist_LoadG:
            if(trace){
              VG_(printf)("Ist_LoadG\n");
            }
            addStmtToIRSB(sbOut, st);
            break;
        case Ist_StoreG:
            if(trace){
              VG_(printf)("Ist_StoreG\n");
            }
            addStmtToIRSB(sbOut, st);
            break;
        case Ist_CAS:
            if(trace){
              VG_(printf)("Ist_CAS\n");
            }
            addStmtToIRSB(sbOut, st);
            break;
        case Ist_LLSC:
            if(trace){
              VG_(printf)("Ist_LLSC\n");
            }
            addStmtToIRSB(sbOut, st);
            break;
        case Ist_Dirty:
            if(trace){
              VG_(printf)("Ist_Dirty\n");
            }
            addStmtToIRSB(sbOut, st);
            break;
        case Ist_MBE:
            if(trace){
              VG_(printf)("Ist_MBE\n");
            }
            addStmtToIRSB(sbOut, st);
            break;
        case Ist_Exit:
            if(trace){
              VG_(printf)("Ist_Exit\n");
            }
            addStmtToIRSB(sbOut, st);
            break;

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
      VG_(printf)("table[%lx] is not null\n", i );
      //free_addr_list(table[i]);

      for(ULong j=0; j<0xFFFF; j++){
        if(table[i][j].head!=NULL){
          VG_(printf)("head at %lx is not null\n", j);
          free_addr_list(&table[i][j]);
        }
      }
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
   // this is used for memory
   table = (AddrList**)VG_(malloc)("Memory shadow", 0xFFFF*sizeof(AddrList*));
   
   //initialize to NULL
   for(ULong i = 0; i < 0xFFFF; i++){
     table[i] = NULL;
   }

   // this is used for temporary variables
   tempshadow = (AddrList*)VG_(malloc)("Temp shadow", 0xFFFF*sizeof(AddrList));

   // set head to NULL
   for(ULong i = 0; i < 0xFFFF; i++){
    tempshadow[i].head = NULL;
   }

}






VG_DETERMINE_INTERFACE_VERSION(dd_pre_clo_init)

/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
