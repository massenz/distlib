// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.

#pragma once

#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <glog/logging.h>

// TODO: make parsing lazy: parse() should be protected and triggered lazily upon first access.

namespace utils {

/**
 * Command-line options parser.
 *
 * @author M. Massenzio (marco@alertavert.com) 2016-11-24
 */
class ParseArgs {

  std::vector<std::string> args_;

  std::map<std::string, std::string> parsed_options_;
  std::vector<std::string> positional_args_;

  std::string progname_;

protected:

  std::map<std::string, std::string> parsed_options() const {
    return parsed_options_;
  }

  std::vector<std::string> positional_args() const {
    return positional_args_;
  };

public:
  ParseArgs() = delete;
  ParseArgs(const ParseArgs& other) = delete;

  /**
   * Used to build a parser from a list of strings.
   *
   * <p>Identical in behavior to the other C-style constructor, but will <strong>NOT</strong> assume
   * that the first element of the vector is the program name.
   *
   * @param args a list of both positional and named arguments.
   */
  explicit ParseArgs(const std::vector<std::string>& args) : args_(args) {}

  /**
   * Builds a command-line arguments parser.
   *
   * Use directly with your `main()` arguments, to parse the `--option=value` named arguments and
   * all other "positional" arguments.
   *
   * <p>Example:
   * <pre>
   *   int main(int argc, const char* argv[]) {
   *     utils::ParseArgs parser(argv, argc);
   *     parser.parse();
   *
   *     if (parser.has("help")) {
   *       usage();
   *       return EXIT_SUCCESS;
   *     }
   *     // ...
   *   }
   * </pre>
   *
   * <p>`args[0]` is assumed to be the name of the program (see `progname()`).
   *
   * @param args an array of C-string values.
   * @param len the number of elements in `args`.
   */
  ParseArgs(const char* args[], size_t len) : progname_(args[0]) {
    size_t pos = progname_.rfind("/");
    if (pos != std::string::npos) {
      progname_ = progname_.substr(pos + 1);
    }
    for (int i = 1; i < len; ++i) {
      args_.push_back(std::string(args[i]));
    }
  }

  void parse() {
    std::for_each(args_.begin(), args_.end(), [this] (const std::string& s) {

      // TODO: would this be simpler, faster using std::regex?
      VLOG(2) << "Parsing: " << s;
      if (std::strspn(s.c_str(), "-") == 2) {
        size_t pos = s.find('=', 2);

        std::string key, value;
        if (pos != std::string::npos) {
          key = s.substr(2, pos - 2);
          if (key.length() == 0) {
            LOG(ERROR) << "Illegal option value; no name for configuration: " << s;
            return;
          }
          value = s.substr(pos + 1);
        } else {
          // Empty values are allowed for "flag-type" options (such as `--enable-log`).
          // They can also "turn off" the flag, by pre-pending a `no-`: `--no-edit`)
          key = s.substr(2, s.length());
          if (key.find("no-") == 0) {
            key = key.substr(3);
            value = "off";
          } else {
            value = "on";
          }
        }
        parsed_options_[key] = value;
        VLOG(2) << key << " -> " << value;
      } else {
        VLOG(2) << "Positional(" << positional_args_.size() + 1 << "): " << s;
        positional_args_.push_back(s);
      }
    });
  };

  std::string progname() const {
    return progname_;
  }

  /**
   * Gets the value of the `--name=value` option.
   *
   * @param name
   * @return the value of the named option if specified, or the empty string instead.
   */
  std::string get(const std::string& name) const {
    return parsed_options()[name];
  }

  /**
   * Alias for `get(std::string)`.
   *
   * @return the value of the named option if specified, or the empty string instead.
   */
  const char* get(const char* name) const {
    return parsed_options()[name].c_str();
  }

  /**
   * @param pos the index of the positional argument
   * @return the value of the positional argument at position `pos`, if valid.
   * @throws `std::out_of_range` if `pos` > `count()`
   */
  std::string at(int pos) const {
    if (pos < count()) {
      return positional_args().at(pos);
    }
    throw new std::out_of_range("Not enough positional arguments");
  }

  /**
   * @return the number of positional arguments found.
   */
  int count() const {
    return positional_args().size();
  }

  /**
   * Alias for `get(pos)`.
   *
   * @param pos
   * @return the value of the positional argument at position `pos`.
   */
  std::string operator[](int pos) const {
    return at(pos);
  }

  /**
   * Alias for `get(name)`.
   *
   * @param name
   * @return the value of the named option
   */
  std::string operator[](const std::string& name) const {
    return get(name);
  }

  /**
   * @return all the names of the options being parsed (those of the `--name=value` form).
   */
  std::vector<std::string> getNames() {
    std::vector<std::string> names;
    for(auto elem : parsed_options_) {
      names.push_back(elem.first);
    }
    return names;
  }

  /**
   * Converts an on/off flag into the boolean equivalent.
   *
   * @param name the name of the option being passed on the command line.
   * @param ifAbsentValue if the flag has not been specified, the default value returned.
   * @return `false` if the option is "off"; `true` otherwise.
   */
  bool has(const std::string& name, bool ifAbsentValue = false) {
    std::string value = get(name);
    if (value.empty()) return ifAbsentValue;
    if (value == "on") {
      return true;
    } else if (value == "off") {
      return false;
    } else {
      LOG(ERROR) << "Option '" << name << "' does not appear to be a flag (on/off): '" << value << "'";
      return false;
    }
  }

  /**
   * Explicit check on a flag being disabled.
   *
   * @param name the name of the option being checked (e.g., "help", when checking for an
   *        explicit "--no-help" being passed on the command line).
   * @return `true` iff the named flag was passed as a `--no-xxx` option (where `xxx` is the
   *        `name` of the flag).
   */
  bool hasDisabled(const std::string& name) {
    std::string value = get(name);
    if (value == "off") {
      return true;
    }
    return false;
  }
};

} // namespace utils
