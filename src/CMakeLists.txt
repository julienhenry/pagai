SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

file(GLOB TOP_SRC *.cc)

if(${STATIC_LINK})
  # probably not the cleanest way to do (-static won't exist for all
  # compilers). Using find_library and putting .a at the front of
  # CMAKE_FIND_LIBRARY_SUFFIXES should also do the trick if ever
  # needed.
  set(CMAKE_EXE_LINKER_FLAGS " ${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

if(${PRINT_DEBUG})
  add_definitions(-DPRINT_DEBUG)
endif()

if(${PRINT_DEBUG_SMT})
  add_definitions(-DPRINT_DEBUG_SMT)
endif()

# TODO: we could do configure-time checks with find_file(FOO NAMES foo.h PATH ...)
include_directories(
  ${YICES_PATH}/include 
  ${Z3_PATH}/include
  ${GMP_PREFIX}/include
  ${MPFR_PREFIX}/include
  ${APRON_PREFIX}/include 
  ${CUDD_PATH}/include
)

# TODO: If we don't add /usr/lib at the front, then the pagai executable
# tries to link against libgmp.so.3 which doesn't exist. It seems to
# be linked to the version of libutils chosen by the linker.
set(CMAKE_EXE_LINKER_FLAGS "-L/usr/lib ${CMAKE_EXE_LINKER_FLAGS}")
link_directories(
  ${GMP_PREFIX}/lib
  ${MPFR_PREFIX}/lib 
  ${PPL_PREFIX}/lib
)

add_executable(pagai ${TOP_SRC})

configure_llvm(pagai bitreader selectiondag)

# Apron and dependencies
add_apron_library(pagai polkaMPQ)
add_apron_library(pagai octD)
add_apron_library(pagai boxMPFR)
add_apron_library(pagai ap_ppl)
add_apron_library(pagai ap_pkgrid)
add_apron_library(pagai apron)
add_ppl_library(pagai ppl)
# SMT solvers
add_yices_library(pagai)
add_z3_library(pagai)
target_link_libraries(pagai mpfr gmpxx gmp)

# TODO: these libraries are not always needed
# (it seems they are by libz3.a but not libz3.so ...),
# we should detect and adapt...
target_link_libraries(pagai rt gomp)

# CUDD (BDD library)
# TODO: There is a confusion between /lib/libutil and CUDD's libutil,
# hence we need to add CUDD one by its full name (If we don't do this,
# it doesn't link on Julien's machine). While we're there, make all
# CUDD library paths absolute and unambiguous.
add_cudd_library(pagai obj)
add_cudd_library(pagai cudd)
add_cudd_library(pagai util)
add_cudd_library(pagai mtr)
add_cudd_library(pagai st)
add_cudd_library(pagai epd)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/DOXYFILE.in ${CMAKE_CURRENT_BINARY_DIR}/DOXYFILE @ONLY)
ADD_CUSTOM_TARGET(doc
  COMMAND doxygen ${CMAKE_CURRENT_BINARY_DIR}/DOXYFILE
  )
