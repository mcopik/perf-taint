
#ifndef __PERF_TAINT_FUNCTION_DATABASE_HPP__
#define __PERF_TAINT_FUNCTION_DATABASE_HPP__

#include <perf-taint/llvm-pass/common.hpp>
#include <perf-taint/llvm-pass/Function.hpp>

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace llvm {

  class GlobalVariable;
  class Function;
  class Value;

}
  
namespace perf_taint {

  struct Instrumenter;

  struct FunctionDatabase
  {
    Instrumenter * instrumenter;
    llvm::GlobalVariable * glob_indices;

    struct DataBaseEntry
    {
      json_t loops_data;
    };

    struct ImplicitParameter
    {
      std::string name;
      int param_idx;

      // TODO: 50 shades of c++
      ImplicitParameter(const std::string & _name, int _param_idx):
          name(_name), param_idx(_param_idx) {}
    };

    struct ParameterSource
    {
      llvm::SmallVector<std::tuple<int, const ImplicitParameter*>, 5> function_parameters;
      const ImplicitParameter* return_value;

      ParameterSource():
        return_value(nullptr) {}
    };

    std::unordered_map<std::string, DataBaseEntry> functions;
    std::unordered_map<std::string, ParameterSource> parameter_sources;
    llvm::SmallVector<ImplicitParameter, 5> implicit_parameters;

    void read(std::ifstream &);
    bool contains(llvm::Function * f);
    typedef std::vector< std::vector<int> > vec_t;
    int processLoop(llvm::Function * f, llvm::Value *, Function &, vec_t &);
    size_t parameters_count() const;
    const std::string & parameter_name(size_t idx) const;
    const ImplicitParameter * find_parameter(const std::string & name) const;
    void setInstrumenter(Instrumenter * _instr);
    void annotateParameters(llvm::Function * called_function,
        llvm::Value * call) const;
  };

}

#endif

