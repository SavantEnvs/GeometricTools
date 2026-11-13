# STLBinaryFile::Load allocates from the untrusted triangle count

`STLBinaryFile::Load` (`GTE/Mathematics/STLBinaryFile.h:71`) does `triangles.resize(numTriangles)` with the
`uint32_t` count read from the file header before checking that the file holds that many records. An
80-byte header plus a 4-byte count such as `0x6d6d6d6d` makes a 100-byte file request tens of GB; libFuzzer
reports `out-of-memory` (rss limit) instead of the loader rejecting the truncated file.

Reproduce: `/mayhem/fuzz_stl_binary mayhem/fuzz_stl_binary/known-findings/triangle-count-allocation/reproducer.stl`
(libFuzzer default 2 GB rss limit; the standalone binary may instead hit `std::bad_alloc` or succeed lazily depending on overcommit).
