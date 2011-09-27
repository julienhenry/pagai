cmake_minimum_required(VERSION 2.8)

project(PAGAI CXX)

# In theory, we could use the package discovery like this:
# SET(CMAKE_MODULE_PATH "${LLVM_DIR}")
# find_package(LLVM)
# (see http://llvm.org/docs/CMake.html#embedding )
# In practice, I didn't manage to get llvm_map_components_to_libraries
# to work, hence we do it by calling llvm-config ourselves instead.
execute_process(COMMAND llvm-config --cxxflags jit interpreter nativecodegen bitreader selectiondag
                OUTPUT_VARIABLE LLVM_CXXFLAGS)
# CMake doesn't like newline in CXXFLAGS, and there's at least a
# trailing \n in llvm-config's output.
STRING(REGEX REPLACE "\n" " " LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
# llvm-config has a -fno-rtti -fno-exception by default, uncomment this if needed.
# STRING(REGEX REPLACE "-fno-rtti" " " LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
# STRING(REGEX REPLACE "-fno-exceptions" " " LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")

execute_process(COMMAND llvm-config --ldflags
                OUTPUT_VARIABLE LLVM_LDFLAGS)
STRING(REGEX REPLACE "\n" " " LLVM_LDFLAGS "${LLVM_LDFLAGS}")

execute_process(COMMAND llvm-config --libs jit interpreter nativecodegen bitreader selectiondag
                OUTPUT_VARIABLE LLVM_LIBS)

# turn -lfoo -lbar into foo;bar, so that CMake considers it as a list
STRING(REGEX REPLACE "\n" "" LLVM_LIBS "${LLVM_LIBS}")
STRING(REGEX REPLACE "^-l" "" LLVM_LIBS "${LLVM_LIBS}")
STRING(REGEX REPLACE " -l" ";" LLVM_LIBS "${LLVM_LIBS}")

# Path to various libraries
set(GMP_PREFIX "/usr" CACHE PATH "Path to the GMP multiprecision library")
set(MPFR_PREFIX "/usr" CACHE PATH "Path to the MPFR library")
set(APRON_PREFIX "/usr" CACHE PATH "Path to the APRON abstract domains library")
set(PPL_PREFIX "/usr" CACHE PATH "Path to the PPL Polyhedra library")
set(CAMLIDL_PREFIX "/usr" CACHE PATH "Path to the CAMLIDL library")
set(YICES_PATH "/home/jhenry/local/yices" CACHE PATH "Path to the YICES SMT solver library")
set(Z3_PATH "/home/jhenry/local/z3" CACHE PATH "Path to the Z3 SMT solver library")
set(CUDD_PATH "/home/jhenry/local/cudd-2.4.2" CACHE PATH "Path to the CUDD BDD library")

macro(add_cudd_library LIB)
  find_library(CUDD_LIB${LIB} NAMES ${LIB} PATHS ${CUDD_PATH} ${CUDD_PATH}/${LIB} NO_DEFAULT_PATH)
  target_link_libraries(pagai ${CUDD_LIB${LIB}})
endmacro()

# configuration
option(STATIC_LINK "Whether to build executables as static" OFF)
find_program(CLANG_PATH "clang++")
set(CMAKE_CXX_COMPILER "${CLANG_PATH}")

add_subdirectory(src)
