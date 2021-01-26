//===- Driver.h -------------------------------------------------*- C++ -*-===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef FLANG_DRIVER_H
#define FLANG_DRIVER_H

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/raw_ostream.h"

#include "Platform/Platform.h"

namespace flang
{
namespace driver
{

using namespace llvm;

class FlangDriver
{
public:
  FlangDriver(std::string Executable);
  ~FlangDriver();

public:
  void compile(llvm::opt::InputArgList &Args);

private:
  std::string getTemporaryPath(StringRef Prefix, StringRef Suffix) const;
  void addCommand(std::string Exec, llvm::opt::ArgStringList &Args);
  std::string getProgramPath(std::string program);
  void addTemporaryFile(std::string fpath);
  bool isFreeFormFortran(std::string InputFile);
  std::string getShortPath(std::string path);
  std::string getLibPath(std::string lib);
  std::vector<std::string> getLibPaths(std::vector<std::string> libs);
  std::string getIncludePath();
  std::string sanitize(std::string arg);

  /*
    Returns *.ir, *.c, *.obj, and other files but NOT *.f or *.f90 files.
  */
  std::vector<std::string> upper(llvm::opt::InputArgList &Args, std::vector<std::string> InputFiles);
  void lower(llvm::opt::InputArgList &Args, std::vector<std::string> InputFiles, std::string MainFile);

private:
  bool isVerbose = false;

  llvm::Triple triple;
  std::vector<std::string> temporaryFiles;
  std::string directory;

  platform::Platform *platform;
  //  void readConfigs(llvm::opt::InputArgList &Args);
  //  void createFiles(llvm::opt::InputArgList &Args);
  //  void inferMachineType();
  //
  //
  //  // True if we are in --whole-archive and --no-whole-archive.
  //  bool InWholeArchive = false;
  //
  //  // True if we are in --start-lib and --end-lib.
  //  bool InLib = false;
  //
  //  // True if we are in -format=binary and -format=elf.
  //  bool InBinary = false;
  //
  //  std::vector<InputFile *> Files;
};

} // namespace driver
} // namespace flang

#endif