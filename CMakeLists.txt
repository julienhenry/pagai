cmake_minimum_required(VERSION 2.8)

project(PAGAI CXX)

# configuration
option(STATIC_LINK "Whether to build executables as static" OFF)
option(CHECK_EXTERNALS "Whether to check that external dependancies are OK before compiling" ON)

# Path to various libraries
set(EXTERNAL_PATH "${CMAKE_BINARY_DIR}/external/" CACHE PATH "Path to auto-installed external dependencies")
set(LLVM_PATH "${EXTERNAL_PATH}/llvm" CACHE PATH "Path to LLVM installation")
set(GMP_PREFIX "/usr" CACHE PATH "Path to the GMP multiprecision library")
set(MPFR_PREFIX "/usr" CACHE PATH "Path to the MPFR library")
set(APRON_PREFIX "/usr" CACHE PATH "Path to the APRON abstract domains library")
set(PPL_PREFIX "/usr" CACHE PATH "Path to the PPL Polyhedra library")
set(CAMLIDL_PREFIX "/usr" CACHE PATH "Path to the CAMLIDL library")
set(YICES_PATH "${EXTERNAL_PATH}/yices" CACHE PATH "Path to the YICES SMT solver library")
set(Z3_PATH "${EXTERNAL_PATH}/z3" CACHE PATH "Path to the Z3 SMT solver library")
set(CUDD_PATH "${EXTERNAL_PATH}/cudd" CACHE PATH "Path to the CUDD BDD library")

macro(configure_llvm PROG)
  # In theory, we could use the package discovery like this:
  # SET(CMAKE_MODULE_PATH "${LLVM_DIR}")
  # find_package(LLVM)
  # (see http://llvm.org/docs/CMake.html#embedding )
  # In practice, I didn't manage to get llvm_map_components_to_libraries
  # to work, hence we do it by calling llvm-config ourselves instead.
  find_program(LLVM_CONFIG_PROG llvm-config-2.9 NAMES llvm-config-2.9 llvm-config HINTS "${LLVM_PATH}/bin")
  
  execute_process(COMMAND "${LLVM_CONFIG_PROG}" --cxxflags ${ARGN}
    OUTPUT_VARIABLE LLVM_CXXFLAGS)
  # CMake doesn't like newline in CXXFLAGS, and there's at least a
  # trailing \n in llvm-config's output.
  STRING(REGEX REPLACE "\n" " " LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
  # llvm-config has a -fno-rtti -fno-exception by default, uncomment this if needed.
  STRING(REGEX REPLACE "-fno-rtti" " " LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
  STRING(REGEX REPLACE "-fno-exceptions" " " LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")

  execute_process(COMMAND "${LLVM_CONFIG_PROG}" --ldflags
    OUTPUT_VARIABLE LLVM_LDFLAGS)
  STRING(REGEX REPLACE "\n" " " LLVM_LDFLAGS "${LLVM_LDFLAGS}")
  
  execute_process(COMMAND "${LLVM_CONFIG_PROG}" --libs ${ARGN}
    OUTPUT_VARIABLE LLVM_LIBS)
  
  execute_process(COMMAND "${LLVM_CONFIG_PROG}" --version
    OUTPUT_VARIABLE LLVM_VERSION)
  STRING(REGEX REPLACE "\n" "" LLVM_VERSION "${LLVM_VERSION}")
  
  if(NOT "${LLVM_VERSION}" STREQUAL "2.9")
    message(WARNING "LLVM version ${LLVM_VERSION} found, while version 2.9 is needed")
    set(ERROR_OCCURED ON)
  endif()
  
  # turn -lfoo -lbar into foo;bar, so that CMake considers it as a list
  STRING(REGEX REPLACE "\n" "" LLVM_LIBS "${LLVM_LIBS}")
  STRING(REGEX REPLACE "^-l" "" LLVM_LIBS "${LLVM_LIBS}")
  STRING(REGEX REPLACE " -l" ";" LLVM_LIBS "${LLVM_LIBS}")

  target_link_libraries(pagai ${LLVM_LIBS})

  # llvm-config --ldflags was hardcoding -lfoo, but the order of
  # libraries is, and we don't have control on the ordering between
  # LDFLAGS and CMake use of target_link_libraries => remove -lfoo and
  # use target_link_libraries() instead.
  STRING(REGEX REPLACE " *-lpthread *" " " LLVM_LDFLAGS "${LLVM_LDFLAGS}")
  STRING(REGEX REPLACE " *-ldl *" " " LLVM_LDFLAGS "${LLVM_LDFLAGS}")
  target_link_libraries("${PROG}" dl)
  target_link_libraries("${PROG}" pthread)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXXFLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS " ${CMAKE_EXE_LINKER_FLAGS} ${LLVM_LDFLAGS}")
endmacro()

macro(add_cudd_library LIB)
  find_library(CUDD_LIB_${LIB} 
    NAMES ${LIB}
    PATHS ${CUDD_PATH} ${CUDD_PATH}/${LIB}
    NO_DEFAULT_PATH)
  if("${CUDD_LIB_${LIB}}" STREQUAL CUDD_LIB_${LIB}-NOTFOUND)
    message(WARNING
      "Required CUDD library '${LIB}' (i.e lib${LIB}) not found  "
      "(I've searched in ${CUDD_PATH} and ${CUDD_PATH}/${LIB}).  "
      "Please, install CUDD if needed and set \$CUDD_PATH accordingly."
      )
    set(ERROR_OCCURED ON PARENT_SCOPE)
    # We didn't find the absolute path, but if the user wants to try
    # anyway, we can try with just the library name.
    target_link_libraries(pagai ${LIB})
  else()
    target_link_libraries(pagai ${CUDD_LIB_${LIB}})
  endif()
  unset(CUDD_LIB_${LIB} CACHE)
endmacro()

# Compiler choice: clang++ by default
find_program(CLANG_PATH "clang++")
if("${CLANG_PATH}" STREQUAL CLANG_PATH-NOTFOUND)
  message(WARNING "clang++ executable not found, falling back to g++")
  find_program(CLANG_PATH "g++")
endif()

if("${CLANG_PATH}" STREQUAL CLANG_PATH-NOTFOUND)
  message(WARNING "g++ executable not found either. Falling back to CMake's default")
else()
  set(CMAKE_CXX_COMPILER "${CLANG_PATH}")
endif()

option(PRINT_DEBUG "Activate debug traces" OFF)
option(PRINT_DEBUG_SMT "Print the SMT formulae during the analysis" OFF)

add_subdirectory(src)

# TODO: configure-time checks for
# - Z3
# - Yices
# - Apron
if(${CHECK_EXTERNALS})
  if(${ERROR_OCCURED})
    message(FATAL_ERROR
      "Some errors occured during configuration (see above). Aborting.  "
      "Many errors about missing dependencies can be solved by typing "
      "simply \"make\" from the external/ directory."
      )
  endif()
endif()
