

#ifndef DFSAN_INSTR_PASS_HPP
#define DFSAN_INSTR_PASS_HPP

#include "ParameterFinder.hpp"
#include "DebugInfo.hpp"

#include <llvm/ADT/Optional.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>

#include <fstream>
#include <unordered_set>
#include <unordered_map>

#include <nlohmann/json.hpp>

class DebugInfo;

namespace llvm {
    class Function;
    class CallGraph;
    class LoopInfo;
}

namespace extrap {

    struct Statistics
    {
        int functions_count;
        int empty_functions;
        int functions_checked;
        int calls_to_check;

        Statistics():
            functions_count(0),
            empty_functions(0),
            functions_checked(0),
            calls_to_check(0)
        {}

        void found_function();
        void empty_function();
        void label_function(int labels);
        void print();
    };

    struct FileIndex
    {
        typedef std::map<llvm::StringRef,std::tuple<int, llvm::StringRef>> idx_t;
        typedef typename idx_t::iterator iterator;
        // file_name -> (idx, dir)
        idx_t index;

        void import(llvm::Module &, DebugInfo &);
        int getIdx(llvm::StringRef &);
        iterator begin();
        iterator end();
    };

    struct Instrumenter
    {
        llvm::Module & m;
        llvm::IRBuilder<> builder;
        DebugInfo info;
        FileIndex file_index;
        //std::unordered_map<Parameters::id_t, llvm::GlobalVariable> allocated;
        size_t functions_count;
        size_t params_count;

        // `parameters` dfsan labels, dynamically assigned at runtime
        llvm::GlobalVariable * glob_labels;
        llvm::GlobalVariable * glob_files;
        
        llvm::GlobalVariable * glob_funcs_count;
        // `functions` C strings, assigned at compile time 
        llvm::GlobalVariable * glob_funcs_names;
        // `functions` ints, storing # of args
        llvm::GlobalVariable * glob_funcs_args;
        // 2*`functions` integers, line of code and file index, compile time
        llvm::GlobalVariable * glob_funcs_dbg;
        llvm::GlobalVariable * glob_params_count;
        // `params` C strings, assigned at compile time 
        llvm::GlobalVariable * glob_params_names;

        // Callsites
        // int8* of size operand_count * callsite_count
        // computed for each function
        llvm::GlobalVariable * glob_callsites_result;
        // int32* of size `functions
        // stores offsets to the array above
        // necessary because each function has different signature
        // and different number of callsites
        llvm::GlobalVariable * glob_callsites_offsets;
        
        // `functions` * `parameters` integers, storing control-flow dependency
        // of a function on each parameter
        llvm::GlobalVariable * glob_result_array;

        static constexpr const char * glob_labels_name
            = "__EXTRAP_INSTRUMENTATION_LABELS";
        static constexpr const char * glob_files_name
            = "__EXTRAP_INSTRUMENTATION_FILES";
        static constexpr const char * glob_funcs_count_name
            = "__EXTRAP_INSTRUMENTATION_FUNCS_COUNT";
        static constexpr const char * glob_params_count_name
            = "__EXTRAP_INSTRUMENTATION_PARAMS_COUNT";
        static constexpr const char * glob_result_array_name
            = "__EXTRAP_INSTRUMENTATION_RESULTS";
        static constexpr const char * glob_funcs_args_name
            = "__EXTRAP_INSTRUMENTATION_FUNCS_ARGS";
        static constexpr const char * glob_funcs_names_name
            = "__EXTRAP_INSTRUMENTATION_FUNCS_NAMES";
        static constexpr const char * glob_funcs_dbg_name
            = "__EXTRAP_INSTRUMENTATION_FUNCS_DBG";
        static constexpr const char * glob_callsites_result_name
            = "__EXTRAP_INSTRUMENTATION_CALLSITES_RESULTS";
        static constexpr const char * glob_callsites_offsets_name
            = "__EXTRAP_INSTRUMENTATION_CALLSITES_OFFSETS";
        static constexpr const char * glob_params_names_name
            = "__EXTRAP_INSTRUMENTATION_PARAMS_NAMES";

        // __EXTRAP_CHECK_LABEL(int * addr, function_idx)
        llvm::Function * load_function;
        // __EXTRAP_STORE_LABEL(int * addr, param_idx)
        llvm::Function * store_function;
        // __EXTRAP_CHECK_CALLSITE(int * addr, size, func_idx, arg_idx, site_idx)
        llvm::Function * callsite_function;
        // __EXTRAP_AT_EXIT()
        llvm::Function * at_exit_function;
        // __EXTRAP_INIT()
        llvm::Function * init_function;

        Instrumenter(llvm::Module & _m):
            m(_m),
            builder(m.getContext()),
            functions_count(0),
            params_count(0),
            glob_labels(nullptr),
            glob_files(nullptr),
            glob_funcs_count(nullptr),
            glob_funcs_names(nullptr),
            glob_funcs_args(nullptr),
            glob_funcs_dbg(nullptr),
            glob_params_count(nullptr),
            glob_params_names(nullptr),
            glob_callsites_result(nullptr),
            glob_callsites_offsets(nullptr),
            glob_result_array(nullptr)
        {
            file_index.import(m, info); 
            declareFunctions();
        }

