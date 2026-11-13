// Fuzzes the decimal-string constructors of gte::BSNumber (integers) and gte::BSRational
// (fixed-point/scientific decimals), then exercises exact arithmetic on a parsed value. The
// library reports malformed numbers with std::runtime_error (LogAssert); that is a rejection.
#include <Mathematics/ArbitraryPrecision.h>

#include <cstdint>
#include <stdexcept>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
    if (size < 2)
    {
        return -1;
    }

    uint8_t mode = data[0];
    std::string text(reinterpret_cast<char const*>(data + 1), size - 1);

    try
    {
        if ((mode & 1) == 0)
        {
            using Number = gte::BSNumber<gte::UIntegerAP32>;
            Number x(text);
            Number y = x * x - x;
            volatile double d = static_cast<double>(y);
            (void)d;
            volatile bool lt = x < y;
            (void)lt;
        }
        else
        {
            using Number = gte::BSRational<gte::UIntegerAP32>;
            Number x(text);
            Number y = x * x + x;
            volatile double d = static_cast<double>(y);
            (void)d;
            volatile bool lt = x < y;
            (void)lt;
        }
    }
    catch (std::runtime_error const&)
    {
        return -1;
    }
    return 0;
}
