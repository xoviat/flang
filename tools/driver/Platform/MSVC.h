//===- Driver.h -------------------------------------------------*- C++ -*-===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef FLANG_PLATFORM_WINDOWS_H
#define FLANG_PLATFORM_WINDOWS_H

#include "Platform.h"

namespace flang
{
namespace platform
{

class MSVC : public Platform
{
public:
  using Platform::Platform;

public:
  std::vector<std::string> GetFlangLibsLinkerArgs(std::vector<std::string> libs);
  std::vector<std::string> GetMainLinkerArgs(std::vector<std::string> libs);
  std::vector<std::string> GetCompileDefinitions();

public:
  std::string ObjectExtension() { return ".obj"; };
  std::string LibraryExtension() { return ".lib"; };
  std::string ExecutableExtension() { return ".exe"; };
};

} // namespace platform
} // namespace flang

#endif