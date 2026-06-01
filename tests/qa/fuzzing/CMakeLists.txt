# tests/qa/fuzzing/CMakeLists.txt
# Opt-in: -DNIFT_QA_FUZZING=ON. Requires clang with libFuzzer.
if(NOT NIFT_QA_FUZZING)
    return()
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "QA fuzzing requires Clang; skipping.")
    return()
endif()

set(FUZZ_FLAGS -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer -g -O1)
set(FUZZ_LINK  -fsanitize=fuzzer,address,undefined)

function(nift_add_fuzz name source target_link)
    add_executable(${name} ${source})
    target_compile_options(${name} PRIVATE ${FUZZ_FLAGS})
    target_link_options(${name}    PRIVATE ${FUZZ_LINK})
    target_link_libraries(${name}  PRIVATE ${target_link})
    target_compile_features(${name} PRIVATE cxx_std_20)
endfunction()

nift_add_fuzz(fuzz_parser        fuzz_parser.cpp        nift::parser)
nift_add_fuzz(fuzz_simd_scanner  fuzz_simd_scanner.cpp  nift::build)
nift_add_fuzz(fuzz_cli           fuzz_cli.cpp           nift::cli)
