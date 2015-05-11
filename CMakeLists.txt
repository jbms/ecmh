cmake_minimum_required (VERSION 2.8)

# Define: -DBENCHMARK_INVERSION_CHAINS  for full inversion chain benchmarks.

set(RUN_BENCHMARK
  CACHE STRING "Command prefix for running benchmarks")

if (NOT CMAKE_CXX_COMPILER)
  set(CMAKE_C_COMPILER clang)
  set(CMAKE_CXX_COMPILER clang++)
endif()

project (multiset_hash)

# This has to happen after the project command, otherwise CMAKE_CXX_FLAGS_* won't be set properly.
if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)



## DEPENDENCIES
find_package(Boost COMPONENTS unit_test_framework program_options REQUIRED)
find_package(jbms-utility REQUIRED)
find_package(jbms-array_view REQUIRED)
find_package(jbms-openssl REQUIRED)
find_package(boost-endian REQUIRED)


enable_testing()

# clang -dM -E - < /dev/null  to see defines
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++1y -ggdb -Wall -Werror")
if (NOT C_ARCH_FLAGS)
  if (ARCH_NAME STREQUAL "core2")
    set(C_ARCH_FLAGS ${C_ARCH_FLAGS} -mmmx -msse -msse2 -msse3 -msse4.1)
  elseif (ARCH_NAME STREQUAL "westmere")
    set(C_ARCH_FLAGS ${C_ARCH_FLAGS} -mmmx -msse -msse2 -msse3 -msse4.1 -msse4.2 -mpopcnt -maes -mpclmul -DHAVE_RDTSCP)
  elseif (ARCH_NAME STREQUAL "sandybridge")
    set(C_ARCH_FLAGS ${C_ARCH_FLAGS} -mmmx -msse -msse2 -msse3 -msse4.1 -msse4.2 -mpopcnt -maes -mpclmul -mavx -DHAVE_RDTSCP)
  elseif (ARCH_NAME STREQUAL "ivybridge")
    set(C_ARCH_FLAGS ${C_ARCH_FLAGS} -mmmx -msse -msse2 -msse3 -msse4.1 -msse4.2 -mpopcnt -maes -mpclmul -mavx -mrdrnd -mf16c -DHAVE_RDTSCP)
  elseif (ARCH_NAME STREQUAL "haswell")
    set(C_ARCH_FLAGS ${C_ARCH_FLAGS} -mmmx -msse -msse2 -msse3 -msse4.1 -msse4.2 -mpopcnt -maes -mpclmul -mavx -mavx2 -mrdrnd -mfma -mbmi -mbmi2 -mf16c -DFAST_CLMUL -DHAVE_RDTSCP)
  else()
    set(C_ARCH_FLAGS -march=native)
  endif()
  message("Set C_ARCH_FLAGS to ${C_ARCH_FLAGS}")
endif()

function(add_arch_flags target)
  #get_target_property(temp ${target} COMPILE_OPTIONS)
  #message("options = ${temp}")
  target_compile_options("${target}" PUBLIC ${C_ARCH_FLAGS})
endfunction()

include_directories(
  ${jbms-utility_INCLUDE_DIRS}
  ${jbms-array_view_INCLUDE_DIRS}
  ${jbms-openssl_INCLUDE_DIRS}
  ${boost-endian_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIRS}
  )

include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/src")

function(get_modulus_with_underscores result_var)
  set(modulus ${ARGN})
  list(REMOVE_ITEM modulus 0)
  list(LENGTH modulus count)
  set(first 1)
  foreach(mvar ${modulus})
    if (first EQUAL 0) 
      set(result "${result}_")
    endif()
    set(first 0)
    set(result "${result}${mvar}")
  endforeach()
  set(${result_var} "${result}" PARENT_SCOPE)
endfunction()


add_custom_target(benchmark_results)
add_custom_target(benchmark_core)
add_custom_target(inversion_chains)
add_custom_target(inversion_chain_options)
add_custom_target(benchmark_inversion_chain_options)
add_custom_target(benchmarks)
add_custom_target(benchmark_inverse_variants)
add_dependencies(benchmarks benchmark_core benchmark_inverse_variants inversion_chains inversion_chain_options benchmark_inversion_chain_options)


