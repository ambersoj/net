// Belief.hpp
#pragma once

#include <string>
#include <nlohmann/json.hpp>

struct Belief
{
    std::string component;   // who committed it (e.g. "NET")
    std::string subject;     // e.g. "net.tx.done"
    bool polarity;           // true / false
    nlohmann::json context;  // optional, opaque
};
