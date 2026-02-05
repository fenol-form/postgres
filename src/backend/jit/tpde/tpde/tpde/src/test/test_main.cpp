// SPDX-FileCopyrightText: 2025 Contributors to TPDE <https://tpde.org>
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#define ARGS_NOEXCEPT
#include <args/args.hxx>

#include "TestIR.hpp"
#include "TestIRAdaptor.hpp"
#include "TestIRCompilerA64.hpp"
#include "TestIRCompilerX64.hpp"
#include "tpde/Analyzer.hpp"
#include "tpde/base.hpp"

enum class Arch {
  x64,
  a64,
};

enum class RunTestUntil {
  /// IR-Parsing
  ir_parsing,
  /// marks the end of the flags that will only run the analyzer
  only_analyzer,
  /// No restriction
  full,
};

void print_ir_detailed(tpde::test::TestIR* ir) {
  using namespace tpde;
  std::stringstream ss;
  ss << "Blocks: \n";
  for (u64 i = 0; i < ir->blocks.size(); ++i) {
    ss << i << ": " << ir->blocks[i].name << "\n";
    ss << "    " << ir->blocks[i].block_info << " -- block_info\n";
    ss << "    " << ir->blocks[i].block_info2 << " -- block_info2\n";
    ss << "    " << ir->blocks[i].inst_begin_idx << " -- inst_begin_idx\n";
    ss << "    " << ir->blocks[i].phi_end_idx << " -- phi_end_idx\n";
    ss << "    " << ir->blocks[i].inst_end_idx << " -- inst_end_idx\n";
    ss << "    " << ir->blocks[i].succ_begin_idx << " -- succ_begin_idx\n";
    ss << "    " << ir->blocks[i].succ_end_idx << " -- succ_end_idx\n";
  }
  ss << "Functions: \n";
  for (u64 i = 0; i < ir->functions.size(); ++i) {
    ss << i << ": " << ir->functions[i].name << "\n";
    ss << "    " << ir->functions[i].arg_begin_idx << " -- arg_begin_idx\n";
    ss << "    " << ir->functions[i].arg_end_idx << " -- arg_end_idx\n";
    ss << "    " << ir->functions[i].block_begin_idx << " -- block_begin_idx\n";
    ss << "    " << ir->functions[i].block_end_idx << " -- block_end_idx\n";
    ss << "    " << ir->functions[i].declaration << " -- declaration\n";
    ss << "    " << ir->functions[i].local_only << " -- local_only\n";
    ss << "    " << ir->functions[i].has_call << " -- has_call\n";
  }
  ss << "Value: \n";
  for (u64 i = 0; i < ir->values.size(); ++i) {
    ss << i << ": " << ir->values[i].name << "\n";
    ss << "    " << (u32)ir->values[i].op << " -- op\n";
    ss << "    " << (u32)ir->values[i].type << " -- type\n";
    ss << "    " << ir->values[i].op_count << " -- op_count\n";
    ss << "    " << ir->values[i].call_func_idx << " -- call_func_idx\n";
    ss << "    " << ir->values[i].op_begin_idx << " -- op_begin_idx\n";
    ss << "    " << ir->values[i].op_end_idx << " -- op_end_idx\n";
  }
  ss << "Value operands: \n";
  for (u64 i = 0; i < ir->value_operands.size(); ++i) {
    ss << "(" << i << ")" << ir->value_operands[i] << " ";
  }
  ss << "\n";
  std::cout << ss.str();
}

