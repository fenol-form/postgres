/// llvmjit.c adjusted for TPDE

#include "postgres.h"
#include "jit/tpdejit.h"

#include <llvm-c/Types.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Support.h>
#include <llvm-c/Target.h>

// typedef struct LLVMJitHandle
// {
// 	LLVMOrcLLJITRef lljit;
// 	LLVMOrcResourceTrackerRef resource_tracker;
// } LLVMJitHandle; 
// что-то свое должно быть ?

/* types & functions commonly needed for JITing */
LLVMTypeRef TypeSizeT;
LLVMTypeRef TypeDatum;
LLVMTypeRef TypeParamBool;
LLVMTypeRef TypeStorageBool;
LLVMTypeRef TypePGFunction;
LLVMTypeRef StructNullableDatum;
LLVMTypeRef StructHeapTupleData;
LLVMTypeRef StructMinimalTupleData;
LLVMTypeRef StructTupleDescData;
LLVMTypeRef StructTupleTableSlot;
LLVMTypeRef StructHeapTupleHeaderData;
LLVMTypeRef StructHeapTupleTableSlot;
LLVMTypeRef StructMinimalTupleTableSlot;
LLVMTypeRef StructMemoryContextData;
LLVMTypeRef StructFunctionCallInfoData;
LLVMTypeRef StructExprContext;
LLVMTypeRef StructExprEvalStep;
LLVMTypeRef StructExprState;
LLVMTypeRef StructAggState;
LLVMTypeRef StructAggStatePerGroupData;
LLVMTypeRef StructAggStatePerTransData;
LLVMTypeRef StructPlanState;

LLVMValueRef AttributeTemplate;
LLVMValueRef ExecEvalSubroutineTemplate;
LLVMValueRef ExecEvalBoolSubroutineTemplate;
