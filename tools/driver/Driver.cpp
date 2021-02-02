//===- Driver.h -------------------------------------------------*- C++ -*-===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <string>
#include <iostream>
#include <sstream>
#include <regex>
#include <algorithm>

#include "Driver.h"
#include "Options.h"
#include "ErrorHandler.h"

#include "llvm/Support/Program.h"
#include "llvm/Option/Option.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/APInt.h"

#include "Platform/MSVC.h"
#include "Platform/Linux.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace llvm;
using namespace llvm::opt;
using namespace llvm::sys;

using namespace flang::driver;
using namespace flang;

std::string FlangDriver::getTemporaryPath(StringRef Prefix, StringRef Suffix) const
{
  SmallString<128> Path;
  std::error_code EC = llvm::sys::fs::createTemporaryFile(Prefix, Suffix, Path);
  if (EC)
  {
    return "";
  }

  return std::string(Path);
}

std::string FlangDriver::getShortPath(std::string path)
{
#ifdef _WIN32
  /*
      Note: this function is not completely correctly implemented, but
      should work for most cases.
  */
  std::wstring widepath = std::wstring(path.begin(), path.end());
  // consider an input fileName of type PCWSTR (const wchar_t*)
  DWORD requiredBufferLength = GetShortPathNameW(widepath.c_str(), nullptr, 0);

  if (0 == requiredBufferLength) // means failure
    return path;

  wchar_t *buffer = new wchar_t[requiredBufferLength];

  DWORD result = GetShortPathNameW(widepath.c_str(), buffer, requiredBufferLength);

  if (0 == result)
    return path;

  // buffer now contains the full path name of fileName, use it.
  std::wstring wideshortpath(buffer);
  std::string shortpath(wideshortpath.begin(), wideshortpath.end());

  return shortpath;
#else
  return path;
#endif
}

std::string FlangDriver::getLibPath(std::string lib)
{
  return this->getShortPath(
      std::string(llvm::sys::path::parent_path(directory)) + "/lib/" + lib + platform->LibraryExtension());
}

std::vector<std::string> FlangDriver::getLibPaths(std::vector<std::string> libs)
{
  std::vector<std::string> libPaths;
  for (auto lib : libs)
  {
    libPaths.push_back(this->getLibPath(lib));
  }

  return libPaths;
}

std::string FlangDriver::getIncludePath()
{
  return getShortPath(
      std::string(llvm::sys::path::parent_path(directory)) + "/include/flang");
}

std::string FlangDriver::sanitize(std::string arg)
{
  std::replace(arg.begin(), arg.end(), '\\', '/');

  return std::regex_replace(arg, std::regex("\""), "\\\"");
  ;
}

void FlangDriver::addCommand(std::string Exec, llvm::opt::ArgStringList &Args)
{

  std::stringstream cmd;
  cmd << "\"\"" << Exec << "\" ";
  for (auto Arg : Args)
    cmd << "\"" << this->sanitize(std::string(Arg)) << "\" ";

  cmd << "\"";

  if (isVerbose)
    std::cout << std::endl
              << cmd.str() << std::endl;

  std::vector<StringRef> ArgList;
  ArgList.push_back(StringRef(Exec));
  for (auto Arg : Args)
    ArgList.push_back(StringRef(Arg));

  if (sys::ExecuteAndWait(StringRef(Exec), ArrayRef<StringRef>(ArgList)) != 0)
  {
    fatal("compilation stopped: subcommand failed.");
  }
}

std::string FlangDriver::getProgramPath(std::string Name)
{
  // If all else failed, search the path.
  if (llvm::ErrorOr<std::string> P =
          llvm::sys::findProgramByName(Name))
    return *P;

  return "";
}

bool FlangDriver::isFreeFormFortran(std::string InputFile)
{
  return llvm::StringSwitch<bool>(llvm::sys::path::extension(InputFile))
      .Case(".f", false)
      .Case(".F", false)
      .Case(".for", false)
      .Case(".FOR", false)
      .Case(".fpp", false)
      .Case(".FPP", false)
      .Case(".f90", true)
      .Case(".f95", true)
      .Case(".f03", true)
      .Case(".f08", true)
      .Case(".F90", true)
      .Case(".F95", true)
      .Case(".F03", true)
      .Case(".F08", true)
      .Default(true);
}

void FlangDriver::addTemporaryFile(std::string fpath)
{
  this->temporaryFiles.push_back(fpath);
}

FlangDriver::FlangDriver(std::string Executable)
{
  this->directory = std::string(llvm::sys::path::parent_path(Executable));

  /*
    TODO: Cross compilation
  */
  this->triple = llvm::Triple(sys::getProcessTriple());

  /*
    Set the platform based on the triple
  */
  if (this->triple.isWindowsMSVCEnvironment())
  {
    this->platform = new platform::MSVC(triple);
  }
  else
  {
    this->platform = new platform::Linux(triple);
  }
}

/*
  Cleanup temporary files
*/
FlangDriver::~FlangDriver()
{
  for (auto temporaryFile : this->temporaryFiles)
    llvm::sys::fs::remove(temporaryFile);

  delete this->platform;
}

/*
  Take a source file all the way through to an exectuble, if necessary
*/
void FlangDriver::compile(llvm::opt::InputArgList &Args)
{
  if (Args.hasArg(options::OPT__version))
  {
    ArgStringList VersionArgs = {"--version"};
    this->addCommand(this->getProgramPath("clang"), VersionArgs);

    return;
  }

  if (Args.hasArg(options::OPT_v) || true)
    isVerbose = true;

  if (isVerbose)
  {
    for (auto Arg : Args)
    {
      std::cout << std::string(Arg->getOption().getName()) << " : ";
      for (auto Value : Arg->getValues())
        std::cout << ", " << std::string(Value);

      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  if (isVerbose)
  {
    std::cout << "Driver directory: " << this->directory << std::endl;
    std::cout << "Triple: " << this->triple.str() << std::endl;
    std::cout << "Include path: " << this->getIncludePath() <<  std::endl;
  }

  if (!Args.hasArg(options::OPT_INPUT))
    fatal("no input files");

  std::vector<std::string> InputFiles;
  // Add input file name to the compilation line
  for (auto Arg : Args.filtered(options::OPT_INPUT))
  {
    Arg->claim();
    InputFiles.push_back(Args.MakeArgString(Arg->getValue()));
  }

  std::vector<std::string> ProcessedFiles = this->upper(Args, InputFiles);
  if (!ProcessedFiles.empty())
    this->lower(Args, ProcessedFiles, InputFiles.back());
}