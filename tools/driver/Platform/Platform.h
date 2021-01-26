//===- Driver.h -------------------------------------------------*- C++ -*-===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef FLANG_PLATFORM_H
#define FLANG_PLATFORM_H

#include <string>
#include <vector>

#include "llvm/ADT/Triple.h"

namespace flang
{
namespace platform
{

class Platform
{
public:
  Platform(){};
  virtual ~Platform(){};

  explicit Platform(llvm::Triple t) { triple = t; };

public:
  virtual std::vector<std::string> GetFlangLibsLinkerArgs(std::vector<std::string> libs) { return {}; };
  virtual std::vector<std::string> GetMainLinkerArgs(std::vector<std::string> libs) { return {}; };
  virtual std::vector<std::string> GetIncludePath() { return {}; };
  virtual std::vector<std::string> GetCompileDefinitions() { return {}; };

public:
  virtual std::string ObjectExtension() { return ""; };
  virtual std::string ExecutableExtension() { return ""; };
  virtual std::string LibraryExtension() { return ""; };

protected:
  llvm::Triple triple;
};

} // namespace platform
} // namespace flang

#endif