function(benchmark_binary_field_core_operations)

  get_modulus_with_underscores(modulus_with_underscores ${ARGN})
  set(field_name "GF2_${modulus_with_underscores}")

  set(test_name "benchmark_core_${field_name}")
  set(bench_cpp_out "${CMAKE_CURRENT_BINARY_DIR}/src/${test_name}.cpp")

  set(gen "${PROJECT_SOURCE_DIR}/python/generate_binary_field_core_benchmark.py")
  set(gen_deps "${gen}")

  add_custom_command(
    OUTPUT "${bench_cpp_out}"
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src"
    COMMAND python2 "${gen}" --out_name="${bench_cpp_out}" ${ARGN}
    DEPENDS ${gen_deps}
    )

  add_executable("${test_name}" "${bench_cpp_out}")
  target_link_libraries("${test_name}" ${OPENSSL_LIBRARIES})
  add_arch_flags("${test_name}")
  set(result "benchmark_results/${test_name}")
  set(result_target "benchmark_results__${test_name}")

  # message("FIXME: skipping generation of ${result}")
  add_custom_command(
    OUTPUT "${result}"
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/benchmark_results"
    COMMAND "${PROJECT_SOURCE_DIR}/python/run_command_and_write_output.py" "${result}" ${RUN_BENCHMARK} "./${test_name}"
    
    # Skip dependency on the benchmark executable, to avoid excessive regeneration of the results when the code is rebuilt
    # Delete the results to regenerate them
    #DEPENDS "${test_name}"
    )

  add_custom_target(
    "${result_target}"
    DEPENDS "${result}"
    SOURCES "${result}")

  add_dependencies(benchmark_results "${result_target}")
  add_dependencies(benchmark_core "${test_name}")

endfunction()


function(benchmark_binary_field field_name field_header_basename)
  include ( CMakeParseArguments )

  cmake_parse_arguments (
    "_bench_arg"
    ""
    ""
    "ARGUMENTS;DEPENDENCIES"
    ${ARGN} )


  set(test_name "benchmark_${field_header_basename}")
  set(bench_cpp_out "${CMAKE_CURRENT_BINARY_DIR}/src/${test_name}.cpp")

  set(gen "${PROJECT_SOURCE_DIR}/python/generate_binary_field_benchmark.py")
  set(gen_deps "${gen}")

  add_custom_command(
    OUTPUT "${bench_cpp_out}"
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src"
    COMMAND python2 "${gen}"
    --out_name="${bench_cpp_out}" --field_name="${field_name}"
    --field_header_basename="${field_header_basename}"
    ${_bench_arg_ARGUMENTS}
    DEPENDS ${gen_deps}
    )

  add_executable("${test_name}" "${bench_cpp_out}" ${_bench_arg_DEPENDENCIES})
  target_link_libraries("${test_name}" ${OPENSSL_LIBRARIES})
  target_link_libraries("${test_name}" ${field_header_basename})
  add_dependencies(benchmarks "${test_name}")  
endfunction()

function(add_code_generator name)
  add_executable("${name}" "${CMAKE_CURRENT_SOURCE_DIR}/src/generate/${name}.cpp")
  target_link_libraries("${name}" ${OPENSSL_LIBRARIES} ${Boost_PROGRAM_OPTIONS_LIBRARY})
endfunction()

add_code_generator("generate_binary_field_code")
add_code_generator("generate_standard_binary_elliptic_curve") 
add_code_generator("generate_quadratic_extension_curve")

function(define_binary_field_inversion_chain_benchmark benchmark_name chain_data)
  set(gen_inv_options_benchmark "${PROJECT_SOURCE_DIR}/python/generate_binary_field_inversion_chain_options_benchmark.py")
  set(benchmark_cpp "${CMAKE_CURRENT_BINARY_DIR}/${benchmark_name}.cpp")
  
  add_custom_command(
    OUTPUT "${benchmark_cpp}"
    COMMAND python2 "${gen_inv_options_benchmark}"
    ${ARGN}
    --chain_data ${chain_data}
    --out_name="${benchmark_cpp}"
    DEPENDS "${gen_inv_options_benchmark}" ${chain_data}
    )

  add_executable("${benchmark_name}" "${benchmark_cpp}")
  add_arch_flags("${benchmark_name}")
endfunction()  
    

