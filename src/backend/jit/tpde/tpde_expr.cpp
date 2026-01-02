extern "C"
{
#include "c.h"
#include "postgres.h"

#include "fmgr.h"
#include "jit/jit.h"
#include "jit/llvmjit.h"
#include "jit/tpdejit.h"
#include "nodes/execnodes.h"
#include "utils/elog.h"
}

static void tpde_release_context(JitContext *context);
static void tpde_reset_after_error(void);
static ExprStateEvalFunc tpde_get_compiled_expr(TPDECompiledExprState* cstate);
extern bool tpde_compile_expr(ExprState *state);


static LLVMJitContext* retrieve_or_init_jit_context(ExprState* state) 
{
	LLVMJitContext *context = NULL;

	llvm_enter_fatal_on_oom();

	if (state->parent->state->es_jit)
		context = (LLVMJitContext *) state->parent->state->es_jit;
	else
	{
		context = llvm_create_context(state->parent->state->es_jit_flags);
		state->parent->state->es_jit = &context->base;
	}
	return context;
}

// static 

static void tpde_release_context(JitContext *context)
{
}

static void tpde_reset_after_error(void)
{
}

bool tpde_compile_expr(ExprState *state) 
{
	elog(DEBUG1, "%s", "Trying to compile JIT\n");
	
	// some initialization
	// TODO: переименовать llvm_compile_expr во что-то более подходящее типа build_ir
	//       выпилить оттуда всё, что не связано с switch(opcode) (инициализация сессии и непосрелственно кодген, оставить это здесь)

	// context / session initialization
	LLVMJitContext* context = retrieve_or_init_jit_context(state);

	// building llvm IR
	if (!llvm_build_ir(state, context)) {
		elog(ERROR, "%s", "Could not build LLVM IR");
		return false;
	}
	
	TPDECompiledExprState *cstate = (TPDECompiledExprState*)(state->evalfunc_private);
	Assert(cstate);
	// compile and link to binary via tpde
	ExprStateEvalFunc func = tpde_get_compiled_expr(cstate);
	Assert(func);
	state->evalfunc = func;

	return true;
}

static ExprStateEvalFunc tpde_get_compiled_expr(TPDECompiledExprState* cstate)
{
	elog(FATAL, "CODE EMITTING IS NOT IMPLEMENTED SO FAR");
	// Not implemented
	return NULL;
}

void _PG_jit_provider_init(JitProviderCallbacks *cb)
{
	cb->reset_after_error = tpde_reset_after_error;
	cb->release_context = tpde_release_context;
	cb->compile_expr = tpde_compile_expr;
}