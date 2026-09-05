# PQL-011 C++20 quality review

Decision: C++ REVIEW PASSED after embedded-NUL fix.

Architect performed an independent C++/ownership review. The pimpl declares the
connection before pqxx::work, so reverse destruction rolls back before connection
closure. Noncopyable ownership and documented thread confinement prevent accidental
sharing. Public headers do not expose pqxx. The internal run template has a concrete
purpose: every repository entrypoint applies identical transaction-poisoning behavior.

Parameters are bound, not interpolated. Checked numeric and timestamp conversions
avoid silent narrowing. Text validation now rejects NUL before binding names,
sources and options; the reviewed regression verifies rejection and prior-write
rollback. Exceptions expose sanitized errors and distinguish uncertain commit.

Root compiled with GCC 14.2, C++20 and -Wall -Wextra -Wpedantic -Werror against
libpqxx 7.10.0. All tests passed. Native MSVC linking and sanitizer execution of this
adapter were not performed. No unresolved material findings.