function(define_binary_field_inversion_chain_options)
  get_modulus_with_underscores(modulus_with_underscores ${ARGN})
  set(field_name "GF2_${modulus_with_underscores}")

  # Inverse pareto frontier generation
  foreach(suffix "" "_minimum_length")
    set(gen_inv_options "${PROJECT_SOURCE_DIR}/python/generate_binary_field_inversion_chain${suffix}_options.py")
    set(inv_options "${CMAKE_CURRENT_BINARY_DIR}/${field_name}_inversion-chain${suffix}-options.json")
    set(extra_deps)
    set(extra_args)
    if (suffix STREQUAL "")
      set(extra_deps "${benchmark_data}")
      set(extra_args --benchmark_data="${benchmark_data}")
    endif()

    add_custom_command(
      OUTPUT "${inv_options}"
      COMMAND python2 "${gen_inv_options}" --output="${inv_options}"
      ${extra_args}
      ${ARGN}
      --multi_square_block_size 4 6 8 10 12
      DEPENDS ${gen_inv_options} "${PROJECT_SOURCE_DIR}/python/generate_chains.py" ${extra_deps}
      )
    set(inv_options_target "${field_name}_inversion-chain${suffix}-options")
    add_custom_target(
      ${inv_options_target}
      SOURCES "${inv_options}")

    add_dependencies(inversion_chain_options "${inv_options_target}")

    set(benchmark_name "benchmark_${field_name}_inversion-chain${suffix}-options")
    define_binary_field_inversion_chain_benchmark("${benchmark_name}" "${inv_options}" ${ARGN})
    add_dependencies(benchmark_inversion_chain_options "${benchmark_name}")
  endforeach()

endfunction()


