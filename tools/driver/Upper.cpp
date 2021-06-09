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
#include <fstream>

#include "Driver.h"
#include "Options.h"
#include "ErrorHandler.h"

#include "llvm/Option/Option.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Config/llvm-config.h"

using namespace llvm;
using namespace llvm::opt;

using namespace flang::driver;
using namespace flang;

std::vector<std::string> FlangDriver::upper(llvm::opt::InputArgList &Args, std::vector<std::string> InputFiles)
{
  ArgStringList CommonCmdArgs;
  ArgStringList UpperCmdArgs;
  ArgStringList LowerCmdArgs;
  SmallString<256> Stem;
  bool NeedIEEE = true;
  bool NeedFastMath = false;
  bool NeedRelaxedMath = false;

  const char *LLVMVersion = Args.MakeArgString(std::to_string(LLVM_VERSION_MAJOR * 10));

  const char *InputFile = "";
  // Add input file name to the compilation line
  for (auto Input : InputFiles)
  {
    InputFile = Args.MakeArgString(Input);
    UpperCmdArgs.push_back(InputFile);
    if (isVerbose)
      std::cout << "Input file: " << InputFile << std::endl;
  }

  /***** Process file arguments to both parts *****/
  // const InputInfo &Input = Inputs[0];
  // types::ID InputType = Input.getType();
  // Check file type sanity
  // assert(types::isFortran(InputType) && "Can only accept Fortran");

  // if (Args.hasArg(options::OPT_fsyntax_only) ||
  //     Args.hasArg(options::OPT_E)) {
  // For -fsyntax-only produce temp files only
  Stem = this->getTemporaryPath("", "");
  // } else {
  //   Stem = llvm::sys::path::filename(OutFile);
  //   llvm::sys::path::replace_extension(Stem, "");
  // }
  const char *OutFile = Args.MakeArgString(Stem + ".ir");
  this->addTemporaryFile(OutFile);

  // Add temporary output for ILM
  const char *ILMFile = Args.MakeArgString(Stem + ".ilm");
  LowerCmdArgs.push_back(ILMFile);
  this->addTemporaryFile(ILMFile);

  /***** Process common args *****/

  // Add "inform level" flag
  if (Args.hasArg(options::OPT_Minform_EQ))
  {
    // Parse arguments to set its value
    for (Arg *A : Args.filtered(options::OPT_Minform_EQ))
    {
      A->claim();
      CommonCmdArgs.push_back("-inform");
      CommonCmdArgs.push_back(A->getValue(0));
    }
  }
  else
  {
    // Default value
    CommonCmdArgs.push_back("-inform");
    CommonCmdArgs.push_back("warn");
  }

  for (auto Arg : Args.filtered(options::OPT_Msave_on))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-save");
  }

  for (auto Arg : Args.filtered(options::OPT_Msave_off))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-nosave");
  }

  // Treat denormalized numbers as zero: On
  for (auto Arg : Args.filtered(options::OPT_Mdaz_on))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-x");
    CommonCmdArgs.push_back("129");
    CommonCmdArgs.push_back("4");
    CommonCmdArgs.push_back("-y");
    CommonCmdArgs.push_back("129");
    CommonCmdArgs.push_back("0x400");
  }

  // Treat denormalized numbers as zero: Off
  for (auto Arg : Args.filtered(options::OPT_Mdaz_off))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-y");
    CommonCmdArgs.push_back("129");
    CommonCmdArgs.push_back("4");
    CommonCmdArgs.push_back("-x");
    CommonCmdArgs.push_back("129");
    CommonCmdArgs.push_back("0x400");
  }

  // Bounds checking: On
  for (auto Arg : Args.filtered(options::OPT_Mbounds_on))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-x");
    CommonCmdArgs.push_back("70");
    CommonCmdArgs.push_back("2");
  }

  // Bounds checking: Off
  for (auto Arg : Args.filtered(options::OPT_Mbounds_off))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-y");
    CommonCmdArgs.push_back("70");
    CommonCmdArgs.push_back("2");
  }

  // Generate code allowing recursive subprograms
  for (auto Arg : Args.filtered(options::OPT_Mrecursive_on))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-recursive");
  }

  // Disable recursive subprograms
  for (auto Arg : Args.filtered(options::OPT_Mrecursive_off))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-norecursive");
  }

  // Enable generating reentrant code (disable optimizations that inhibit it)
  for (auto Arg : Args.filtered(options::OPT_Mreentrant_on))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-reentrant");
  }

  // Allow optimizations inhibiting reentrancy
  for (auto Arg : Args.filtered(options::OPT_Mreentrant_off))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-noreentrant");
  }

  // Swap byte order for unformatted IO
  for (auto Arg : Args.filtered(options::OPT_Mbyteswapio, options::OPT_byteswapio))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-x");
    CommonCmdArgs.push_back("125");
    CommonCmdArgs.push_back("2");
  }

  // Treat backslashes as regular characters
  for (auto Arg : Args.filtered(options::OPT_fnobackslash, options::OPT_Mbackslash))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-x");
    CommonCmdArgs.push_back("124");
    CommonCmdArgs.push_back("0x40");
  }

  // Treat backslashes as C-style escape characters
  for (auto Arg : Args.filtered(options::OPT_fbackslash, options::OPT_Mnobackslash))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-y");
    CommonCmdArgs.push_back("124");
    CommonCmdArgs.push_back("0x40");
  }

  bool mp = false;
  // handle OpemMP options
  if (auto *A = Args.getLastArg(options::OPT_mp, options::OPT_nomp,
                                options::OPT_fopenmp, options::OPT_fno_openmp))
  {
    for (auto Arg : Args.filtered(options::OPT_mp, options::OPT_nomp))
    {
      Arg->claim();
    }
    for (auto Arg : Args.filtered(options::OPT_fopenmp,
                                  options::OPT_fno_openmp))
    {
      Arg->claim();
    }

    if (A->getOption().matches(options::OPT_mp) ||
        A->getOption().matches(options::OPT_fopenmp))
    {
      mp = true;

      CommonCmdArgs.push_back("-mp");

      // Allocate threadprivate data local to the thread
      CommonCmdArgs.push_back("-x");
      CommonCmdArgs.push_back("69");
      CommonCmdArgs.push_back("0x200");

      // Use the 'fair' schedule as the default static schedule
      // for parallel do loops
      CommonCmdArgs.push_back("-x");
      CommonCmdArgs.push_back("69");
      CommonCmdArgs.push_back("0x400");

      // Disable use of native atomic instructions
      // for OpenMP atomics pending either a named
      // option or a libatomic bundled with flang.
      UpperCmdArgs.push_back("-x");
      UpperCmdArgs.push_back("69");
      UpperCmdArgs.push_back("0x1000");
    }
  }

  // Align large objects on cache lines
  for (auto Arg : Args.filtered(options::OPT_Mcache_align_on))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-x");
    CommonCmdArgs.push_back("119");
    CommonCmdArgs.push_back("0x10000000");
    LowerCmdArgs.push_back("-x");
    LowerCmdArgs.push_back("129");
    LowerCmdArgs.push_back("0x40000000");
  }

  // Disable special alignment of large objects
  for (auto Arg : Args.filtered(options::OPT_Mcache_align_off))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-y");
    CommonCmdArgs.push_back("119");
    CommonCmdArgs.push_back("0x10000000");
    LowerCmdArgs.push_back("-y");
    LowerCmdArgs.push_back("129");
    LowerCmdArgs.push_back("0x40000000");
  }

  // -Mstack_arrays
  for (auto Arg : Args.filtered(options::OPT_Mstackarrays))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-x");
    CommonCmdArgs.push_back("54");
    CommonCmdArgs.push_back("8");
  }

  // -g should produce DWARFv2
  for (auto Arg : Args.filtered(options::OPT_g_Flag))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-x");
    CommonCmdArgs.push_back("120");
    CommonCmdArgs.push_back("0x200");
  }

  // -gdwarf-2
  for (auto Arg : Args.filtered(options::OPT_gdwarf_2))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-x");
    CommonCmdArgs.push_back("120");
    CommonCmdArgs.push_back("0x200");
  }

  // -gdwarf-3
  for (auto Arg : Args.filtered(options::OPT_gdwarf_3))
  {
    Arg->claim();
    CommonCmdArgs.push_back("-x");
    CommonCmdArgs.push_back("120");
    CommonCmdArgs.push_back("0x4000");
  }

  // -Mipa has no effect
  if (Arg *A = Args.getLastArg(options::OPT_Mipa))
  {
    //    getToolChain().getDriver().Diag(diag::warn_drv_clang_unsupported)
    //      << A->getAsString(Args);
  }

  // -Minline has no effect
  if (Arg *A = Args.getLastArg(options::OPT_Minline_on))
  {
    // getToolChain().getDriver().Diag(diag::warn_drv_clang_unsupported)
    //   << A->getAsString(Args);
  }

  // Handle -fdefault-real-8 (and its alias, -r8) and -fno-default-real-8
  if (Arg *A = Args.getLastArg(options::OPT_r8,
                               options::OPT_default_real_8_f,
                               options::OPT_default_real_8_fno))
  {
    const char *fl;
    // For -f version add -x flag, for -fno add -y
    if (A->getOption().matches(options::OPT_default_real_8_fno))
    {
      fl = "-y";
    }
    else
    {
      fl = "-x";
    }

    for (Arg *A : Args.filtered(options::OPT_r8,
                                options::OPT_default_real_8_f,
                                options::OPT_default_real_8_fno))
    {
      A->claim();
    }

    UpperCmdArgs.push_back(fl);
    UpperCmdArgs.push_back("124");
    UpperCmdArgs.push_back("0x8");
    UpperCmdArgs.push_back(fl);
    UpperCmdArgs.push_back("124");
    UpperCmdArgs.push_back("0x80000");
  }

  // Process and claim -i8/-fdefault-integer-8/-fno-default-integer-8 argument
  if (Arg *A = Args.getLastArg(options::OPT_i8,
                               options::OPT_default_integer_8_f,
                               options::OPT_default_integer_8_fno))
  {
    const char *fl;

    if (A->getOption().matches(options::OPT_default_integer_8_fno))
    {
      fl = "-y";
    }
    else
    {
      fl = "-x";
    }

    for (Arg *A : Args.filtered(options::OPT_i8,
                                options::OPT_default_integer_8_f,
                                options::OPT_default_integer_8_fno))
    {
      A->claim();
    }

    UpperCmdArgs.push_back(fl);
    UpperCmdArgs.push_back("124");
    UpperCmdArgs.push_back("0x10");
  }

  // Pass an arbitrary flag for first part of Fortran frontend
  for (Arg *A : Args.filtered(options::OPT_Wh_EQ))
  {
    A->claim();
    StringRef Value = A->getValue();
    SmallVector<StringRef, 8> PassArgs;
    Value.split(PassArgs, StringRef(","));
    for (StringRef PassArg : PassArgs)
    {
      UpperCmdArgs.push_back(Args.MakeArgString(PassArg));
    }
  }

  // Flush to zero mode
  // Disabled by default, but can be enabled by a switch
  if (Args.hasArg(options::OPT_Mflushz_on))
  {
    // For -Mflushz set -x 129 2 for second part of Fortran frontend
    for (Arg *A : Args.filtered(options::OPT_Mflushz_on))
    {
      A->claim();
      LowerCmdArgs.push_back("-x");
      LowerCmdArgs.push_back("129");
      LowerCmdArgs.push_back("2");
    }
  }
  else
  {
    LowerCmdArgs.push_back("-y");
    LowerCmdArgs.push_back("129");
    LowerCmdArgs.push_back("2");
    for (Arg *A : Args.filtered(options::OPT_Mflushz_off))
    {
      A->claim();
    }
  }

  // Enable FMA
  for (Arg *A : Args.filtered(options::OPT_Mfma_on, options::OPT_fma))
  {
    A->claim();
    LowerCmdArgs.push_back("-x");
    LowerCmdArgs.push_back("172");
    LowerCmdArgs.push_back("0x40000000");
    LowerCmdArgs.push_back("-x");
    LowerCmdArgs.push_back("179");
    LowerCmdArgs.push_back("1");
  }

  // Disable FMA
  for (Arg *A : Args.filtered(options::OPT_Mfma_off, options::OPT_nofma))
  {
    A->claim();
    LowerCmdArgs.push_back("-x");
    LowerCmdArgs.push_back("171");
    LowerCmdArgs.push_back("0x40000000");
    LowerCmdArgs.push_back("-x");
    LowerCmdArgs.push_back("178");
    LowerCmdArgs.push_back("1");
  }

  // For -fPIC set -x 62 8 for second part of Fortran frontend
  for (Arg *A : Args.filtered(options::OPT_fPIC))
  {
    A->claim();
    LowerCmdArgs.push_back("-x");
    LowerCmdArgs.push_back("62");
    LowerCmdArgs.push_back("8");
  }

  StringRef OptOStr("0");
  if (Arg *A = Args.getLastArg(options::OPT_O_Group))
  {
    if (A->getOption().matches(options::OPT_O4))
    {
      OptOStr = "4"; // FIXME what should this be?
    }
    else if (A->getOption().matches(options::OPT_Ofast))
    {
      OptOStr = "2"; // FIXME what should this be?
    }
    else if (A->getOption().matches(options::OPT_O0))
    {
      // intentionally do nothing
    }
    else
    {
      assert(A->getOption().matches(options::OPT_O) && "Must have a -O flag");
      StringRef S(A->getValue());
      if ((S == "s") || (S == "z"))
      {
        // -Os = size; -Oz = more size
        OptOStr = "2"; // FIXME -Os|-Oz => -opt ?
      }
      else if ((S == "1") || (S == "2") || (S == "3"))
      {
        OptOStr = S;
      }
      else
      {
        OptOStr = "4";
      }
    }
  }
  unsigned OptLevel = std::stoi(OptOStr.str());

  if (Args.hasArg(options::OPT_g_Group))
  {
    // pass -g to lower
    LowerCmdArgs.push_back("-debug");
  }

  /* Pick the last among conflicting flags, if a positive and negative flag
     exists for ex. "-ffast-math -fno-fast-math" they get nullified. Also any
     previously overwritten flag remains that way. 
     For ex. "-Kieee -ffast-math -fno-fast-math". -Kieee gets overwritten by 
     -ffast-math which then gets negated by -fno-fast-math, finally behaving as
     if none of those flags were passed.
  */
  for (Arg *A : Args.filtered(options::OPT_ffast_math, options::OPT_fno_fast_math,
                              options::OPT_Ofast, options::OPT_Kieee_off,
                              options::OPT_Kieee_on, options::OPT_frelaxed_math))
  {
    if (A->getOption().matches(options::OPT_ffast_math) ||
        A->getOption().matches(options::OPT_Ofast))
    {
      NeedIEEE = NeedRelaxedMath = false;
      NeedFastMath = true;
    }
    else if (A->getOption().matches(options::OPT_Kieee_on))
    {
      NeedFastMath = NeedRelaxedMath = false;
      NeedIEEE = true;
    }
    else if (A->getOption().matches(options::OPT_frelaxed_math))
    {
      NeedFastMath = NeedIEEE = false;
      NeedRelaxedMath = true;
    }
    else if (A->getOption().matches(options::OPT_fno_fast_math))
    {
      NeedFastMath = false;
    }
    else if (A->getOption().matches(options::OPT_Kieee_off))
    {
      NeedIEEE = false;
    }
    A->claim();
  }

  if (NeedFastMath)
  {
    // Lower: -x 216 1
    LowerCmdArgs.push_back("-x");
    LowerCmdArgs.push_back("216");
    LowerCmdArgs.push_back("1");
    // Lower: -ieee 0
    LowerCmdArgs.push_back("-ieee");
    LowerCmdArgs.push_back("0");
  }
  else if (NeedIEEE)
  {
    // Common: -y 129 2
    CommonCmdArgs.push_back("-y");
    CommonCmdArgs.push_back("129");
    CommonCmdArgs.push_back("2");

    LowerCmdArgs.insert(
        LowerCmdArgs.end(),
        {
            // Lower: -x 6 0x100
            "-x",
            "6",
            "0x100",
            // Lower: -x 42 0x400000
            "-x",
            "42",
            "0x400000",
            // Lower: -y 129 4
            "-y",
            "129",
            "4",
            // Lower: -x 129 0x400
            "-x",
            "129",
            "0x400",
            // Lower: -y 216 1 (OPT_fno_fast_math)
            "-y",
            "216",
            "1",
            // Lower: -ieee 1
            "-ieee",
            "1",
        });
  }
  else if (NeedRelaxedMath)
  {

    LowerCmdArgs.insert(
        LowerCmdArgs.end(),
        {
            // Lower: -x 15 0x400
            "-x",
            "15",
            "0x400",
            // Lower: -y 216 1 (OPT_fno_fast_math)
            "-y",
            "216",
            "1",
            // Lower: -ieee 0
            "-ieee",
            "0",
        });
  }
  else
  {
    // Lower: -ieee 0
    LowerCmdArgs.push_back("-ieee");
    LowerCmdArgs.push_back("0");
  }

  /***** Upper part of the Fortran frontend *****/

  // TODO do we need to invoke this under GDB sometimes?
  const char *UpperExec = Args.MakeArgString(this->getProgramPath("flang1"));

  UpperCmdArgs.insert(
      UpperCmdArgs.end(),
      {
          "-opt",
          Args.MakeArgString(OptOStr),
          "-terse",
          "1",
          "-inform",
          "warn",
          "-nohpf",
          "-nostatic",

      });

  UpperCmdArgs.append(CommonCmdArgs.begin(), CommonCmdArgs.end()); // Append common arguments

  UpperCmdArgs.insert(
      UpperCmdArgs.end(),
      {
          "-x",
          "19",
          "0x400000",
          "-quad",
          "-x",
          "68",
          "0x1",
          "-x",
          "59",
          "4",
          "-x",
          "15",
          "2",
          "-x",
          "49",
          "0x400004",
          "-x",
          "51",
          "0x20",
          "-x",
          "57",
          "0x4c",
          "-x",
          "58",
          "0x10000",
          "-x",
          "124",
          "0x1000",
          "-tp",
          "px",
          "-x",
          "57",
          "0xfb0000",
          "-x",
          "58",
          "0x78031040",
          "-x",
          "47",
          "0x08",
          "-x",
          "48",
          "4608",
          "-x",
          "49",
          "0x100",
      });

  if (OptLevel >= 2)
  {
    UpperCmdArgs.insert(
        UpperCmdArgs.end(),
        {
            "-x",
            "70",
            "0x6c00",
            "-x",
            "119",
            "0x10000000",
            "-x",
            "129",
            "2",
            "-x",
            "47",
            "0x400000",
            "-x",
            "52",
            "2",

        });
  }

  // Add system include arguments.
  // getToolChain().AddFlangSystemIncludeArgs(Args, UpperCmdArgs);

  // bool isWindowsMSVC = getToolChain().getTriple().isWindowsMSVCEnvironment();

  std::vector<std::string> CommonDefinitions = {
      "__NO_MATH_INLINES",
      "__x86_64",
      "__x86_64__",
      "__THROW=",
      "__extension__=",
      "__amd_64__amd64__",
      "__k8",
      "__k8__",
      "__PGLLVM__",
  };

  std::vector<std::string> PlatformDefinitions = this->platform->GetCompileDefinitions();

  for (auto definition : PlatformDefinitions)
  {
    UpperCmdArgs.push_back("-def");
    UpperCmdArgs.push_back(Args.MakeArgString(definition));
  }

  for (auto definition : CommonDefinitions)
  {
    UpperCmdArgs.push_back("-def");
    UpperCmdArgs.push_back(Args.MakeArgString(definition));
  }

  /*
    When the -E option is given, run flang1 in preprocessor only mode
  */
  // Enable preprocessor
  if (Args.hasArg(options::OPT_Mpreprocess) ||
      Args.hasArg(options::OPT_cpp) ||
      Args.hasArg(options::OPT_E))
  {
    UpperCmdArgs.push_back("-preprocess");
    for (auto Arg : Args.filtered(options::OPT_Mpreprocess, options::OPT_cpp, options::OPT_E))
    {
      Arg->claim();
    }

    // When -E option is provided, run only the fortran preprocessor.
    // Only in -E mode, consume -P if it exists
    if (Args.hasArg(options::OPT_E))
    {
      UpperCmdArgs.push_back("-es");
      // Line marker mode is disabled
      if (Args.hasArg(options::OPT_P))
      {
        Args.ClaimAllArgs(options::OPT_P);
      }
      else
      {
        // -pp enables line marker mode in fortran preprocessor
        UpperCmdArgs.push_back("-pp");
      }
    }
  }

  // Enable standards checking
  if (Args.hasArg(options::OPT_Mstandard))
  {
    UpperCmdArgs.push_back("-standard");
    for (auto Arg : Args.filtered(options::OPT_Mstandard))
    {
      Arg->claim();
    }
  }

  // Free or fixed form file
  if (Args.hasArg(options::OPT_fortran_format_Group))
  {
    // Override file name suffix, scan arguments for that
    for (Arg *A : Args.filtered(options::OPT_fortran_format_Group))
    {
      A->claim();
      switch (A->getOption().getID())
      {
      default:
        llvm_unreachable("missed a case");
      case options::OPT_fixed_form_on:
      case options::OPT_free_form_off:
      case options::OPT_Mfixed:
      case options::OPT_Mfree_off:
      case options::OPT_Mfreeform_off:
        UpperCmdArgs.push_back("-nofreeform");
        break;
      case options::OPT_free_form_on:
      case options::OPT_fixed_form_off:
      case options::OPT_Mfree_on:
      case options::OPT_Mfreeform_on:
        UpperCmdArgs.push_back("-freeform");
        break;
      }
    }
  }
  else
  {
    // Deduce format from file name suffix
    if (isFreeFormFortran(InputFile))
    {
      UpperCmdArgs.push_back("-freeform");
    }
    else
    {
      UpperCmdArgs.push_back("-nofreeform");
    }
  }

  // Extend lines to 132 characters
  for (auto Arg : Args.filtered(options::OPT_Mextend))
  {
    Arg->claim();
    UpperCmdArgs.push_back("-extend");
  }

  for (auto Arg : Args.filtered(options::OPT_ffixed_line_length_VALUE))
  {
    StringRef Value = Arg->getValue();
    if (Value == "72")
    {
      Arg->claim();
    }
    else if (Value == "132")
    {
      Arg->claim();
      UpperCmdArgs.push_back("-extend");
    }
    else
    {
      // getToolChain().getDriver().Diag(diag::err_drv_unsupported_fixed_line_length)
      //   << Arg->getAsString(Args);
    }
  }

  // Add system include directories
  UpperCmdArgs.push_back("-idir");
  UpperCmdArgs.push_back(Args.MakeArgString(this->getIncludePath()));

  // Add user-defined include directories
  for (auto Arg : Args.filtered(options::OPT_I))
  {
    Arg->claim();
    UpperCmdArgs.push_back("-idir");
    UpperCmdArgs.push_back(Args.MakeArgString(this->getShortPath(Arg->getValue(0))));
  }

  // Add user-defined module directories
  for (auto Arg : Args.filtered(options::OPT_ModuleDir, options::OPT_J))
  {
    Arg->claim();
    UpperCmdArgs.push_back("-moddir");
    UpperCmdArgs.push_back(Args.MakeArgString(this->getShortPath(Arg->getValue(0))));
  }

  // "Define" preprocessor flags
  for (auto Arg : Args.filtered(options::OPT_D))
  {
    Arg->claim();
    UpperCmdArgs.push_back("-def");
    UpperCmdArgs.push_back(Arg->getValue(0));
  }

  // "Define" preprocessor flags
  for (auto Arg : Args.filtered(options::OPT_U))
  {
    Arg->claim();
    UpperCmdArgs.push_back("-undef");
    UpperCmdArgs.push_back(Arg->getValue(0));
  }

  UpperCmdArgs.push_back("-vect");
  UpperCmdArgs.push_back("48");

  // Semantics for assignments to allocatables
  if (Arg *A = Args.getLastArg(options::OPT_Mallocatable_EQ))
  {
    // Argument is passed explicitly
    StringRef Value = A->getValue();
    if (Value == "03")
    {                               // Enable Fortran 2003 semantics
      UpperCmdArgs.push_back("-x"); // Set XBIT
    }
    else if (Value == "95")
    {                               // Enable Fortran 2003 semantics
      UpperCmdArgs.push_back("-y"); // Unset XBIT
    }
    else
    {
      // getToolChain().getDriver().Diag(diag::err_drv_invalid_allocatable_mode)
      //   << A->getAsString(Args);
    }
  }
  else
  {                               // No argument passed
    UpperCmdArgs.push_back("-x"); // Default is 03
  }
  UpperCmdArgs.push_back("54");
  UpperCmdArgs.push_back("1"); // XBIT value

  UpperCmdArgs.push_back("-x");
  UpperCmdArgs.push_back("70");
  UpperCmdArgs.push_back("0x40000000");
  UpperCmdArgs.push_back("-y");
  UpperCmdArgs.push_back("163");
  UpperCmdArgs.push_back("0xc0000000");
  UpperCmdArgs.push_back("-x");
  UpperCmdArgs.push_back("189");
  UpperCmdArgs.push_back("0x10");

  // Enable NULL pointer checking
  if (Args.hasArg(options::OPT_Mchkptr))
  {
    UpperCmdArgs.push_back("-x");
    UpperCmdArgs.push_back("70");
    UpperCmdArgs.push_back("4");
    for (auto Arg : Args.filtered(options::OPT_Mchkptr))
    {
      Arg->claim();
    }
  }

  // Set a -x flag for first part of Fortran frontend
  for (Arg *A : Args.filtered(options::OPT_Hx_EQ))
  {
    A->claim();
    StringRef Value = A->getValue();
    auto XFlag = Value.split(",");
    UpperCmdArgs.push_back("-x");
    UpperCmdArgs.push_back(Args.MakeArgString(XFlag.first));
    UpperCmdArgs.push_back(Args.MakeArgString(XFlag.second));
  }

  // Set a -y flag for first part of Fortran frontend
  for (Arg *A : Args.filtered(options::OPT_Hy_EQ))
  {
    A->claim();
    StringRef Value = A->getValue();
    auto XFlag = Value.split(",");
    UpperCmdArgs.push_back("-y");
    UpperCmdArgs.push_back(Args.MakeArgString(XFlag.first));
    UpperCmdArgs.push_back(Args.MakeArgString(XFlag.second));
  }

  // Set a -q (debug) flag for first part of Fortran frontend
  for (Arg *A : Args.filtered(options::OPT_Hq_EQ))
  {
    A->claim();
    StringRef Value = A->getValue();
    auto XFlag = Value.split(",");
    UpperCmdArgs.push_back("-q");
    UpperCmdArgs.push_back(Args.MakeArgString(XFlag.first));
    UpperCmdArgs.push_back(Args.MakeArgString(XFlag.second));
  }

  // Set a -qq (debug) flag for first part of Fortran frontend
  for (Arg *A : Args.filtered(options::OPT_Hqq_EQ))
  {
    A->claim();
    StringRef Value = A->getValue();
    auto XFlag = Value.split(",");
    UpperCmdArgs.push_back("-qq");
    UpperCmdArgs.push_back(Args.MakeArgString(XFlag.first));
    UpperCmdArgs.push_back(Args.MakeArgString(XFlag.second));
  }

  const char *STBFile = Args.MakeArgString(Stem + ".stb");
  this->addTemporaryFile(STBFile);
  UpperCmdArgs.push_back("-stbfile");
  UpperCmdArgs.push_back(STBFile);

  const char *ModuleExportFile = Args.MakeArgString(Stem + ".cmod");
  this->addTemporaryFile(ModuleExportFile);
  UpperCmdArgs.push_back("-modexport");
  UpperCmdArgs.push_back(ModuleExportFile);

  const char *ModuleIndexFile = Args.MakeArgString(Stem + ".cmdx");
  this->addTemporaryFile(ModuleIndexFile);
  UpperCmdArgs.push_back("-modindex");
  UpperCmdArgs.push_back(ModuleIndexFile);

  if (Args.hasArg(options::OPT_E))
  {
    if (Arg *A = Args.getLastArg(options::OPT_o))
    {
      UpperCmdArgs.push_back("-output");
      UpperCmdArgs.push_back(Args.MakeArgString(A->getValue()));
    }
    /*
      If there is no output passed, then flang1 will print to stdout.
    */
  }
  else
  {
    UpperCmdArgs.push_back("-output");
    UpperCmdArgs.push_back(ILMFile);
  }
  this->addCommand(UpperExec, UpperCmdArgs);

  // For -fsyntax-only or -E that is it
  if (Args.hasArg(options::OPT_fsyntax_only) ||
      Args.hasArg(options::OPT_E))
    return std::vector<std::string>();

  /***** Lower part of Fortran frontend *****/

  const char *LowerExec = Args.MakeArgString(this->getProgramPath("flang2"));

  // TODO FLANG arg handling
  LowerCmdArgs.insert(
      LowerCmdArgs.end(),
      {
          "-fn",
          InputFile,
          "-opt",
          Args.MakeArgString(OptOStr),
          "-terse",
          "1",
          "-inform",
          "warn",
      });

  LowerCmdArgs.append(CommonCmdArgs.begin(), CommonCmdArgs.end()); // Append common arguments

  LowerCmdArgs.insert(
      LowerCmdArgs.end(),
      {
          "-x",
          "51",
          "0x20",
          "-x",
          "119",
          "0xa10000",
          "-x",
          "122",
          "0x40",
          "-x",
          "123",
          "0x1000",
          "-x",
          "127",
          "4",
          "-x",
          "127",
          "17",
          "-x",
          "19",
          "0x400000",
          "-x",
          "28",
          "0x40000",
          "-x",
          "120",
          "0x10000000",
          "-x",
          "70",
          "0x8000",
          "-x",
          "122",
          "1",
          "-x",
          "125",
          "0x20000",
          "-quad",
          "-x",
          "59",
          "4",
          "-tp",
          "px",
          "-x",
          "120",
          "0x1000",
          "-x",
          "124",
          "0x1400",
          "-y",
          "15",
          "2",
          "-x",
          "57",
          "0x3b0000",
          "-x",
          "58",
          "0x48000000",
          "-x",
          "49",
          "0x100",
          "-astype",
          "0",
          "-x",
          "183",
          "4",
          "-x",
          "121",
          "0x800",
          "-x",
          "54",
          "0x10",
          "-x",
          "70",
          "0x40000000",
          "-x",
          "249",
          LLVMVersion,
          "-x",
          "124",
          "1",
          "-y",
          "163",
          "0xc0000000",
          "-x",
          "189",
          "0x10",
          "-y",
          "189",
          "0x4000000",
          // Remove "noinline" attriblute
          "-x",
          "183",
          "0x10",
      });

  // Set a -x flag for second part of Fortran frontend
  for (Arg *A : Args.filtered(options::OPT_Mx_EQ))
  {
    A->claim();
    StringRef Value = A->getValue();
    auto XFlag = Value.split(",");
    LowerCmdArgs.push_back("-x");
    LowerCmdArgs.push_back(Args.MakeArgString(XFlag.first));
    LowerCmdArgs.push_back(Args.MakeArgString(XFlag.second));
  }

  // Set a -y flag for second part of Fortran frontend
  for (Arg *A : Args.filtered(options::OPT_My_EQ))
  {
    A->claim();
    StringRef Value = A->getValue();
    auto XFlag = Value.split(",");
    LowerCmdArgs.push_back("-y");
    LowerCmdArgs.push_back(Args.MakeArgString(XFlag.first));
    LowerCmdArgs.push_back(Args.MakeArgString(XFlag.second));
  }

  // Set a -q (debug) flag for second part of Fortran frontend
  for (Arg *A : Args.filtered(options::OPT_Mq_EQ))
  {
    A->claim();
    StringRef Value = A->getValue();
    auto XFlag = Value.split(",");
    LowerCmdArgs.push_back("-q");
    LowerCmdArgs.push_back(Args.MakeArgString(XFlag.first));
    LowerCmdArgs.push_back(Args.MakeArgString(XFlag.second));
  }

  // Set a -qq (debug) flag for second part of Fortran frontend
  for (Arg *A : Args.filtered(options::OPT_Mqq_EQ))
  {
    A->claim();
    StringRef Value = A->getValue();
    auto XFlag = Value.split(",");
    LowerCmdArgs.push_back("-qq");
    LowerCmdArgs.push_back(Args.MakeArgString(XFlag.first));
    LowerCmdArgs.push_back(Args.MakeArgString(XFlag.second));
  }

  // Pass an arbitrary flag for second part of Fortran frontend
  for (Arg *A : Args.filtered(options::OPT_Wm_EQ))
  {
    A->claim();
    StringRef Value = A->getValue();
    SmallVector<StringRef, 8> PassArgs;
    Value.split(PassArgs, StringRef(","));
    for (StringRef PassArg : PassArgs)
    {
      LowerCmdArgs.push_back(Args.MakeArgString(PassArg));
    }
  }

  LowerCmdArgs.push_back("-stbfile");
  LowerCmdArgs.push_back(STBFile);

  if (Args.hasArg(options::OPT_emit_llvm) && Args.hasArg(options::OPT_S))
  {
    if (Arg *A = Args.getLastArg(options::OPT_o))
    {
      LowerCmdArgs.push_back("-asm");
      LowerCmdArgs.push_back(Args.MakeArgString(A->getValue()));
    }
    else
    {
      fatal("output file is required when -S and -emit-llvm options are passed; pending implementation.");
    }
  }
  else
  {
    LowerCmdArgs.push_back("-asm");
    LowerCmdArgs.push_back(OutFile);
  }

  // const llvm::Triple &Triple = getToolChain().getEffectiveTriple();
  // const std::string &TripleStr = Triple.getTriple();
  // LowerCmdArgs.push_back("-target");
  // LowerCmdArgs.push_back(Args.MakeArgString(TripleStr));

  if (!Args.hasArg(options::OPT_noFlangLibs))
  {
    std::vector<std::string> mainlibs = {"flangmain"};

    std::vector<std::string> rtlibs;
    if (Args.hasArg(options::OPT_staticFlangLibs)) {
      rtlibs.push_back("libflang");
      rtlibs.push_back("libflangrti");
      rtlibs.push_back("libpgmath");
    } else {
      rtlibs.push_back("flang");
      rtlibs.push_back("flangrti");
      rtlibs.push_back("pgmath");
    }

    if (mp) {
      rtlibs.push_back("libomp");
    } else if (Args.hasArg(options::OPT_staticFlangLibs)) {
      rtlibs.push_back("libompstub");
    } else {
      rtlibs.push_back("ompstub");
    }

    for (auto rtlibArg : this->platform->GetFlangLibsLinkerArgs(this->getLibPaths(rtlibs)))
    {
      LowerCmdArgs.push_back("-linker");
      LowerCmdArgs.push_back(Args.MakeArgString(rtlibArg));
    }

    if (!Args.hasArg(options::OPT_Mnomain) && !Args.hasArg(options::OPT_no_fortran_main))
    {
      for (auto rtlibArg : this->platform->GetMainLinkerArgs(this->getLibPaths(mainlibs)))
      {
        LowerCmdArgs.push_back("-linker");
        LowerCmdArgs.push_back(Args.MakeArgString(rtlibArg));
      }
    }
  }

  for (auto Arg : Args.filtered(options::OPT_noFlangLibs))
  {
    Arg->claim();
  }

  this->addCommand(LowerExec, LowerCmdArgs);

  std::vector<std::string> OutFiles;

  if (!(Args.hasArg(options::OPT_emit_llvm) && Args.hasArg(options::OPT_S)))
    OutFiles.push_back(OutFile);

  return OutFiles;
}