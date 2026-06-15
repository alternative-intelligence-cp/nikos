#pragma once

#include <string>
#include <vector>
#include <unordered_set>

namespace ikos {
namespace analyzer {

/// \brief Taint Configuration
///
/// Holds the list of sources, sinks, and sanitizers for the taint analysis.
class TaintConfig {
public:
  enum class SinkType {
    CommandInjection,
    PathTraversal,
    FormatString,
    SQLInjection
  };

  struct Sink {
    std::string name;
    SinkType type;
    std::vector<unsigned> sensitive_args; // 0-indexed arguments that are sensitive
  };

private:
  std::unordered_set<std::string> _sources;
  std::vector<Sink> _sinks;
  std::unordered_set<std::string> _sanitizers;

public:
  TaintConfig();

  /// \brief Load configuration from a JSON file
  void load_from_file(const std::string& filepath);

  /// \brief Check if a function is a taint source
  bool is_source(const std::string& name) const;

  /// \brief Get sink definition if the function is a sink
  const Sink* get_sink(const std::string& name) const;

  /// \brief Check if a function is a sanitizer
  bool is_sanitizer(const std::string& name) const;

};

} // end namespace analyzer
} // end namespace ikos
