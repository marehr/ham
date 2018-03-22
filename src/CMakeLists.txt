# Copyright (c) 2018 Marcel Ehrhardt <marcel.ehrhardt@fu-berlin.de>
# Copyright (c) 2018 Matthias Noack <ma.noack.pr@gmail.com>
#
# See accompanying file LICENSE and README for further information.

project(ham_exe LANGUAGES CXX)
cmake_minimum_required(VERSION 3.2 FATAL_ERROR) # TODO verfify

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -Wall -Wextra")

add_subdirectory(ham)

### Benchmarks

## Explicit targets (not built by default)
# Intel LEO offload directive benchmark, requires Intel compiler
if (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
	add_executable(benchmark_intel_leo EXCLUDE_FROM_ALL benchmark_intel_leo.cpp)
	target_link_libraries(benchmark_intel_leo ham_interface)
endif ()

if (MPI_FOUND)
	add_executable (benchmark_ham_offload_mpi benchmark_ham_offload.cpp)
	target_link_libraries (benchmark_ham_offload_mpi ham_offload_mpi)
endif()

if (SCIF_FOUND)
	add_executable (benchmark_ham_offload_scif benchmark_ham_offload.cpp)
	target_link_libraries (benchmark_ham_offload_scif ham_offload_scif)
endif()

### Examples/Tests

# some tests for the active msg layer
add_executable(active_msgs active_msgs.cpp)
target_link_libraries(active_msgs ham_interface)

if (MPI_FOUND)
	add_executable(ham_offload_test_mpi ham_offload.cpp)
	target_link_libraries(ham_offload_test_mpi ham_offload_mpi)

	add_executable(ham_offload_test_explicit_mpi ham_offload_explicit.cpp)
	target_link_libraries(ham_offload_test_explicit_mpi ham_offload_mpi_explicit)

	add_executable(inner_product_mpi inner_product.cpp)
	target_link_libraries(inner_product_mpi ham_offload_mpi)

	add_executable(test_data_transfer_mpi test_data_transfer.cpp)
	target_link_libraries(test_data_transfer_mpi ham_offload_mpi)

	add_executable(test_argument_transfer_mpi test_argument_transfer.cpp)
	target_link_libraries(test_argument_transfer_mpi ham_offload_mpi)
endif()

if (SCIF_FOUND)
	add_executable(ham_offload_test_scif ham_offload.cpp)
	target_link_libraries(ham_offload_test_scif ham_offload_scif)

	add_executable(ham_offload_test_explicit_scif ham_offload_explicit.cpp)
	target_link_libraries(ham_offload_test_explicit_scif ham_offload_scif_explicit)

	add_executable(inner_product_scif inner_product.cpp)
	target_link_libraries(inner_product_scif ham_offload_scif)

	add_executable(test_data_transfer_scif test_data_transfer.cpp)
	target_link_libraries(test_data_transfer_scif ham_offload_scif)

	add_executable(test_argument_transfer_scif test_argument_transfer.cpp)
	target_link_libraries(test_argument_transfer_scif ham_offload_scif)
endif()

if (MPI_FOUND)
  set(HAM_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
  add_executable (two_target_inner_product_mpi two_target_inner_product.cpp)
  target_link_libraries (two_target_inner_product_mpi ham_offload_mpi)

  add_executable(serialize_vector_mpi serialize_vector.cpp)
  target_compile_features(serialize_vector_mpi PUBLIC cxx_std_17)
  target_link_libraries(serialize_vector_mpi ham_offload_mpi)
  target_include_directories(serialize_vector_mpi PUBLIC "${HAM_ROOT}/cereal/include")

  add_executable(seqan_offload_mpi seqan_offload.cpp)
  target_compile_features(seqan_offload_mpi PUBLIC cxx_std_17)
  target_link_libraries(seqan_offload_mpi ham_offload_mpi)
  target_compile_options(seqan_offload_mpi PUBLIC -fconcepts)
  target_include_directories(seqan_offload_mpi PUBLIC "${HAM_ROOT}/cereal/include")
  target_include_directories(seqan_offload_mpi PUBLIC "${HAM_ROOT}/../seqan3/include/")
  target_include_directories(seqan_offload_mpi PUBLIC "${HAM_ROOT}/../seqan3/submodules/range-v3/include/")
endif ()
