# PQL-014 C++20 review

Decision: C++ REVIEW PASSED, independent final review by architect.

Curl runtime/handle/buffer/URL lifetimes are explicit. C callbacks contain exceptions,
bound body size and never propagate C++ exceptions through curl. TLS verification,
HTTPS endpoint restriction, disabled redirects and fixed diagnostics avoid leaking
credentials or raw response bodies. Query parameters containing credentials are
never logged. The duplicate-key parser stack rejects duplicates before DOM overwrite.

Repository batch validation precedes writes, asset locks serialize writers and the
existing unit-of-work failure guard prevents partial commits. Date arithmetic uses
integer session labels. Retry and per-instance pacing waits are injectable for tests.

Root compiled Debug and Release with GCC14.2/C++20 warnings as errors, libpqxx7.10,
libcurl8.14.1 and nlohmann_json3.11.3. No material C++ findings remain. Native MSVC,
sanitizers and real transport fault injection were not run.