        // insert a call atexit(__EXTRAP__AT_EXIT)
        void initialize(llvm::Function * main);
        void declareFunctions();
        template<typename Vector, typename FuncIter>
        void createGlobalStorage(const Vector & func_names,
                FuncIter begin, FuncIter end);
        void checkLabel(int function_idx, llvm::BranchInst * br);
        void callCheckLabel(int function_idx, size_t size, llvm::Value * cast);
       
        void annotateParams(const std::vector< std::tuple<const llvm::Value *, Parameters::id_t> > & params); 
        void setLabel(Parameters::id_t param, const llvm::Value * val);
        void callSetLabel(int param_idx, const char * param_name,
                size_t size, llvm::Value * operand);
        void setInsertPoint(llvm::Instruction & inst);
        llvm::Instruction * createGlobalStringPtr(const char * name, llvm::Instruction * placement);

        void checkCall(int function_idx, int callsite_idx, llvm::CallBase *);
        void callCheckCallArg(int function_idx, int callsite_idx, int arg_idx,
                uint64_t size, llvm::Value * ptr);

        llvm::Function * getAtExit();
    };
    
    struct LabelAnnotator : public llvm::InstVisitor<LabelAnnotator, bool>
    {
        static std::set<Parameters::id_t> annotated_params;
        Instrumenter & instr;
        int param_idx;
        LabelAnnotator(Instrumenter & _instr, int idx):
            instr(_instr),
            param_idx(idx)
            {}
        //struct load
        bool visitLoadInst(llvm::LoadInst &);
        bool visitAllocaInst(llvm::AllocaInst &);
        bool visitInstruction(llvm::Instruction &);
        bool visitBitCastInst(llvm::BitCastInst &);
        // struct load
        bool visitGetElementPtrInst(llvm::GetElementPtrInst &);
        static bool visited(Parameters::id_t);
    };

    struct InstrumenterVisiter : public llvm::InstVisitor<InstrumenterVisiter, void>
    {
        llvm::DataLayout * layout;
        Instrumenter & instr;
        bool avoid_duplicates;
        // avoid duplicates
        llvm::SmallSet<llvm::LoadInst*, 10> processed_loads;
        std::unordered_set<llvm::PHINode*> phis;
        //TODO: std::function overhead?
        std::function<void(uint64_t, llvm::Value*)> f;

        template<typename F>
        InstrumenterVisiter(Instrumenter & _instr, F && _f,
                bool _avoid_duplicates = true):
            layout(new llvm::DataLayout(&_instr.m)),
            instr(_instr),
            f(_f),
            avoid_duplicates(_avoid_duplicates)
            {}
        ~InstrumenterVisiter()
        {
            delete layout;
        }
        void visitLoadInst(llvm::LoadInst &);
        void visitInstruction(llvm::Instruction &);
        void visitPHINode(llvm::PHINode &);

        uint64_t size_of(llvm::Value * val);
    };

    struct DfsanInstr : public llvm::ModulePass
    {
        static char ID;
        std::fstream log;
        llvm::Module * m;
        llvm::CallGraph * cgraph;
        llvm::LoopInfo * linfo;
        FunctionParameters parameters;

        struct Function
        {
            int idx;
            bool overriden;
            llvm::SmallVector<llvm::Value*, 10> callsites;

            Function(int _idx, bool _overriden = false):
                idx(_idx),
                overriden(_overriden)
            {}

            int function_idx()
            {
                return idx;
            }

            void add_callsite(llvm::Value* val)
            {
                callsites.push_back(val);
            }

            size_t callsites_size()
            {
                return callsites.size();
            }

            bool is_overriden()
            {
                return overriden;
            }

        };

        // Insert null value when function is not importan
        // Important function:
        // a) has non-const loops
        // b) has call to OpenMP fork-call
        // c) has MPI call?
        //
        std::unordered_map<llvm::Function *, llvm::Optional<Function>> instrumented_functions;
        std::vector<llvm::Function *> parent_functions;
        int instrumented_functions_counter;
        std::ofstream unknown;
        Statistics stats;
        // TODO: something smarter here
        // first traversal of functions cannot 
        std::vector< std::tuple<const llvm::Value*, Parameters::id_t>> found_params;

        //Statistics stats;
        DfsanInstr():
            ModulePass(ID),
            m(nullptr),
            cgraph(nullptr),
            linfo(nullptr),
            instrumented_functions_counter(0),
            unknown(std::ofstream("unknown", std::ios::out))
        {
        }

        ~DfsanInstr()
        {
            unknown.close();
        }

        void getAnalysisUsage(llvm::AnalysisUsage & AU) const override;
        bool runOnFunction(llvm::Function & f, int override_counter = -1);
        void modifyFunction(llvm::Function & f, Function & func, Instrumenter &);
        bool analyzeFunction(llvm::Function & f, int override_counter = -1);
        bool runOnModule(llvm::Module & f) override;
        bool is_analyzable(llvm::Module & m, llvm::Function & f);
        bool handleOpenMP(llvm::Function &f, int override_counter = -1);
        void foundFunction(llvm::Function &f, bool important, int counter = -1);
        void insertCallsite(llvm::Function & f, llvm::Value * val);
    };

}

#endif
