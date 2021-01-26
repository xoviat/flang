
#include "Linux.h"

using namespace flang::platform;

std::vector<std::string> Linux::GetCompileDefinitions()
{
  return {
      "unix",
      "__unix",
      "__unix__",
      "linux",
      "__linux",
      "__linux__",
      "__LP64__",
      "__LONG_MAX__=9223372036854775807L",
      "__SIZE_TYPE__=unsigned long int",
      "__PTRDIFF_TYPE__=long int",
  };
}

std::vector<std::string> Linux::GetIncludePath() { return {}; };

/*
    /// Convert path list to Fortran frontend argument
static void AddFlangSysIncludeArg(const ArgList &DriverArgs,
                                  ArgStringList &F901Args,
                                  ToolChain::path_list IncludePathList) {
  std::string ArgValue; // Path argument value
   // Make up argument value consisting of paths separated by colons
  bool first = true; 
  for (auto P : IncludePathList) {
    if (first) {
      first = false;
    } else {
      ArgValue += ":";
    }
    ArgValue += P;
  }
   // Add the argument
  F901Args.push_back("-stdinc");
  F901Args.push_back(DriverArgs.MakeArgString(ArgValue));
}
 
 void Linux::AddFlangSystemIncludeArgs(const ArgList &DriverArgs,
                                      ArgStringList &F901Args) const {
  path_list IncludePathList;
  const Driver &D = getDriver();
  std::string SysRoot = computeSysRoot();
   if (DriverArgs.hasArg(options::OPT_nostdinc))
    return;
   {
    SmallString<128> P(D.InstalledDir);
    llvm::sys::path::append(P, "../include");
    IncludePathList.push_back(P.str());
  }
   if (!DriverArgs.hasArg(options::OPT_nostdlibinc))
    IncludePathList.push_back(SysRoot + "/usr/local/include");
   if (!DriverArgs.hasArg(options::OPT_nobuiltininc)) {
    SmallString<128> P(D.ResourceDir);
    llvm::sys::path::append(P, "include");
    IncludePathList.push_back(P.str());
  }
   if (DriverArgs.hasArg(options::OPT_nostdlibinc)) {
    AddFlangSysIncludeArg(DriverArgs, F901Args, IncludePathList);
    return;
  }
   // Check for configure-time C include directories.
  StringRef CIncludeDirs(C_INCLUDE_DIRS);
  if (CIncludeDirs != "") {
    SmallVector<StringRef, 5> dirs;
    CIncludeDirs.split(dirs, ":");
    for (StringRef dir : dirs) {
      StringRef Prefix =
          llvm::sys::path::is_absolute(dir) ? StringRef(SysRoot) : "";
      IncludePathList.push_back(Prefix.str() + dir.str());
    }
    AddFlangSysIncludeArg(DriverArgs, F901Args, IncludePathList);
    return;
  }
   // Lacking those, try to detect the correct set of system includes for the
  // target triple.
   // Add include directories specific to the selected multilib set and multilib.
  if (GCCInstallation.isValid()) {
    const auto &Callback = Multilibs.includeDirsCallback();
    if (Callback) {
      for (const auto &Path : Callback(GCCInstallation.getMultilib()))
        addExternCSystemIncludeIfExists(
            DriverArgs, F901Args, GCCInstallation.getInstallPath() + Path);
    }
  }
   // Implement generic Debian multiarch support.
  const StringRef X86_64MultiarchIncludeDirs[] = {
      "/usr/include/x86_64-linux-gnu",
       // FIXME: These are older forms of multiarch. It's not clear that they're
      // in use in any released version of Debian, so we should consider
      // removing them.
      "/usr/include/i686-linux-gnu/64", "/usr/include/i486-linux-gnu/64"};
  const StringRef X86MultiarchIncludeDirs[] = {
      "/usr/include/i386-linux-gnu",
       // FIXME: These are older forms of multiarch. It's not clear that they're
      // in use in any released version of Debian, so we should consider
      // removing them.
      "/usr/include/x86_64-linux-gnu/32", "/usr/include/i686-linux-gnu",
      "/usr/include/i486-linux-gnu"};
  const StringRef AArch64MultiarchIncludeDirs[] = {
      "/usr/include/aarch64-linux-gnu"};
  const StringRef ARMMultiarchIncludeDirs[] = {
      "/usr/include/arm-linux-gnueabi"};
  const StringRef ARMHFMultiarchIncludeDirs[] = {
      "/usr/include/arm-linux-gnueabihf"};
  const StringRef MIPSMultiarchIncludeDirs[] = {"/usr/include/mips-linux-gnu"};
  const StringRef MIPSELMultiarchIncludeDirs[] = {
      "/usr/include/mipsel-linux-gnu"};
  const StringRef MIPS64MultiarchIncludeDirs[] = {
      "/usr/include/mips64-linux-gnu", "/usr/include/mips64-linux-gnuabi64"};
  const StringRef MIPS64ELMultiarchIncludeDirs[] = {
      "/usr/include/mips64el-linux-gnu",
      "/usr/include/mips64el-linux-gnuabi64"};
  const StringRef PPCMultiarchIncludeDirs[] = {
      "/usr/include/powerpc-linux-gnu"};
  const StringRef PPC64MultiarchIncludeDirs[] = {
      "/usr/include/powerpc64-linux-gnu"};
  const StringRef PPC64LEMultiarchIncludeDirs[] = {
      "/usr/include/powerpc64le-linux-gnu"};
  const StringRef SparcMultiarchIncludeDirs[] = {
      "/usr/include/sparc-linux-gnu"};
  const StringRef Sparc64MultiarchIncludeDirs[] = {
      "/usr/include/sparc64-linux-gnu"};
  ArrayRef<StringRef> MultiarchIncludeDirs;
  switch (getTriple().getArch()) {
  case llvm::Triple::x86_64:
    MultiarchIncludeDirs = X86_64MultiarchIncludeDirs;
    break;
  case llvm::Triple::x86:
    MultiarchIncludeDirs = X86MultiarchIncludeDirs;
    break;
  case llvm::Triple::aarch64:
  case llvm::Triple::aarch64_be:
    MultiarchIncludeDirs = AArch64MultiarchIncludeDirs;
    break;
  case llvm::Triple::arm:
    if (getTriple().getEnvironment() == llvm::Triple::GNUEABIHF)
      MultiarchIncludeDirs = ARMHFMultiarchIncludeDirs;
    else
      MultiarchIncludeDirs = ARMMultiarchIncludeDirs;
    break;
  case llvm::Triple::mips:
    MultiarchIncludeDirs = MIPSMultiarchIncludeDirs;
    break;
  case llvm::Triple::mipsel:
    MultiarchIncludeDirs = MIPSELMultiarchIncludeDirs;
    break;
  case llvm::Triple::mips64:
    MultiarchIncludeDirs = MIPS64MultiarchIncludeDirs;
    break;
  case llvm::Triple::mips64el:
    MultiarchIncludeDirs = MIPS64ELMultiarchIncludeDirs;
    break;
  case llvm::Triple::ppc:
    MultiarchIncludeDirs = PPCMultiarchIncludeDirs;
    break;
  case llvm::Triple::ppc64:
    MultiarchIncludeDirs = PPC64MultiarchIncludeDirs;
    break;
  case llvm::Triple::ppc64le:
    MultiarchIncludeDirs = PPC64LEMultiarchIncludeDirs;
    break;
  case llvm::Triple::sparc:
    MultiarchIncludeDirs = SparcMultiarchIncludeDirs;
    break;
  case llvm::Triple::sparcv9:
    MultiarchIncludeDirs = Sparc64MultiarchIncludeDirs;
    break;
  default:
    break;
  }
  for (StringRef Dir : MultiarchIncludeDirs) {
    if (llvm::sys::fs::exists(SysRoot + Dir)) {
      IncludePathList.push_back(SysRoot + Dir.str());
      break;
    }
  }
   if (getTriple().getOS() == llvm::Triple::RTEMS) {
    AddFlangSysIncludeArg(DriverArgs, F901Args, IncludePathList);
    return;
  }
   // Add an include of '/include' directly. This isn't provided by default by
  // system GCCs, but is often used with cross-compiling GCCs, and harmless to
  // add even when Clang is acting as-if it were a system compiler.
  IncludePathList.push_back(SysRoot + "/include");
   IncludePathList.push_back(SysRoot + "/usr/include");
   AddFlangSysIncludeArg(DriverArgs, F901Args, IncludePathList);
}
*/