# BSRational("." ) makes a later multiply index an empty vector

Input `\x01.` (BSRational path, string `"."`) is accepted by the `BSRational(std::string const&)` constructor.
The resulting value has an empty `UIntegerAP32` bit vector, and `x * x` then reaches
`UIntegerALU32<UIntegerAP32>::Mul` (`GTE/Mathematics/UIntegerALU32.h:326`), which indexes `vector::operator[]`
on a null data pointer: UBSan reports `reference binding to null pointer of type 'const unsigned int'`.

Reproduce: `/mayhem/fuzz_bsnumber_string-standalone mayhem/fuzz_bsnumber_string/known-findings/bsrational-dot-zero-mul/reproducer.bin`
