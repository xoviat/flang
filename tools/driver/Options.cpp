//===- DriverUtils.cpp ----------------------------------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Options.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Option/Option.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"

using namespace llvm;
using namespace llvm::sys;
using namespace llvm::opt;

using namespace flang::options;

// Create OptTable

// Create prefix string literals used in Options.td
#define PREFIX(NAME, VALUE) const char *const NAME[] = VALUE;
#include "Options.inc"
#undef PREFIX

// Create table mapping all options defined in Options.td
static const opt::OptTable::Info OptInfo[] = {
#define OPTION(X1, X2, ID, KIND, GROUP, ALIAS, X7, X8, X9, X10, X11, X12) \
  {X1, X2, X10, X11, OPT_##ID, opt::Option::KIND##Class,                  \
   X9, X8, OPT_##GROUP, OPT_##ALIAS, X7, X12},
#include "Options.inc"
#undef OPTION
};

FlangOptTable::FlangOptTable() : OptTable(OptInfo) {}

static cl::TokenizerCallback getQuotingStyle(opt::InputArgList &Args)
{
  if (auto *Arg = Args.getLastArg(OPT_rsp_quoting))
  {
    StringRef S = Arg->getValue();
    // if (S != "windows" && S != "posix")
    //   error("invalid response file quoting: " + S);
    if (S == "windows")
      return cl::TokenizeWindowsCommandLine;
    return cl::TokenizeGNUCommandLine;
  }
  if (Triple(sys::getProcessTriple()).getOS() == Triple::Win32)
    return cl::TokenizeWindowsCommandLine;

  return cl::TokenizeGNUCommandLine;
}

// Parses a given list of options.
opt::InputArgList FlangOptTable::parse(ArrayRef<const char *> Argv)
{
  // Make InputArgList from string vectors.
  unsigned MissingIndex;
  unsigned MissingCount;
  SmallVector<const char *, 256> Vec(Argv.data(), Argv.data() + Argv.size());

  // We need to get the quoting style for response files before parsing all
  // options so we parse here before and ignore all the options but
  // --rsp-quoting.
  opt::InputArgList Args = this->ParseArgs(Vec, MissingIndex, MissingCount);

  // Expand response files (arguments in the form of @<filename>)
  // and then parse the argument again.
  // cl::ExpandResponseFiles(Saver, getQuotingStyle(Args), Vec);
  // concatLTOPluginOptions(Vec);
  // Args = this->ParseArgs(Vec, MissingIndex, MissingCount);

  // handleColorDiagnostics(Args);

  // if (MissingCount)
  //   error(Twine(Args.getArgString(MissingIndex)) + ": missing argument");
  // for (auto *Arg : Args.filtered(OPT_UNKNOWN))
  //   error("unknown argument: " + Arg->getSpelling());
  return Args;
}