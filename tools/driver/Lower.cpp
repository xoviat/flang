//===- Driver.h -------------------------------------------------*- C++ -*-===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <direct.h>

#include <string>
#include <iostream>
#include <sstream>

#include "Driver.h"
#include "Options.h"
#include "ErrorHandler.h"

#include "llvm/Option/Option.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/FileSystem.h"

using namespace llvm;
using namespace llvm::opt;

using namespace flang::driver;
using namespace flang;

#ifdef _MSC_VER
#define MAXPATHLEN 1024
#endif

std::string get_working_path()
{
  char temp[MAXPATHLEN];
  return (_getcwd(temp, MAXPATHLEN) ? std::string(temp) : std::string(""));
}

/*
  Invoke LLVM on the IR file and maybe link the executable
*/
void FlangDriver::lower(llvm::opt::InputArgList &Args, std::vector<std::string> InputFiles, std::string MainFile)
{
  const char *ObjectExtension = Args.MakeArgString(platform->ObjectExtension());

  // Handle -o
  /*
    If -c is not passed, then our OutFile is temporary. Otherwise permenant.
  */
  const char *OutFile;
  if (Args.hasArg(options::OPT_c))
  {
    OutFile = Args.MakeArgString(
        llvm::sys::path::stem(MainFile) + ObjectExtension);
    if (auto *Arg = Args.getLastArg(options::OPT_o))
      OutFile = Args.MakeArgString(Arg->getValue());
  }
  else
  {
    OutFile = Args.MakeArgString(this->getTemporaryPath("", "") + ObjectExtension);
  }

  const char *Optimization;
  if (Args.hasArg(options::OPT_O))
    Optimization = Args.MakeArgString(
        "-O" + std::string(Args.getLastArg(options::OPT_O)->getValue()));
  else
    Optimization = "-O0";

  // Compile to object
  const char *ClangExec = Args.MakeArgString(this->getProgramPath("clang"));

  ArgStringList ClangCmdArgs = {
      "-cc1",
      "-triple",
      Args.MakeArgString(triple.str()),
      "-emit-obj",
      "-mrelax-all",
      "-mincremental-linker-compatible",
      "-disable-free",
      "-disable-llvm-verifier",
      "-discard-value-names",
      "-main-file-name",
      Args.MakeArgString(llvm::sys::path::filename(MainFile)),
      "-mrelocation-model",
      "pic",
      "-pic-level",
      "2",
      "-mthread-model",
      "posix",
      "-fmath-errno",
//      "-masm-verbose",
      "-mconstructor-aliases",
      "-munwind-tables",
      "-target-cpu",
      "x86-64",
//      "-momit-leaf-frame-pointer",
//      "-dwarf-column-info",
      "-debugger-tuning=gdb",
      Optimization,
      "-fdebug-compilation-dir",
      Args.MakeArgString(get_working_path()),
      "-ferror-limit",
      "19",
//      "-fmessage-length",
//      "120",
      "-fms-extensions",
      "-fms-compatibility",
      "-fms-compatibility-version=19.15.26726",
      "-fdelayed-template-parsing",
      "-fobjc-runtime=gcc",
//      "-fdiagnostics-show-option",
      "-o",
      OutFile,
      "-x",
      "ir",
      "-Wno-override-module",
  };

  if (this->isVerbose)
    ClangCmdArgs.push_back("-v");

  const char *InputFile = "";
  for (auto Input : InputFiles)
  {
    InputFile = Args.MakeArgString(Input);
    ClangCmdArgs.push_back(InputFile);
    if (this->isVerbose)
      std::cout << "Lower Input file: " << InputFile << std::endl;
  }

  this->addCommand(ClangExec, ClangCmdArgs);

  if (Args.hasArg(options::OPT_c))
    return;

  const char *ExecutableExtension = Args.MakeArgString(this->platform->ExecutableExtension());
  /*
    Determine the executable name
  */
  const char *Executable;
  if (auto *Arg = Args.getLastArg(options::OPT_o))
    Executable = Args.MakeArgString(Arg->getValue());
  else
    Executable = Args.MakeArgString("a" + std::string(ExecutableExtension));

  // Link the executable
  ArgStringList LinkCmdArgs;

  if (this->isVerbose)
    LinkCmdArgs.push_back("-v");

  // TODO: Move into platform
  if (this->triple.isWindowsMSVCEnvironment())
  {
    LinkCmdArgs.push_back("-Xlinker");
    LinkCmdArgs.push_back("/nodefaultlib:libcmt");
  }

  /*
    TODO: What if object inputs are given on the command line?
  */
  LinkCmdArgs.push_back(OutFile);
  LinkCmdArgs.push_back("-o");
  LinkCmdArgs.push_back(Executable);

  this->addCommand(ClangExec, LinkCmdArgs);
}