function(define_base_binary_field_library field_name suffix chain_data)
  set(cpp_out "${CMAKE_CURRENT_BINARY_DIR}/src/jbms/binary_field/${field_name}${suffix}.cpp")
  set(h_out "${CMAKE_CURRENT_BINARY_DIR}/src/jbms/binary_field/${field_name}${suffix}.hpp")
  set(gen "${PROJECT_SOURCE_DIR}/python/generate_binary_field_code.py")

  add_custom_command(
    OUTPUT "${cpp_out}" "${h_out}"
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src/jbms/binary_field"
    COMMAND python2 "${gen}" --h_name="${h_out}" --cpp_name="${cpp_out}"
    --generator_program="${CMAKE_CURRENT_BINARY_DIR}/generate_binary_field_code"
    --field_name="${field_name}${suffix}"
    --inversion_chain="${chain_data}"
    --copy_to_huge_pages=1
    --half_trace_block_size=8
    ${ARGN}
    DEPENDS "${gen}" "${chain_data}" "generate_binary_field_code"
    )

  set(obj_lib ${field_name}${suffix})

  add_library(${obj_lib} STATIC ${cpp_out})
  target_link_libraries(${obj_lib} ${OPENSSL_LIBRARIES})
  add_arch_flags("${obj_lib}")

  set(test_name "test_${field_name}${suffix}")
  set(test_gen "${PROJECT_SOURCE_DIR}/python/generate_binary_field_base_test.py")
  set(test_cpp_out "${CMAKE_CURRENT_BINARY_DIR}/src/test_${field_name}${suffix}.cpp")
  add_custom_command(
    OUTPUT "${test_cpp_out}"
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src"
    COMMAND python2 "${test_gen}" --out_name="${test_cpp_out}" --field_name="${field_name}${suffix}" ${ARGN}
    DEPENDS ${test_gen}
    )

  add_executable("${test_name}" "${test_cpp_out}")
  target_link_libraries("${test_name}" ${OPENSSL_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${obj_lib})
  set_property(TARGET "${test_name}" PROPERTY COMPILE_DEFINITIONS BOOST_TEST_MAIN BOOST_TEST_DYN_LINK)

  add_test("${test_name}" ${RUN_BENCHMARK} "./${test_name}")

  benchmark_binary_field(${field_name} ${field_name}${suffix}
    #ARGUMENTS --linear_transforms
    )
endfunction()  

function(define_binary_field)
  get_modulus_with_underscores(modulus_with_underscores ${ARGN})
  set(field_name "GF2_${modulus_with_underscores}")
  set(chain_gen "${PROJECT_SOURCE_DIR}/python/generate_binary_field_inversion_chain.py")
  set(gen "${PROJECT_SOURCE_DIR}/python/generate_binary_field_code.py")
  set(benchmark_data "${CMAKE_CURRENT_BINARY_DIR}/benchmark_results/benchmark_core_${field_name}")

  benchmark_binary_field_core_operations(${ARGN})
  
  if (BENCHMARK_INVERSION_CHAINS)
    define_binary_field_inversion_chain_options(${ARGN})
  endif()

  # Inverse variants
  set(inverse_variants none unoptimized single multi)
  set(inverse_suffix "_inv-none" "_inv-unoptimized" "_inv-single" "")
  set(all_chains)
  foreach(inv_var_i 0 1 2 3)
    list(GET inverse_variants ${inv_var_i} inverse_variant)
    list(GET inverse_suffix ${inv_var_i} suffix)

    set(cpp_out "${CMAKE_CURRENT_BINARY_DIR}/src/jbms/binary_field/${field_name}${suffix}.cpp")
    set(h_out "${CMAKE_CURRENT_BINARY_DIR}/src/jbms/binary_field/${field_name}${suffix}.hpp")
    set(chain_out "${CMAKE_CURRENT_BINARY_DIR}/${field_name}${suffix}_inversion-chain.json")
    set(chain_deps "${chain_gen}" "${PROJECT_SOURCE_DIR}/python/generate_chains.py" "${benchmark_data}")
    set(all_chains ${all_chains} ${chain_out})

    set(alternate_chain "")
    if (${inv_var_i} GREATER 1)
      math(EXPR prev_inv_var_i "${inv_var_i} - 1")
      list(GET inverse_suffix ${prev_inv_var_i} alt_suffix)
      set(alternate_chain "${CMAKE_CURRENT_BINARY_DIR}/${field_name}${alt_suffix}_inversion-chain.json")
      set(chain_deps ${chain_deps} "${field_name}${alt_suffix}_inversion-chain")
    endif()

    add_custom_command(
      OUTPUT "${chain_out}"
      COMMAND python2 "${chain_gen}" --output="${chain_out}"
      --benchmark_data="${benchmark_data}"
      --multi_square_block_size=8
      --inverse_table_opt="${inverse_variant}"
      --alternate_chain="${alternate_chain}"
      ${ARGN}
      DEPENDS ${chain_deps}
      )

    set(chain_target "${field_name}${suffix}_inversion-chain")
    add_custom_target(
      ${chain_target}
      SOURCES "${chain_out}")

    add_dependencies(inversion_chains "${chain_target}")

    if (${inv_var_i} EQUAL 3)
      # Only generate the library for the full variant
      define_base_binary_field_library("${field_name}" "${suffix}" "${chain_out}" ${ARGN})
    endif()

  endforeach() # foreach inverse variant

  if (BENCHMARK_INVERSION_CHAINS)
    define_binary_field_inversion_chain_benchmark("benchmark_${field_name}_inverse-variants" "${all_chains}" ${ARGN})
    add_dependencies(benchmark_inverse_variants "benchmark_${field_name}_inverse-variants")
  endif()
endfunction()

function(define_binary_field_quadratic_extension m)
  get_modulus_with_underscores(modulus_with_underscores ${m} ${ARGN})
  math(EXPR m2 "${m} * 2")
  set(field_name "GF2_${m2}")
  set(h_out "${CMAKE_CURRENT_BINARY_DIR}/src/jbms/binary_field/${field_name}.hpp")
  set(gen "${PROJECT_SOURCE_DIR}/python/generate_binary_field_quadratic_extension_code.py")
  add_custom_command(
    OUTPUT "${h_out}"
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src/jbms/binary_field"
    COMMAND python2 "${gen}" --h_name="${h_out}" "${m}" ${ARGN}
    DEPENDS ${gen}
    )

  set(obj_lib "${field_name}")
  set(base_field_name "GF2_${modulus_with_underscores}")
  add_library(${obj_lib} STATIC "${h_out}")
  target_link_libraries("${obj_lib}" "${base_field_name}")
  add_arch_flags("${obj_lib}")

  set(test_name "test_${field_name}")
  set(test_gen "${PROJECT_SOURCE_DIR}/python/generate_binary_field_quadratic_extension_test.py")
  set(test_cpp_out "${CMAKE_CURRENT_BINARY_DIR}/src/test_${field_name}.cpp")
  add_custom_command(
    OUTPUT "${test_cpp_out}"
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src"
    COMMAND python2 "${test_gen}" --out_name="${test_cpp_out}" "${m2}"
    DEPENDS ${test_gen}
    )

  add_executable("${test_name}" "${test_cpp_out}")
  target_link_libraries("${test_name}" ${OPENSSL_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} "${obj_lib}")
  set_property(TARGET "${test_name}" PROPERTY COMPILE_DEFINITIONS BOOST_TEST_MAIN BOOST_TEST_DYN_LINK)

  add_test("${test_name}" ${RUN_BENCHMARK} "./${test_name}")

  benchmark_binary_field(${field_name} ${field_name})
endfunction()


function(define_binary_elliptic_curve_test_and_benchmark curve_name)

  include ( CMakeParseArguments )

  cmake_parse_arguments (
    "_bench_arg"
    ""
    ""
    "TEST_ARGUMENTS"
    ${ARGN} )

  set(obj_lib ec_${curve_name})

  # Test
  set(test_name "test_ec_${curve_name}")
  set(test_gen "${PROJECT_SOURCE_DIR}/python/generate_binary_elliptic_curve_test.py")
  set(test_cpp_out "${CMAKE_CURRENT_BINARY_DIR}/src/${test_name}.cpp")
  add_custom_command(
    OUTPUT "${test_cpp_out}"
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src"
    COMMAND python2 "${test_gen}" --out_name="${test_cpp_out}" --curve="${curve_name}" ${_bench_arg_TEST_ARGUMENTS}
    DEPENDS ${test_gen}
    )

  add_executable("${test_name}" "${test_cpp_out}")
  target_link_libraries("${test_name}" ${OPENSSL_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} "${obj_lib}")
  set_property(TARGET "${test_name}" PROPERTY COMPILE_DEFINITIONS BOOST_TEST_MAIN BOOST_TEST_DYN_LINK)

  add_test("${test_name}" ${RUN_BENCHMARK} "./${test_name}")

  # Benchmark
  set(test_name "benchmark_ec_${curve_name}")
  set(bench_cpp_out "${CMAKE_CURRENT_BINARY_DIR}/src/${test_name}.cpp")

  set(gen "${PROJECT_SOURCE_DIR}/python/generate_binary_elliptic_curve_benchmark.py")
  set(gen_deps "${gen}")

  add_custom_command(
    OUTPUT "${bench_cpp_out}"
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src"
    COMMAND python2 "${gen}" --out_name="${bench_cpp_out}" --curve_name="${curve_name}"
    DEPENDS ${gen_deps}
    )

  add_executable("${test_name}" "${bench_cpp_out}")
  target_link_libraries("${test_name}" ${OPENSSL_LIBRARIES} "${obj_lib}")
  add_dependencies(benchmarks "${test_name}")  

endfunction()


function(define_standard_binary_elliptic_curve curve_name)
  get_modulus_with_underscores(modulus_with_underscores ${ARGN})
  set(field_name "GF2_${modulus_with_underscores}")
  set(out_dir "${CMAKE_CURRENT_BINARY_DIR}/src/jbms/binary_elliptic_curve")
  set(cpp_out "${out_dir}/${curve_name}.cpp")
  set(h_out "${out_dir}/${curve_name}.hpp")
  set(gen "generate_standard_binary_elliptic_curve")
  set(gen_deps "${gen}")
  add_custom_command(
    OUTPUT "${cpp_out}" "${h_out}"
    COMMAND mkdir -p "${out_dir}"
    COMMAND "./${gen}" --h_name="${h_out}" --cpp_name="${cpp_out}" --curve="${curve_name}"
    DEPENDS ${gen_deps}
    )

  set(obj_lib ec_${curve_name})

  add_library(${obj_lib} STATIC ${cpp_out})
  target_link_libraries(${obj_lib} ${field_name})

  define_binary_elliptic_curve_test_and_benchmark(${curve_name} TEST_ARGUMENTS --against_openssl)
endfunction() # define_standard_elliptic_curve

function(define_quadratic_extension_curve curve_name base_field_modulus a b)
  list(GET base_field_modulus 0 m)
  math(EXPR m2 "${m} * 2")
  set(field_name "GF2_${m2}")
  set(out_dir "${CMAKE_CURRENT_BINARY_DIR}/src/jbms/binary_elliptic_curve")
  set(cpp_out "${out_dir}/${curve_name}.cpp")
  set(h_out "${out_dir}/${curve_name}.hpp")
  set(gen "generate_quadratic_extension_curve")
  set(gen_deps "${gen}")
  set(modulus_arg)
  foreach(mvar ${base_field_modulus})
    set(modulus_arg ${modulus_arg} --modulus="${mvar}")
  endforeach()
  add_custom_command(
    OUTPUT "${cpp_out}" "${h_out}"
    COMMAND mkdir -p "${out_dir}"
    COMMAND "./${gen}" --h_name="${h_out}" --cpp_name="${cpp_out}" --curve="${curve_name}" --a="${a}" --b="${b}" ${modulus_arg}
    DEPENDS ${gen_deps}
    )

  set(obj_lib ec_${curve_name})

  add_library(${obj_lib} STATIC ${cpp_out})
  target_link_libraries(${obj_lib} ${field_name})

  define_binary_elliptic_curve_test_and_benchmark(${curve_name})
endfunction()

function(define_ecmh curve_name hash_name )

  foreach(suffix "" "_blinded")

    set(extra_args --blinded)
    if (suffix STREQUAL "")
      set(extra_args)
    endif()
    
    set(test_name "test_ecmh_${curve_name}_${hash_name}${suffix}")
    set(test_gen "${PROJECT_SOURCE_DIR}/python/generate_ecmh_test.py")
    set(test_cpp_out "${CMAKE_CURRENT_BINARY_DIR}/src/${test_name}${suffix}.cpp")
    add_custom_command(
      OUTPUT "${test_cpp_out}"
      COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src"
      COMMAND python2 "${test_gen}" --out_name="${test_cpp_out}" --curve="${curve_name}" --hash="${hash_name}" ${extra_args}
      DEPENDS ${test_gen}
      )

    add_executable("${test_name}" "${test_cpp_out}")
    target_link_libraries("${test_name}"
      ${OPENSSL_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
      "${hash_name}"
      "ec_${curve_name}"
      )
    set_property(TARGET "${test_name}" PROPERTY COMPILE_DEFINITIONS BOOST_TEST_MAIN BOOST_TEST_DYN_LINK)

    add_test("${test_name}" ${RUN_BENCHMARK} "./${test_name}")


    set(bench_name "benchmark_ecmh_${curve_name}_${hash_name}${suffix}")
    set(bench_gen "${PROJECT_SOURCE_DIR}/python/generate_ecmh_benchmark.py")
    set(bench_cpp_out "${CMAKE_CURRENT_BINARY_DIR}/src/${bench_name}${suffix}.cpp")
    add_custom_command(
      OUTPUT "${bench_cpp_out}"
      COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src"
      COMMAND python2 "${bench_gen}" --out_name="${bench_cpp_out}" --curve="${curve_name}" --hash="${hash_name}" ${extra_args}
      DEPENDS ${bench_gen}
      )

    add_executable("${bench_name}" "${bench_cpp_out}")
    target_link_libraries("${bench_name}"
      ${OPENSSL_LIBRARIES}
      "${hash_name}"
      "ec_${curve_name}"
      )

    add_dependencies(benchmarks "${bench_name}")

  endforeach() # blinded vs regular

endfunction()

function(define_simple_multiset_hash name hash_name)

  set(test_name "test_${name}_${hash_name}")
  set(test_gen "${PROJECT_SOURCE_DIR}/python/generate_simple_multiset_hash_test.py")
  set(test_cpp_out "${CMAKE_CURRENT_BINARY_DIR}/src/${test_name}.cpp")
  add_custom_command(
    OUTPUT "${test_cpp_out}"
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src"
    COMMAND python2 "${test_gen}" --out_name="${test_cpp_out}" --name="${name}" --hash="${hash_name}"
    DEPENDS ${test_gen}
    )

  add_executable("${test_name}" "${test_cpp_out}")
  target_link_libraries("${test_name}"
    ${OPENSSL_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    "${hash_name}"
    )
  set_property(TARGET "${test_name}" PROPERTY COMPILE_DEFINITIONS BOOST_TEST_MAIN BOOST_TEST_DYN_LINK)

  add_test("${test_name}" ${RUN_BENCHMARK} "./${test_name}")


  set(bench_name "benchmark_${name}_${hash_name}")
  set(bench_gen "${PROJECT_SOURCE_DIR}/python/generate_simple_multiset_hash_benchmark.py")
  set(bench_cpp_out "${CMAKE_CURRENT_BINARY_DIR}/src/${bench_name}.cpp")
  add_custom_command(
    OUTPUT "${bench_cpp_out}"
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/src"
    COMMAND python2 "${bench_gen}" --out_name="${bench_cpp_out}" --name="${name}" --hash="${hash_name}"
    DEPENDS ${bench_gen}
    )

  add_executable("${bench_name}" "${bench_cpp_out}")
  target_link_libraries("${bench_name}"
    ${OPENSSL_LIBRARIES}
    "${hash_name}"
    )
  add_dependencies(benchmarks "${bench_name}")  

endfunction()


function(define_hash hash_name)
  add_library(${hash_name} STATIC "${PROJECT_SOURCE_DIR}/src/jbms/hash/${hash_name}/${hash_name}.cpp")
  add_arch_flags(${hash_name})
endfunction()

define_hash(blake2s)
define_hash(blake2b)


set(mod127 127 63 0)
define_binary_field(${mod127})
set(mod163 163 7 6 3 0)
define_binary_field(${mod163})
set(mod193 193 15 0)
define_binary_field(${mod193})
set(mod233 233 74 0)
define_binary_field(${mod233})
set(mod239 239 158 0)
define_binary_field(${mod239})
set(mod283 283 12 7 5 0)
define_binary_field(${mod283})
set(mod409 409 87 0)
define_binary_field(${mod409})
set(mod571 571 10 5 2 0)
define_binary_field(${mod571})

define_binary_field_quadratic_extension(${mod127})

define_quadratic_extension_curve(GLS254 "${mod127}" "0,1" "59C8202CB9E6E0AE2E6D944FA54DE7E5,0")

define_standard_binary_elliptic_curve(sect163k1 ${mod163}) # NIST/SECG/WTLS curve over a 163 bit binary field
define_standard_binary_elliptic_curve(sect163r1 ${mod163}) # SECG curve over a 163 bit binary field
define_standard_binary_elliptic_curve(sect163r2 ${mod163}) # NIST/SECG curve over a 163 bit binary field
define_standard_binary_elliptic_curve(sect193r1 ${mod193}) # SECG curve over a 193 bit binary field
define_standard_binary_elliptic_curve(sect193r2 ${mod193}) # SECG curve over a 193 bit binary field
define_standard_binary_elliptic_curve(sect233k1 ${mod233}) # NIST/SECG/WTLS curve over a 233 bit binary field
define_standard_binary_elliptic_curve(sect233r1 ${mod233}) # NIST/SECG/WTLS curve over a 233 bit binary field
define_standard_binary_elliptic_curve(sect239k1 ${mod239}) # SECG curve over a 239 bit binary field
define_standard_binary_elliptic_curve(sect283k1 ${mod283}) # NIST/SECG curve over a 283 bit binary field
define_standard_binary_elliptic_curve(sect283r1 ${mod283}) # NIST/SECG curve over a 283 bit binary field
define_standard_binary_elliptic_curve(sect409k1 ${mod409}) # NIST/SECG curve over a 409 bit binary field
define_standard_binary_elliptic_curve(sect409r1 ${mod409}) # NIST/SECG curve over a 409 bit binary field
define_standard_binary_elliptic_curve(sect571k1 ${mod571}) # NIST/SECG curve over a 571 bit binary field
define_standard_binary_elliptic_curve(sect571r1 ${mod571}) # NIST/SECG curve over a 571 bit binary field

add_executable("benchmark_mont_mul" "${CMAKE_CURRENT_SOURCE_DIR}/src/test/binary_field/benchmark_mont_mul.cpp")
target_link_libraries("benchmark_mont_mul" ${OPENSSL_LIBRARIES})


foreach(hash blake2s blake2b)
  foreach(curve GLS254 sect163k1 sect193r1 sect233k1 sect239k1 sect283k1 sect409k1 sect571k1
      )
    define_ecmh(${curve} ${hash})
  endforeach()

  foreach(name muhash adhash)
    define_simple_multiset_hash(${name} ${hash})
  endforeach()
endforeach()
