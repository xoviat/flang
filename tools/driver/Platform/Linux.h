//===- Driver.h -------------------------------------------------*- C++ -*-===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef FLANG_PLATFORM_LINUX_H
#define FLANG_PLATFORM_LINUX_H

#include "Platform.h"

namespace flang
{
namespace platform
{

class Linux : public Platform
{
public:
  using Platform::Platform;

  std::vector<std::string> GetIncludePath();
  std::vector<std::string> GetCompileDefinitions();

public:
  std::string ObjectExtension() { return ".o"; };
  std::string LibraryExtension() { return ".a"; };
  std::string ExecutableExtension() { return ""; };
};

} // namespace platform
} // namespace flang

#endif