int main(int argc, char *argv[]) {
  using namespace tpde;

  args::ArgumentParser parser("Testing utility for TPDE");
  args::HelpFlag help(parser, "help", "Display help", {'h', "help"});
  args::ValueFlag<unsigned> log_level(
      parser,
      "log_level",
      "Set the log level to 0=NONE, 1=ERR, 2=WARN(default), 3=INFO, 4=DEBUG, "
      ">5=TRACE",
      {'l', "log-level"},
      2);

  args::Flag print_ir(
      parser, "print_ir", "Print the IR after parsing", {"print-ir"});

  args::Flag print_rpo(
      parser, "print_rpo", "Print the block RPO", {"print-rpo"});

  args::Flag print_layout(parser,
                          "print_layout",
                          "Print the finished block layout",
                          {"print-layout"});

  args::Flag print_loops(
      parser, "print_loops", "Print the loops", {"print-loops"});

  args::Flag print_liveness(parser,
                            "print_liveness",
                            "Print the liveness information",
                            {"print-liveness"});

  args::Flag no_fixed_assignments(
      parser,
      "no_fixed_assignments",
      "Prevent fixed assignments from occuring unless they are forced",
      {"no-fixed-assignments"});

  std::unordered_map<std::string_view, RunTestUntil> run_map{
      {    "full",          RunTestUntil::full},
      {      "ir",    RunTestUntil::ir_parsing},
      {"analyzer", RunTestUntil::only_analyzer},
  };
  args::MapFlag<std::string_view, RunTestUntil> run_until(
      parser,
      "run_until",
      "Run the test only to a certain step in the pipeline",
      {"run-until"},
      run_map,
      RunTestUntil::full);

  std::unordered_map<std::string_view, Arch> arch_map{
      {    "x64", Arch::x64},
      { "x86_64", Arch::x64},
      {    "a64", Arch::a64},
      {"aarch64", Arch::a64},
  };
  args::MapFlag<std::string_view, Arch> arch(parser,
                                             "arch",
                                             "Which architecture to compile to",
                                             {"arch", "target"},
                                             arch_map,
                                             Arch::x64);

  args::ValueFlag<std::string> obj_out_path(
      parser,
      "obj_path",
      "Path where the output object file should be written",
      {'o', "obj-out"});

  args::Positional<std::string> ir_path(
      parser, "ir_path", "Path to the input IR file");

  parser.ParseCLI(argc, argv);
  if (parser.GetError() == args::Error::Help) {
    std::cout << parser;
    return 0;
  }
  if (parser.GetError() != args::Error::None) {
    std::cerr << "Error parsing arguments: " << parser.GetErrorMsg() << '\n';
    return 1;
  }

// TODO(ts): make this configurable
#ifdef TPDE_LOGGING
  {
    spdlog::level::level_enum level = spdlog::level::off;
    switch (log_level.Get()) {
    case 0: level = spdlog::level::off; break;
    case 1: level = spdlog::level::err; break;
    case 2: level = spdlog::level::warn; break;
    case 3: level = spdlog::level::info; break;
    case 4: level = spdlog::level::debug; break;
    default:
      assert(level >= 5);
      level = spdlog::level::trace;
      break;
    }

    spdlog::set_level(level);
  }
#endif

  std::string buf;

  if (ir_path) {
    const auto file_path = args::get(ir_path);
    auto file = std::ifstream{file_path, std::ios::ate};
    if (!file.is_open()) {
      fprintf(stderr, "Failed to open file '%s'\n", file_path.c_str());
      return 1;
    }

    const auto file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    buf.resize(file_size);

    file.read(buf.data(), file_size);
  } else {
    // Read from stdin
    std::string line{};
    while (std::getline(std::cin, line)) {
      if (std::cin.eof()) {
        break;
      }
      buf += line;
      buf += '\n';
    }
  }

  test::TestIR ir{};
  if (!ir.parse_ir(buf)) {
    fprintf(stderr, "Failed to parse IR\n");
    return 1;
  }

  if (print_ir) {
    // ir.print();
    print_ir_detailed(&ir);
    return 0;
  }

  if (run_until.Get() == RunTestUntil::ir_parsing) {
    return 0;
  }

  if (run_until.Get() == RunTestUntil::only_analyzer) {
    test::TestIRAdaptor adaptor{&ir};

    Analyzer<test::TestIRAdaptor> analyzer{&adaptor};

    for (auto func : adaptor.funcs()) {
      if (adaptor.func_extern(func)) {
        continue;
      }

      adaptor.switch_func(func);
      analyzer.switch_func(func);

      if (print_rpo) {
        std::cout << "RPO for func " << adaptor.func_link_name(func) << "\n";
        analyzer.print_rpo(std::cout);
        std::cout << "End RPO\n";
      }

      if (print_layout) {
        std::cout << "Block Layout for " << adaptor.func_link_name(func)
                  << "\n";
        analyzer.print_block_layout(std::cout);
        std::cout << "End Block Layout\n";
      }

      if (print_loops) {
        std::cout << "Loops for " << adaptor.func_link_name(func) << "\n";
        analyzer.print_loops(std::cout);
        std::cout << "End Loops\n";
      }

      if (print_liveness) {
        std::cout << "Liveness for " << adaptor.func_link_name(func) << "\n";
        analyzer.print_liveness(std::cout);
        std::cout << "End Liveness\n";
      }
    }

    return 0;
  }

  using CompileFn = std::vector<u8> (*)(test::TestIR *, bool);
  CompileFn compile_fn;
  switch (arch.Get()) {
  case Arch::x64: compile_fn = &test::compile_ir_x64; break;
  case Arch::a64: compile_fn = &test::compile_ir_arm64; break;
  default: TPDE_UNREACHABLE("invalid architecture");
  }

  std::vector<u8> data = compile_fn(&ir, no_fixed_assignments.Get());
  if (data.empty()) {
    TPDE_LOG_ERR("Failed to compile IR");
    return 1;
  }

  if (obj_out_path && obj_out_path.Get() != "-") {
    std::ofstream out_file{obj_out_path.Get(), std::ios::binary};
    if (!out_file.is_open()) {
      TPDE_LOG_ERR("Failed to open output file");
      return 1;
    }
    out_file.write(reinterpret_cast<const char *>(data.data()), data.size());
  } else {
    std::cout.write(reinterpret_cast<const char *>(data.data()), data.size());
  }

  return 0;
}
