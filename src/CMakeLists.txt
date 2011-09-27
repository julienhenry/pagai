SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

file(GLOB TOP_SRC *.cc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXXFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS " ${CMAKE_EXE_LINKER_FLAGS} ${LLVM_LDFLAGS}")

if(${STATIC_LINK})
  # probably not the cleanest way to do (-static won't exist for all
  # compilers). Using find_library and putting .a at the front of
  # CMAKE_FIND_LIBRARY_SUFFIXES should also do the trick if ever
  # needed.
  set(CMAKE_EXE_LINKER_FLAGS " ${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

include_directories(${YICES_PATH}/include ${Z3_PATH}/include ${CUDD_PATH}/include )

include_directories(${GMP_PREFIX}/include
  ${MPFR_PREFIX}/include
  ${APRON_PREFIX}/include 
  ${CUDD_PATH}/include)

# TODO: If we don't add /usr/lib at the front, then the pagai executable
# tries to link against libgmp.so.3 which doesn't exist. It seems to
# be linked to the version of libutils chosen by the linker.
set(CMAKE_EXE_LINKER_FLAGS "-L/usr/lib ${CMAKE_EXE_LINKER_FLAGS}")
link_directories(/usr/lib/
  ${GMP_PREFIX}/lib
  ${MPFR_PREFIX}/lib 
  ${APRON_PREFIX}/lib
  ${PPL_PREFIX}/lib
  ${CAMLIDL_PREFIX}/lib/ocaml/apron
  ${YICES_PATH}/lib 
  ${Z3_PATH}/lib 
  ${CUDD_PATH}/obj 
  ${CUDD_PATH}/mtr 
  ${CUDD_PATH}/st 
  ${CUDD_PATH}/epd 
  ${CUDD_PATH}/cudd)


add_executable(pagai ${TOP_SRC})
target_link_libraries(pagai ${LLVM_LIBS})
target_link_libraries(pagai polkaMPQ octD boxMPFR ap_ppl ap_pkgrid apron)
target_link_libraries(pagai ppl)
target_link_libraries(pagai yices z3)
target_link_libraries(pagai mpfr gmpxx gmp)
target_link_libraries(pagai dl pthread util)
target_link_libraries(pagai obj cudd mtr st util epd)

# TODO: If we don't add this, it doesn't link on Julien's machine
target_link_libraries(pagai ${CUDD_PATH}/util/libutil.a)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/DOXYFILE.in ${CMAKE_CURRENT_BINARY_DIR}/DOXYFILE @ONLY)
ADD_CUSTOM_TARGET(doc
  COMMAND doxygen ${CMAKE_CURRENT_BINARY_DIR}/DOXYFILE
  )
