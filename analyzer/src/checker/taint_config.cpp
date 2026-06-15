#include <ikos/analyzer/checker/taint_config.hpp>

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>
#include <ikos/core/exception.hpp>
#include <ikos/analyzer/exception.hpp>

namespace ikos {
namespace analyzer {

TaintConfig::TaintConfig() {}

void TaintConfig::load_from_file(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    throw LogicError("Failed to open taint configuration file: " + filepath);
  }

  nlohmann::json j;
  file >> j;

  if (j.contains("sources")) {
    for (const auto& source : j["sources"]) {
      _sources.insert(source.get<std::string>());
    }
  }

  if (j.contains("sinks")) {
    for (const auto& sink : j["sinks"]) {
      Sink s;
      s.name = sink["name"].get<std::string>();
      
      std::string type_str = sink["type"].get<std::string>();
      if (type_str == "CommandInjection") {
        s.type = SinkType::CommandInjection;
      } else if (type_str == "PathTraversal") {
        s.type = SinkType::PathTraversal;
      } else if (type_str == "FormatString") {
        s.type = SinkType::FormatString;
      } else if (type_str == "SQLInjection") {
        s.type = SinkType::SQLInjection;
      } else {
        throw LogicError("Unknown sink type: " + type_str);
      }

      if (sink.contains("sensitive_args")) {
        for (const auto& arg : sink["sensitive_args"]) {
          s.sensitive_args.push_back(arg.get<unsigned>());
        }
      }

      _sinks.push_back(s);
    }
  }

  if (j.contains("sanitizers")) {
    for (const auto& sanitizer : j["sanitizers"]) {
      _sanitizers.insert(sanitizer.get<std::string>());
    }
  }
}

bool TaintConfig::is_source(const std::string& name) const {
  return _sources.find(name) != _sources.end();
}

const TaintConfig::Sink* TaintConfig::get_sink(const std::string& name) const {
  for (const auto& sink : _sinks) {
    if (sink.name == name) {
      return &sink;
    }
  }
  return nullptr;
}

bool TaintConfig::is_sanitizer(const std::string& name) const {
  return _sanitizers.find(name) != _sanitizers.end();
}

} // end namespace analyzer
} // end namespace ikos
