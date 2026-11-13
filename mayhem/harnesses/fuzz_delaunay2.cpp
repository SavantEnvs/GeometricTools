// Fuzzes gte::Delaunay2<double> (interval + exact rational predicates) on point sets decoded from the
// input (pairs of int16 coordinates, so every coordinate is finite), then walks the triangulation.
#include <Mathematics/Delaunay2.h>

#include <array>
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
        gte::Delaunay2<double> delaunay;
        if (delaunay(points) && delaunay.GetDimension() == 2)
        {
            std::array<int32_t, 3> tri{};
            for (size_t t = 0; t < delaunay.GetNumTriangles(); ++t)
            {
                delaunay.GetIndices(t, tri);
            }
            std::vector<size_t> hull;
            delaunay.GetHull(hull);
            if (!points.empty())
            {
                gte::Delaunay2<double>::SearchInfo info{};
                delaunay.GetContainingTriangle(points[0], info);
            }
        }
    }
    catch (std::runtime_error const&)
    {
        return -1;
    }
    return 0;
}
