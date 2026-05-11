// SPDX-FileCopyrightText: 2025 Contributors to TPDE <https://tpde.org>
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "tpde-llvm/OrcCompiler.hpp"

#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Triple.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "tpde-llvm/LLVMCompiler.hpp"

namespace tpde_llvm {

namespace {

class VectorMemoryBuffer : public llvm::MemoryBuffer {
  std::vector<uint8_t> data;

public:
  VectorMemoryBuffer(std::vector<uint8_t> &&v) : data(std::move(v)) {
    const char *ptr = reinterpret_cast<const char *>(data.data());
    init(ptr, ptr + data.size(), /*RequiresNullTerminator=*/false);
  }

  llvm::MemoryBuffer::BufferKind getBufferKind() const override {
    return llvm::MemoryBuffer::MemoryBuffer_Malloc;
  }
};

/*
 * dump_elf_to_disk — write the compiled ELF object to /tmp.
 *
 * Enabled by the environment variable TPDE_DUMP_ASM=1.
 * Each module gets its own file named after the LLVM module identifier
 * (sanitised) with a monotonically increasing counter to avoid collisions:
 *
 *   /tmp/tpde_jit_<counter>_<module_name>.o
 *
 * Inspect with:
 *   objdump -d /tmp/tpde_jit_*.o
 *   llvm-objdump-19 -d --symbolize-operands /tmp/tpde_jit_*.o
 */
static void dump_elf_to_disk(llvm::Module &mod,
                              const std::vector<uint8_t> &elf) {
  static const char *env = std::getenv("TPDE_DUMP_ASM");
  llvm::errs() << "env " << env << "\n"; 
  if (!env || env[0] != '1')
    return;

  static std::atomic<unsigned> counter{0};
  unsigned idx = counter.fetch_add(1, std::memory_order_relaxed);

  /* Sanitise the module name: replace characters that are awkward in paths. */
  std::string mod_name = mod.getModuleIdentifier();
  for (char &c : mod_name)
    if (c == '/' || c == '\\' || c == ' ' || c == ':')
      c = '_';

  std::string path =
      "/postgres/build/tpde_jit_" + std::to_string(idx) + "_" + mod_name + ".o";

  std::error_code ec;
  llvm::raw_fd_ostream out(path, ec, llvm::sys::fs::OF_None);
  if (ec) {
    llvm::errs() << "TPDE: failed to open " << path << ": " << ec.message()
                 << "\n";
    return;
  }
  out.write(reinterpret_cast<const char *>(elf.data()),
            static_cast<size_t>(elf.size()));
  llvm::errs() << "TPDE: dumped ELF object to " << path << "\n";
}

} // anonymous namespace

llvm::Expected<std::unique_ptr<llvm::MemoryBuffer>>
    OrcCompiler::operator()(llvm::Module &mod) {
  std::vector<uint8_t> buf;
  if (compiler && compiler->compile_to_elf(mod, buf)) {
    dump_elf_to_disk(mod, buf);
    return std::make_unique<VectorMemoryBuffer>(std::move(buf));
  }
  if (tm) {
    return llvm::orc::SimpleCompiler(*tm)(mod);
  }
  return llvm::createStringError("TPDE compilation failed");
}

llvm::Expected<std::unique_ptr<llvm::MemoryBuffer>>
    ConcurrentOrcCompiler::operator()(llvm::Module &mod) {
  std::vector<uint8_t> buf;
  auto compiler = LLVMCompiler::create(jtmb.getTargetTriple());
  if (compiler && compiler->compile_to_elf(mod, buf)) {
    dump_elf_to_disk(mod, buf);
    return std::make_unique<VectorMemoryBuffer>(std::move(buf));
  }
  auto tm = llvm::cantFail(jtmb.createTargetMachine());
  return llvm::orc::SimpleCompiler(*tm)(mod);
}

} // namespace tpde_llvm
