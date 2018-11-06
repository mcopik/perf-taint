#ifndef __DEPENDENCY_FINDER_HPP__
#define __DEPENDENCY_FINDER_HPP__

#include <algorithm>
#include <unordered_map>
#include <set>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace llvm {
    class Argument;
    class Instruction;
    class Value;
    class PHINode;
}

namespace extrap {

    struct Dependency
    {
        virtual ~Dependency() {}
        virtual void json(nlohmann::json &) const = 0;
    };

    struct FunctionArg : Dependency
    {
        int pos;

        FunctionArg(int _pos): pos(_pos) {}

        void json(nlohmann::json & j) const override;
    };
 
    struct DependencyFinder
    {
        std::unordered_map<std::string, Dependency*> dependencies;
        std::set<llvm::PHINode*> phi_nodes;

        ~DependencyFinder();

        void find(const llvm::Argument * arg);
        void find(llvm::Value * v);
        void find(llvm::Instruction * instr);
    };

    // Serialization of Dependency
    void to_json(nlohmann::json& j, const Dependency * p);
}


#endif