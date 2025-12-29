#include "postgres.h"

#include "fmgr.h"
#include "jit/jit.h"
#include "jit/llvmjit.h"
#include "nodes/execnodes.h"
#include "utils/elog.h"

static void tpde_release_context(JitContext *context);
static void tpde_reset_after_error(void);
extern bool tpde_compile_expr(ExprState *state);

PG_MODULE_MAGIC_EXT(
					.name = "tpdejit",
					.version = PG_VERSION
); // include/server/fmgr.h:441


static void tpde_release_context(JitContext *context)
{
}

static void tpde_reset_after_error(void)
{
}

bool tpde_compile_expr(ExprState *state) 
{
	elog(DEBUG1, "%s", "Trying to compile JIT\n");
	
	if (!llvm_compile_expr(state)) {
		elog(ERROR, "%s", "Could not build LLVM IR");
	}
	
	// set state->evalfunc
	// and state->es_jit (?)
	// here

	return false;
}

void _PG_jit_provider_init(JitProviderCallbacks *cb)
{
	cb->reset_after_error = tpde_reset_after_error;
	cb->release_context = tpde_release_context;
	cb->compile_expr = tpde_compile_expr;
}