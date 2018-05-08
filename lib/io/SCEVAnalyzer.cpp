//
// Created by mcopik on 5/3/18.
//

#include "io/SCEVAnalyzer.hpp"
#include "LoopCounters.hpp"
#include "results/LoopInformation.hpp"

results::UpdateType SCEVAnalyzer::classify(const SCEV * val)
{
    switch(val->getSCEVType())
    {
        case scAddRecExpr:
            return classify(dyn_cast<SCEVAddRecExpr>(val));
        case scAddMulExpr:
            return classify(dyn_cast<SCEVAddMulExpr>(val));
        case scMulExpr:
            return classify(dyn_cast<SCEVMulExpr>(val));
        default:
            break;
    }

    std::string output;
    raw_string_ostream string_os(output);
    string_os << *val;
    log << "Unrecognized SCEV type: " << val->getSCEVType() << " " << string_os.str() << '\n';
    return results::UpdateType::UNKNOWN;
}

results::UpdateType SCEVAnalyzer::classify(const SCEVAddRecExpr *val)
{
    // represents a closed-form function of type A + B*x
    // i.e. an update of type x = A; x += B;
    if(val->isAffine()) {
        if(val->getOperand(1)->isOne()) {
            return results::UpdateType::INCREMENT;
        } else {
            return results::UpdateType::ADD;
        }
    } else {
        return results::UpdateType::UNKNOWN;
    }
}

results::UpdateType SCEVAnalyzer::classify(const SCEVAddMulExpr *val)
{
    std::string output;
    raw_string_ostream string_os(output);
    string_os << *val;
    log << "Recognized SCEV type: " << val->getSCEVType() << " " << string_os.str() << '\n';
    if(val->representsAffineUpdate())
        return results::UpdateType::AFFINE;
    else
        return results::UpdateType::MULTIPLY;
}

results::UpdateType SCEVAnalyzer::classify(const SCEVMulExpr *val)
{
    //look for a multiplication with a constant
    if(val->getOperand(0)->getSCEVType() == scConstant)
        return classify(val->getOperand(1));
    else if(val->getOperand(1)->getSCEVType() == scConstant)
        return classify(val->getOperand(0));
    //FIXME: something smarter
    std::string output;
    raw_string_ostream string_os(output);
    string_os << *val;
    log << "Unrecognized multiplication SCEV: " << string_os.str() << '\n';
    return results::UpdateType::UNKNOWN;
}

std::string SCEVAnalyzer::toString(const SCEV * val)
{
    switch(val->getSCEVType())
    {
        case scAddExpr:
            return toString(dyn_cast<SCEVAddExpr>(val));
        case scMulExpr:
            return toString(dyn_cast<SCEVMulExpr>(val));
        case scAddRecExpr:
            return toString(dyn_cast<SCEVAddRecExpr>(val));
        case scAddMulExpr:
            return toString(dyn_cast<SCEVAddMulExpr>(val));
        case scConstant:
            return toString(dyn_cast<SCEVConstant>(val));
        case scTruncate:
            return toString(dyn_cast<SCEVTruncateExpr>(val));
        case scUnknown:
            //SCEVUnknown * val = dyn_cast<SCEVUnknown>(val);
            return "unknown";//toString(val->getValue(), SE);
        default:
            assert(!"Unknown SCEV type!");
    }
}

std::string SCEVAnalyzer::toString(const SCEVConstant * val)
{
    return std::to_string(dyn_cast<SCEVConstant>(val)->getAPInt().getSExtValue());
}

std::string SCEVAnalyzer::toString(const SCEVTruncateExpr * expr)
{
    //FIXME: do we need that information?
    std::string type;
    raw_string_ostream os(type);
    expr->getType()->print(os);
    return "trunc(" + toString(expr->getOperand()) + ", " + os.str() + ")";
}

std::string SCEVAnalyzer::toString(const SCEVAddExpr * expr)
{
    std::string str;
    // here select var name
    //expr->getLoop();
    str = toString(expr->getOperand(0));
    str += " + ";
    str = toString(expr->getOperand(1));
    return str;
}

std::string SCEVAnalyzer::toString(const SCEVMulExpr * expr)
{
    std::string str;
    // here select var name
    //expr->getLoop();
    str = toString(expr->getOperand(0));
    str += " * ";
    str = toString(expr->getOperand(1));
    return str;
}

std::string SCEVAnalyzer::toString(const SCEVAddRecExpr * expr)
{
    std::string str;
    // here select var name
    //expr->getLoop();
    str = toString(expr->getOperand(0));
    for(int i = 1; i < expr->getNumOperands(); ++i) {
        str += " + ";
        str += toString(expr->getOperand(i));
        std::string variable = counters.getCounterName(expr->getLoop());
        str += " *" + variable + (i > 1 ? "^" + std::to_string(i) : "");
    }
    return str;
}

std::string SCEVAnalyzer::toString(const SCEVAddMulExpr * expr)
{
    std::string str;
    // here select var name
    //expr->getLoop();
    str = toString(expr->getOperand(0)) + " + ";
    std::string variable = counters.getCounterName(expr->getLoop());
    str += toString(expr->getOperand(2)) + "*" + variable;
    return str;
}

const SCEV * SCEVAnalyzer::get(Value * val)
{
    return SE.getSCEV(val);
}

ScalarEvolution & SCEVAnalyzer::getSE()
{
    return SE;
}