// Fuzzes gte::ConvexHull2<double> on point sets decoded from the input (pairs of int16 coordinates,
// so every coordinate is finite, which is the library's input contract).
#include <Mathematics/ConvexHull2.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
    size_t const numPoints = size / 4;
    if (numPoints == 0)
    {
        return -1;
    }

    std::vector<gte::Vector2<double>> points(numPoints);
    for (size_t i = 0; i < numPoints; ++i)
    {
        int16_t x, y;
        std::memcpy(&x, data + 4 * i, 2);
        std::memcpy(&y, data + 4 * i + 2, 2);
        points[i] = { static_cast<double>(x), static_cast<double>(y) };
    }

    try
    {
        gte::ConvexHull2<double> hull;
        hull(points);
        for (int32_t index : hull.GetHull())
        {
            volatile int32_t v = index;
            (void)v;
        }
    }
    catch (std::runtime_error const&)
    {
        return -1;
    }
    return 0;
}
