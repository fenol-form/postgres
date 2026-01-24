#ifndef TPDEJIT_H
#define TPDEJIT_H

#if defined(USE_TPDE)

#include <llvm-c/Types.h>
#include "access/tupdesc.h"
#include "fmgr.h"
#include "jit/jit.h"
#include "nodes/pg_list.h"
#include "jit/llvmjit.h"
#include "jit/llvmjit_emit.h"

/*
 * File needs to be includable by both C and C++ code, and include other
 * headers doing the same. Therefore wrap C portion in our own extern "C" if
 * in C++ mode.
 */
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct TPDECompiledExprState
{
	LLVMJitContext *context;
	const char *funcname;
} TPDECompiledExprState;

void ResourceOwnerRememberJIT(ResourceOwner owner, LLVMJitContext *handle);
void ResourceOwnerForgetJIT(ResourceOwner owner, LLVMJitContext *handle);


void tpde_create_compiler(const char * llvm_triple);

extern bool llvm_build_ir(struct ExprState *state, struct LLVMJitContext* context);

extern LLVMJitContext *llvm_create_context(int jitFlags);
void llvm_release_context(JitContext* context);
void llvm_compile_module(LLVMJitContext *context);
void tpde_add_llvm_ir_module(LLVMJitContext* context);
void create_target_machine(const char* triple, const char* cpu, const char* features);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif							/* USE_TPDE*/
#endif	                        /* TPDEJIT_H*/