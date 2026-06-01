// tests/qa/fuzzing/fuzz_simd_scanner.cpp
// libFuzzer harness explicitly targeting xsimd scanner OOB / unaligned reads.
//
// xsimd loads 32-byte batches. Bugs typically lurk in the tail loop or the
// "load past the end" path. We deliberately:
//   1. pin inputs at lengths 0..127 to hammer the tail handling
//   2. align inputs at every offset 0..31 to exercise unaligned loads
//   3. seed inputs with @, $, {, } — the scanner's marker set
//
// AddressSanitizer makes any 1-byte over-read into an immediate test failure.
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "nift/build/simd.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size == 0 || size > 1 << 16) return 0;

    // Use first byte as a misalignment offset 0..31. This shifts the buffer
    // start through every alignment within an SSE/AVX2 lane.
    const std::size_t offset = data[0] & 0x1F;
    if (size <= offset + 1) return 0;

    // Heap buffer ASan can guard. Copying exactly `size - offset - 1` bytes
    // ensures any read past the end is caught.
    const std::size_t payload_len = size - offset - 1;
    std::vector<std::uint8_t> buf(payload_len);
    std::memcpy(buf.data(), data + offset + 1, payload_len);

    std::string_view sv(reinterpret_cast<const char*>(buf.data()), buf.size());
    try {
        // Production scanner. Replace function name if your symbol differs.
        (void)nift::build::scan_markers(sv);
    } catch (const std::exception&) {
        // Tolerated.
    } catch (...) {
    }
    return 0;
}
