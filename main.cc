#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream> //std::stringstream
#include <stdexcept> // TODO: Remove use of exceptions, instead use messages and fallback.
#include <sys/ioctl.h>
#include <unistd.h>
#include <unordered_map>
#include <variant>
#include <vector>

#include "src/arg_parser.h"
#include "src/compile.h"
#include "takoConfig.h"
#include "util.h"

const std::vector<Arg> args = {
    {'h', "help", "Prints this help message.", ""},
    {'V', "version", "Prints the version number.", ""},
    {'O', "", "Number of optimisation passes.", "level"},
    {'o', "out", "File to write results to.", "file"},
    {'i', "interactive", "Run interpreter.", ""},
    {'s', "step", "Stop after this step.", "last"},
};

void takoInfo() {
  std::cerr << "tako - version " << VERSION_STR << "\n";
  std::cerr << "An experimental compiler for ergonomic software verification\n";
}

int main(int argc, char *argv[]) {
  std::vector<std::string> targets;
  std::unordered_map<std::string, std::string> values;

  const std::string prog = argv[0];

  try {
    parseArgs(args, 1, argc, argv, targets, values);
  } catch (const std::runtime_error &er) {
    std::cerr << "Invalid command line argument: " << er.what() << "\n";
    return 1;
  }

  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  Config config;
  config.height = w.ws_row;
  config.width = w.ws_col;

  if (argc < 2 || values.find("help") != values.end() ||
      values.find("version") != values.end()) {
    takoInfo();

    if (argc >= 2 && values.find("help") == values.end()) {
      return 1;
    }
    std::cerr << makeUsage(prog, args);
    return 1;
  }
  std::string out = "%.o";
  const auto out_it = values.find("out");
  if (out_it != values.end()) {
    out = out_it->second;
  }
  const auto last_step_it = values.find("step");
  PassStep last_step = PassStep::Final;
  if (last_step_it != values.end()) {
    const auto opt =
        PassStep::_from_string_nocase_nothrow(last_step_it->second.c_str());
    if (opt) {
      last_step = *opt;
      std::cerr << "Up to " << last_step << "\n";
    } else {
      std::cerr << "No known pass step named " << last_step_it->second << ".\n";
      return 1;
    }
  }
  Messages msgs;
  for (const auto filename : targets) {
    std::string this_out = out;
    this_out.replace(this_out.find('%'), 1, filename);

    std::ifstream inFile;
    inFile.open(filename);

    // TODO: use the file+stream natively using memmap.
    std::stringstream strStream;
    strStream << inFile.rdbuf();
    const std::string contents = strStream.str();

    std::cerr << "> " << filename << " -> " << this_out << "\n";
    Context ctx(msgs, contents, filename, PassStep::Init, last_step, config);

    runCompiler(ctx);
  }

  if (values.find("interactive") != values.end()) {
    takoInfo();
    std::string content;
    std::string line;
    while (true) {
      std::cerr << "> ";
      if (!getline(std::cin, line) || line == ":q") {
        break;
      }
      std::cout << "\n";
      Context ctx(msgs, content+"\n"+line, "stdin", PassStep::Init, last_step, config);
      // TODO: Run for a definition?
      runCompilerInteractive(ctx);
      if (msgs.empty()) {
        // For now, we'll assume it works if there's no errors.
        // TODO: Do something 'real' instead.
        content += "\n"+line;
      }
    }
  }

  return 0;
}