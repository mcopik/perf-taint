
#include <iostream>
#include <fstream>

#include <dfsan-instr/Statistics.hpp>


namespace perf_taint {

  void Statistics::init()
  {
    stats["functions"] = json_t::object();
  }

  void Statistics::found_function(const std::string & name)
  {
    functions_count++;
    stats["functions"][name] = json_t::object();
  }
 
  void Statistics::pruned_function(const std::string & name,
      const std::string & reason)
  {
    stats["functions"][name]["pruned"].push_back(reason);
    ++pruned_count;
    pruned_functions[reason]++;
  }
  
  void Statistics::instrumented_function(const std::string & name,
      const std::string & reason)
  {
    ++instrumented_count;
    instrumented_functions[reason]++;
    stats["functions"][name]["instrumented"].push_back(reason);
  }

  void Statistics::print(const std::string & file_name)
  {
    stats["statistics"]["functions"]["total"] = functions_count;
    stats["statistics"]["functions"]["total_pruned"] = pruned_count;
    stats["statistics"]["functions"]["total_instrumented"] = instrumented_count;
    stats["statistics"]["functions"]["pruned"] = pruned_functions;
    stats["statistics"]["functions"]["instrumented"] = instrumented_functions;

    int loop_count = 0, static_analyzed = 0, static_instrumented = 0;
    bool with_scev_stats = false;
    if(stats["functions"].begin().value()["loops"].count("scev_analyzed_constant"))
      with_scev_stats = true;
    for(auto & func : stats["functions"]) {
      loop_count += func["loops"]["count"].get<int>();
      if(with_scev_stats) {
        static_analyzed += func["loops"]["scev_analyzed_constant"].get<int>();
        static_instrumented += func["loops"]["scev_analyzed_nonconstant"].get<int>();
      }
    }
    stats["statistics"]["loops"]["total"] = loop_count;
    stats["statistics"]["loops"]["static_analyzed"] = static_analyzed;
    stats["statistics"]["loops"]["static_instrumented"] = static_instrumented;


    std::ofstream json_out(file_name);
    json_out << stats.dump(2) << std::endl;
  }

}
