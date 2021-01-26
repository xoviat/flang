//===- tools/lld/lld.cpp - Linker Driver Dispatcher -----------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Driver.h"
#include "Options.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/FileSystem.h"
#include <cstdlib>

using namespace llvm;
using namespace llvm::sys;

using namespace flang::driver;
using namespace flang::options;

// If this function returns true, lld calls _exit() so that it quickly
// exits without invoking destructors of globally allocated objects.
//
// We don't want to do that if we are running tests though, because
// doing that breaks leak sanitizer. So, lit sets this environment variable,
// and we use it to detect whether we are running tests or not.
static bool canExitEarly() { return StringRef(getenv("LLD_IN_TEST")) != "1"; }

bool compile(const char *Argv0, ArrayRef<const char *> Args, bool CanExitEarly)
{

  // errorHandler().LogName = args::getFilenameWithoutExe(Args[0]);
  // errorHandler().ErrorOS = &Error;
  // errorHandler().ColorDiagnostics = Error.has_colors();
  // errorHandler().ErrorLimitExceededMsg =
  //     "too many errors emitted, stopping now (use "
  //     "-error-limit=0 to see all errors)";

  // initLLVM();
  FlangOptTable Parser;
  opt::InputArgList ArgList = Parser.parse(Args.slice(1));

  // This just needs to be some symbol in the binary; C++ doesn't
  // allow taking the address of ::main however.
  void *P = (void *)(intptr_t)canExitEarly;

  FlangDriver(llvm::sys::fs::getMainExecutable(Argv0, P)).compile(ArgList);

  // Exit immediately if we don't need to return to the caller.
  // This saves time because the overhead of calling destructors
  // for all globally-allocated objects is not negligible.
  // if (CanExitEarly)
  //   exitLld(errorCount() ? 1 : 0);

  // freeArena();
  // return !errorCount();
  return true;
}

/// Universal linker main(). This linker emulates the gnu, darwin, or
/// windows linker based on the argv[0] or -flavor option.
int main(int Argc, const char **Argv)
{
  // InitLLVM X(Argc, Argv);

  std::vector<const char *> Args(Argv, Argv + Argc);
  compile(Argv[0], ArrayRef<const char *>(Args), canExitEarly());
}
