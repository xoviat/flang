
#ifndef FLANG_OPTIONS_H
#define FLANG_OPTIONS_H

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/StringSaver.h"

namespace flang
{
namespace options
{

using namespace llvm;

// extern llvm::StringSaver Saver;

// Parses command line options.
class FlangOptTable : public llvm::opt::OptTable
{
public:
  FlangOptTable();
  llvm::opt::InputArgList parse(ArrayRef<const char *> Argv);
};

/// Flags specifically for flang options.  Must not overlap with
/// llvm::opt::DriverFlag.
enum FlangFlags
{
  DriverOption = (1 << 4),
  LinkerInput = (1 << 5),
  NoArgumentUnused = (1 << 6),
  Unsupported = (1 << 7),
  CoreOption = (1 << 8),
  CLOption = (1 << 9),
  CC1Option = (1 << 10),
  CC1AsOption = (1 << 11),
  NoDriverOption = (1 << 12),
  Ignored = (1 << 13)
};

// Create enum with OPT_xxx values for each option in Options.td
enum
{
  OPT_INVALID = 0,
#define OPTION(_1, _2, ID, _4, _5, _6, _7, _8, _9, _10, _11, _12) OPT_##ID,
#include "Options.inc"
#undef OPTION
};

} // namespace options
} // namespace flang

#endif
