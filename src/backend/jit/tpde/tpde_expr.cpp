#include <cstdint>
#include <llvm-19/llvm/ADT/StringExtras.h>
#include <llvm-19/llvm/ExecutionEngine/JITSymbol.h>
#include <llvm-19/llvm/ExecutionEngine/Orc/Core.h>
#include <llvm-19/llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm-19/llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm-19/llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm-19/llvm/ExecutionEngine/Orc/Layer.h>
#include <llvm-19/llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h>
#include <llvm-19/llvm/ExecutionEngine/Orc/Shared/ExecutorAddress.h>
#include <llvm-19/llvm/ExecutionEngine/Orc/Shared/ExecutorSymbolDef.h>
#include <llvm-19/llvm/ExecutionEngine/Orc/SymbolStringPool.h>
#include <llvm-19/llvm/IR/DataLayout.h>
#include <llvm-19/llvm/IR/Module.h>
#include <llvm-19/llvm/Support/DynamicLibrary.h>
#include <llvm-19/llvm/Support/Error.h>
#include <llvm-19/llvm/Target/TargetMachine.h>
#include <llvm-19/llvm/TargetParser/Triple.h>
#include <llvm-c-19/llvm-c/LLJIT.h>
#include <llvm-19/llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm-c-19/llvm-c/TargetMachine.h>
#include <llvm-c-19/llvm-c/Types.h>
#include <memory>
#include <string_view>
#include "tpde/tpde-llvm/include/tpde-llvm/OrcCompiler.hpp"
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

static bool lljit_initialized = false;
static std::unique_ptr<llvm::orc::LLJIT> lljit;


static void tpde_release_context(JitContext *context);
static void tpde_reset_after_error(void);
static ExprStateEvalFunc tpde_codegen(TPDECompiledExprState* cstate);
extern bool tpde_compile_expr(ExprState *state);

/*
 * Attempt to resolve symbol, so LLVM can emit a reference to it.
 */
static llvm::orc::ExecutorAddr llvm_resolve_symbol(std::string_view symname, void *ctx) {
	uintptr_t	addr;
	char	   *funcname;
	char	   *modname;

	/*
	 * macOS prefixes all object level symbols with an underscore. But neither
	 * dlsym() nor PG's inliner expect that. So undo.
	 */
#if defined(__darwin__)
	if (symname[0] != '_')
		elog(ERROR, "expected prefixed symbol name, but got \"%s\"", symname);
	symname++;
#endif

	llvm_split_symbol_name(symname.data(), &modname, &funcname);

	/* functions that aren't resolved to names shouldn't ever get here */
	Assert(funcname);

	if (modname)
		addr = (uintptr_t) load_external_function(modname, funcname,
												  true, NULL);
	else
		addr = (uintptr_t) llvm::sys::DynamicLibrary::SearchForAddressOfSymbol(symname.data());
		// addr = (uintptr_t) LLVMSearchForAddressOfSymbol(symname);

	pfree(funcname);
	if (modname)
		pfree(modname);

	/* let LLVM will error out - should never happen */
	if (!addr)
		elog(WARNING, "failed to resolve name %s", symname.data());
	
	return llvm::orc::ExecutorAddr(addr);
}

class CustomResolveSymbolGenerator : public llvm::orc::DefinitionGenerator {
public:
	llvm::Error tryToGenerate(
		llvm::orc::LookupState &LS, llvm::orc::LookupKind K, llvm::orc::JITDylib &JD,
		llvm::orc::JITDylibLookupFlags JDLookupFlags,
		const llvm::orc::SymbolLookupSet &LookupSet
	) final {
		llvm::orc::SymbolMap symbols;

		for (auto sym_it = LookupSet.begin(); sym_it != LookupSet.end(); ++sym_it) {
			std::string_view name = *sym_it->first;
			// LLVMOrcRetainSymbolStringPoolEntry(LookupSet[i].Name);
			auto addr = llvm_resolve_symbol(name, NULL);
			symbols[sym_it->first] = llvm::orc::ExecutorSymbolDef(addr, llvm::JITSymbolFlags::Exported);
		}
		auto mu = std::make_unique<llvm::orc::AbsoluteSymbolsMaterializationUnit>(symbols);
		return JD.define(std::move(mu));
	}
};


void tpde_create_compiler(/*TargetMachine but it should be NULL for TPDE*/) {
	using namespace llvm;
	using namespace tpde_llvm;
	ExitOnError ExitOnErr;
	auto builder = orc::LLJITBuilder();
	assert(!lljit_initialized);

	builder.CreateCompileFunction = [](orc::JITTargetMachineBuilder jtmb) 
		-> Expected<std::unique_ptr<orc::IRCompileLayer::IRCompiler>>
	{
		return std::make_unique<OrcCompiler>(jtmb.getTargetTriple());
		/* OrcCompiler requires target machine to be NULL, so dont bother with it's creation*/
	};

	// see what builder takes at llvmjit.c:1220 :
	// builder.CreateObjectLinkingLayer = []()
	builder.CreateObjectLinkingLayer = [](orc::ExecutionSession& ES, const Triple& triple)
		-> Expected<std::unique_ptr<orc::ObjectLayer>>
	{
		return std::make_unique<orc::ObjectLinkingLayer>(ES);
		// see llvmjit.c:1172 there is event listeners definition for JIT debugging
	};
	lljit = ExitOnErr(builder.create());

	lljit->getExecutionSession().setErrorReporter([](Error e) {
		auto strerr = llvm::toString(std::move(e));
		elog(WARNING, "error during JITing: %s", strerr.c_str());
	});

	auto& main_jit_dylib = lljit->getMainJITDylib();
	char global_prefix = lljit->getDataLayout().getGlobalPrefix();

	auto main_gen = llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(global_prefix);
	main_jit_dylib.addGenerator(ExitOnErr(std::move(main_gen)));

	auto ref_gen = std::make_unique<CustomResolveSymbolGenerator>();
	main_jit_dylib.addGenerator(std::move(ref_gen));

	lljit_initialized = true;
}


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

	llvm_leave_fatal_on_oom();

	return context;
}

// static 

static void tpde_release_context(JitContext *context)
{
	llvm_release_context(context);
	// ... ?
}

static void tpde_reset_after_error(void)
{
	llvm_reset_after_error();
	// ... ?
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
	ExprStateEvalFunc func = tpde_codegen(cstate);
	Assert(func);
	state->evalfunc = func;

	return true;
}

static ExprStateEvalFunc tpde_codegen(TPDECompiledExprState* cstate)
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