enable_language(CXX)

find_program(LLVM_TABLEGEN_EXE "llvm-tblgen" ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)

include(AddLLVM)
include(TableGen)
include(HandleLLVMOptions)

set(flang_TABLEGEN_EXE "${LLVM_TABLEGEN_EXE}")
set(LLVM_TARGET_DEFINITIONS Options.td)

tablegen(flang Options.inc -gen-opt-parser-defs)

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/OptionsRemoved.stamp"
        COMMAND "${CMAKE_COMMAND}" -E remove "${CMAKE_CURRENT_BINARY_DIR}/Options.inc.d"
        COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Options.inc.d" "${CMAKE_CURRENT_BINARY_DIR}/Options.inc.d"
        COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_CURRENT_BINARY_DIR}/OptionsRemoved.stamp"
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Options.inc"
)

add_custom_target(
    flang_driver_options_tablegen
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/OptionsRemoved.stamp"
)

add_executable(flang
   flang.cpp
   Options.cpp
   Driver.cpp
   Upper.cpp
   Lower.cpp
   ErrorHandler.cpp
   Platform/Linux.cpp
   Platform/MSVC.cpp
)

set_source_files_properties(Driver.cpp PROPERTIES COMPILE_FLAGS /EHsc)

add_dependencies(
    flang 
    flang_driver_options_tablegen
)

target_include_directories(flang PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(flang
    LLVMCore
    LLVMSupport
    LLVMOption
)

# Install the driver
install(TARGETS flang
        RUNTIME DESTINATION ${LLVM_TOOLS_BINARY_DIR})
