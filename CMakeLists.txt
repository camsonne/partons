# BASIC DEFINITIONS ========================================================================

# define minimum version of cmake
cmake_minimum_required(VERSION 3.5)

# define project name and its language
project(PARTONS CXX)

# define c++ standard and issue all the warning demanded by this standard
# (C++17 by default; the optional LibTorch backend, see below, needs C++20 --
# LibTorch >= 2.6 headers do not compile with an earlier standard)
option(PARTONS_WITH_TORCH "Build the LibTorch tensor backend (DVCSCFFTorch)" OFF)
if(PARTONS_WITH_TORCH)
   set(PARTONS_CXX_STANDARD 20)
else()
   set(PARTONS_CXX_STANDARD 17)
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   add_definitions(-std=c++${PARTONS_CXX_STANDARD} -pedantic -Wno-vla-extension)
else()
   add_definitions(-std=c++${PARTONS_CXX_STANDARD} -pedantic -Wno-vla-extension -fext-numeric-literals)
endif()
set(CMAKE_CXX_STANDARD ${PARTONS_CXX_STANDARD})

if (NOT DEFINED CMAKE_MACOSX_RPATH)
   set(CMAKE_MACOSX_RPATH 0)
endif()

# VERSION ==================================================================================

# read file
file(READ ${CMAKE_SOURCE_DIR}/data/version.txt VERSION_STR)

# strip spaces
string(STRIP ${VERSION_STR} VERSION_STR)

# get major, minor and patch versions
string(REGEX MATCH "^[0-9]*\\." TMP_STR ${VERSION_STR})
string(REGEX REPLACE "\\." "" PARTONS_VERSION_MAJOR ${TMP_STR})

string(REGEX MATCH "\\.[0-9]*\\." TMP_STR ${VERSION_STR})
string(REGEX REPLACE "\\." "" PARTONS_VERSION_MINOR ${TMP_STR})

string(REGEX MATCH "\\.[0-9]*$" TMP_STR ${VERSION_STR})
string(REGEX REPLACE "\\." "" PARTONS_VERSION_PATCH ${TMP_STR})

message("-- PARTONS version is: " ${PARTONS_VERSION_MAJOR} "." ${PARTONS_VERSION_MINOR} "." ${PARTONS_VERSION_PATCH})

execute_process(COMMAND bash -c "cd ${CMAKE_SOURCE_DIR}; git branch | grep '\*' | cut -d ' ' -f2 2> /dev/null" OUTPUT_VARIABLE PARTONS_GIT_BRANCH)
execute_process(COMMAND bash -c "cd ${CMAKE_SOURCE_DIR}; git rev-parse HEAD 2> /dev/null" OUTPUT_VARIABLE PARTONS_GIT_REVISION)
string(REGEX REPLACE "\n$" "" PARTONS_GIT_BRANCH "${PARTONS_GIT_BRANCH}")
string(REGEX REPLACE "\n$" "" PARTONS_GIT_REVISION "${PARTONS_GIT_REVISION}")

message("-- Git branch (revision): " ${PARTONS_GIT_BRANCH} " (" ${PARTONS_GIT_REVISION} ")")

# configure
configure_file(${CMAKE_SOURCE_DIR}/cmake/Version/PartonsVersion.h_in ${CMAKE_SOURCE_DIR}/include/partons/PartonsVersion.h)

# FIND LIBRARIES ===========================================================================

# find libraries: additional modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# find libraries: ElementaryUtils
find_package(ElementaryUtils REQUIRED)

# find libraries: NumA++
find_package(NumA++ REQUIRED)

# find libraries: SFML
find_package(SFML COMPONENTS system REQUIRED)

# find libraries: CLN
find_package(CLN REQUIRED)

# find libraries: GSL
find_package(GSL REQUIRED)

# find libraries: Apfel++
find_package(Apfel++ REQUIRED)

# find libraries: LHAPDF
find_package(LHAPDF REQUIRED)

# find libxml2
find_package(LibXml2 REQUIRED)

# LIBTORCH BACKEND (optional) ==============================================================
#
# Off by default: the library then has no dependency on LibTorch and is built
# exactly as before. With -DPARTONS_WITH_TORCH=ON the DVCSCFFTorch module (DVCS
# CFFs as batched, differentiable tensor operations on a fixed quadrature grid,
# sharing DVCSCFFStandard's coefficient functions through DVCSCFFKernels.h) and
# its check program test/torch/dvcs_cff_torch_check are compiled in.
#
#   cmake -DPARTONS_WITH_TORCH=ON -DCMAKE_PREFIX_PATH=/path/to/libtorch ...
#
# (the option itself is declared with the C++ standard at the top of this file)
set(PARTONS_TORCH_LIBRARIES "")
if(PARTONS_WITH_TORCH)
    find_package(Torch REQUIRED)
    add_definitions(-DPARTONS_WITH_TORCH)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
    include_directories(${TORCH_INCLUDE_DIRS})
    set(PARTONS_TORCH_LIBRARIES ${TORCH_LIBRARIES})
    message("-- LibTorch backend: ON (DVCSCFFTorch)")
else()
    message("-- LibTorch backend: OFF (use -DPARTONS_WITH_TORCH=ON to enable)")
endif()

# directories containing headers 
include_directories(${ElementaryUtils_INCLUDE_DIR} ${NumA++_INCLUDE_DIR} ${SFML_INCLUDE_DIR} ${CLN_INCLUDE_DIR} ${GSL_INCLUDE_DIRS} ${Apfel++_INCLUDE_DIR} ${LHAPDF_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIRS})

# FINALIZE ==================================================================================

# generate list of source files
file(

        GLOB_RECURSE

        source_files

        src/*
)

# define target library
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_library(

        PARTONS

        SHARED

        ${source_files}
)

# define libraries to be linked
target_link_libraries(

	PARTONS
          
	${SFML_LIBRARIES}
          
	${CLN_LIBRARIES}
          
	${ElementaryUtils_LIBRARIES}
          
	${NumA++_LIBRARIES}

	${GSL_LIBRARIES}

	${Apfel++_LIBRARIES}

	${LHAPDF_LIBRARIES}

	${LIBXML2_LIBRARIES}

	${PARTONS_TORCH_LIBRARIES}
)

# LibTorch backend check program: DVCSCFFTorch against DVCSCFFStandard on a
# standard GPD model, the batched API against the module path, and autograd.
if(PARTONS_WITH_TORCH)
    add_executable(dvcs_cff_torch_check test/torch/dvcs_cff_torch_check.cpp)
    target_link_libraries(dvcs_cff_torch_check
        PARTONS
        ${ElementaryUtils_LIBRARIES}
        ${NumA++_LIBRARIES}
        ${PARTONS_TORCH_LIBRARIES})
    # Partons::init() needs partons.properties (and what it points to) next to
    # the executable.
    add_custom_command(TARGET dvcs_cff_torch_check POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/test/torch/config
        $<TARGET_FILE_DIR:dvcs_cff_torch_check>)
    enable_testing()
    add_test(NAME dvcs_cff_torch_check
        COMMAND dvcs_cff_torch_check
        WORKING_DIRECTORY $<TARGET_FILE_DIR:dvcs_cff_torch_check>)
endif()

# install
install(TARGETS PARTONS 
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY data DESTINATION share)
