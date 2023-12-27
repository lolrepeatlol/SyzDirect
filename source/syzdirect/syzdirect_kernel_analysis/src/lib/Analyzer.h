#ifndef ANALYZER_GLOBAL_H
#define ANALYZER_GLOBAL_H

#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include "llvm/Support/CommandLine.h"
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <tuple>
#include "Common.h"


// 
// typedefs
//
typedef std::pair<Instruction *, BasicBlock *> CFGEdge;
typedef vector< pair<llvm::Module*, llvm::StringRef> > ModuleList;
// Mapping module to its file name.
typedef unordered_map<llvm::Module*, llvm::StringRef> ModuleNameMap;
// The set of all functions.
typedef llvm::SmallPtrSet<llvm::Function*, 8> FuncSet;
// Mapping from function name to function.
typedef unordered_map<string, llvm::Function*> NameFuncMap;
typedef llvm::SmallPtrSet<llvm::CallInst*, 8> CallInstSet;
typedef DenseMap<Function*, CallInstSet> CallerMap;
typedef DenseMap<CallInst *, FuncSet> CalleeMap;
// Pointer analysis types.
typedef DenseMap<Value *, SmallPtrSet<Value *, 16>> PointerAnalysisMap;
typedef unordered_map<Function *, PointerAnalysisMap> FuncPointerAnalysisMap;
typedef unordered_map<Function *, AAResults *> FuncAAResultsMap;

//ErrorPaths analysis types.
typedef std::map<CFGEdge, int> EdgeErrMap;


typedef map<Type*, string> TypeNameMap;

class RefFieldInfo {
public:
    StringRef field_type;
    string field_name;
    CallInst* callsite;
    Function* callFunc;
    Value* refobj;
//    OpType opType;

public:
    RefFieldInfo() = default;

};

typedef map<string,set<RefFieldInfo *>> RefcountFieldInfoMap; // [struct_name, <field_name, field_type, callsiye>]
typedef map<pair<string,string>, map<StringRef, size_t>> RefcountOpTimesMap;
//[(struct_name, field_name), [func_name, op_times]]


struct ConstraintInfo {
	set<pair<uint64_t, string>> switchConstSet;
	set<pair<uint64_t, string>> handlerConstSet;
  set<pair<uint64_t, string>> ifNotConstSet;
	string relateModule;
};

struct TargetSignature {
	string commonSyscall;
	string handler;
	vector<string> functionList;
	vector<vector<string>> blockSigList;
	int rank;
	ConstraintInfo constraintInfo;
};

struct CallTraceInfo {
	vector<pair<Function*, Instruction*>> callTrace;
	int depth;
	int icallNum;
	bool isSyscallEntry;
};

struct GlobalContext {

	GlobalContext() {
		// Initialize statistucs.
		NumSecurityChecks = 0;
		NumCondStatements = 0;
	}

	unsigned NumSecurityChecks;
	unsigned NumCondStatements;
	// Map global types to type_name
	TypeNameMap GlobalTypes;

	// Map global function name to function.
	NameFuncMap GlobalFuncs;

	// Functions whose addresses are taken.
	FuncSet AddressTakenFuncs;

	// Map a callsite to all potential callee functions.
	CalleeMap Callees;

	// Map a function to all potential caller instructions.
	CallerMap Callers;

	// Indirect call instructions.
	std::vector<CallInst *> IndirectCallInsts;

	// Unified functions -- no redundant inline functions
	DenseMap<size_t, Function *> UnifiedFuncMap;
	set<Function *> UnifiedFuncSet;

	// Map function signature to functions
	DenseMap<size_t, FuncSet> sigFuncsMap;

	// Modules.
	ModuleList Modules;
	ModuleNameMap ModuleMaps;
	set<string> InvolvedModules;

	// Functions handling errors
	set<string> ErrorHandleFuncs;
	map<string, tuple<int8_t, int8_t, int8_t>> CopyFuncs;
	DenseMap<Function *, EdgeErrMap> ErrorPathSets;

	// Identified sanity checks
	DenseMap<Function *, set<SecurityCheck>> SecurityCheckSets;
	DenseMap<Function *, set<Value *>> CheckInstSets;

	// Functions labelled
	set<string> GroundTruthFuncs;

	// Allocate function
	set<string> AllocFuncs;

	// Pointer analysis results.
	FuncPointerAnalysisMap FuncPAResults;
	FuncAAResultsMap FuncAAResults;

	map<string, pair<int8_t, int8_t>> DataFetchFuncs;


	map<StringRef,StructType*>StructTypeMap;
	set<StructType*>StructTypeSet;
	map<StringRef,set<StringRef>> embeddedMap;

	// syzlang syscall related
	unordered_set<string> syscallSet;
	unordered_set<string> syscallBaseSet;
	unordered_set<string> syscallBlackList;
	unordered_set<string> syscallVariantSet;

	map<string, map<string, vector<string>>> kernelSig2syscallVariant;

	// register function name -> set of <position of const, position of registered function>
	map<string, set<pair<int, int>>> RegisterFunctionMap;
	// function name -> set of <const number, const name>
	map<string, set<pair<uint64_t, string>>> HandlerConstraint;

	// structure name -> <filed index, field name>
	map<string, map<int, string>> StructFieldInfo;
	// function name -> <set of ConstantStruct, address-taken func index>
	map<string, set<pair<ConstantStruct*, unsigned int>>> AddresstakenFunctionMap;
	// <full file path, function name> -> Function class
	map<pair<string, string>, Function*> FunctionMap;
	map<string, Function*> FunctionNameMap;
	map<Function*, set<Function*>> MissingCallerMap;
	map<Function*, set<Function*>> FPCallerMap;

	map<Function*, map<BasicBlock*,string>> BasicBlockIndexMap;
  // funcname -> module constraint
  map<string,string> Func2ConstFromFopsMap;
};

extern GlobalContext GlobalCtx;

class IterativeModulePass {
protected:
	GlobalContext *Ctx;
	const char * ID;
public:
	IterativeModulePass(GlobalContext *Ctx_, const char *ID_)
		: Ctx(Ctx_), ID(ID_) { }

	// Run on each module before iterative pass.
	virtual bool doInitialization(llvm::Module *M)
		{ return true; }

	// Run on each module after iterative pass.
	virtual bool doFinalization(llvm::Module *M)
		{ return true; }

	// Iterative pass.
	virtual bool doModulePass(llvm::Module *M)
		{ return false; }

	virtual void run(ModuleList &modules);
};

#endif
