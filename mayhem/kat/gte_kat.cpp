// Known-answer probe over the GTE entry points the harnesses fuzz. It prints one "name value" line per
// check; mayhem/test.sh compares every line against values worked out independently of GTE.
// Usage: gte_kat <two_triangles.stl>
#include <Mathematics/ArbitraryPrecision.h>
#include <Mathematics/ConvexHull2.h>
#include <Mathematics/Delaunay2.h>
#include <Mathematics/NaturalCubicSpline.h>
#include <Mathematics/STLBinaryFile.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::fprintf(stderr, "usage: gte_kat <two_triangles.stl>\n");
        return 2;
    }

    {
        using N = gte::BSNumber<gte::UIntegerAP32>;
        N x("123456789012345678901234567890"), y("987654321");
        N z = x * y;
        std::printf("bsnumber_product_double %.17g\n", static_cast<double>(z));
        std::printf("bsnumber_product_equals %d\n", z == N("121932631124828532112482853211126352690") ? 1 : 0);
        std::printf("bsnumber_difference_sign %d\n", (N("-5") - N("7")).GetSign());
    }

    {
        using R = gte::BSRational<gte::UIntegerAP32>;
        R sum = R("0.1") + R("0.2");
        std::printf("bsrational_sum_double %.17g\n", static_cast<double>(sum));
        std::printf("bsrational_quarter_third %.17g\n", static_cast<double>(R("0.25") * R("0.5")));
    }

    {
        std::vector<gte::Vector2<double>> pts{ {0, 0}, {4, 0}, {2, 1}, {4, 4}, {1, 3}, {0, 4}, {2, 2} };
        gte::ConvexHull2<double> hull;
        hull(pts);
        std::string indices;
        for (int32_t i : hull.GetHull())
        {
            indices += std::to_string(i) + ",";
        }
        std::printf("convex_hull2_dimension %d\n", hull.GetDimension());
        std::printf("convex_hull2_indices %s\n", indices.c_str());
    }

    {
        std::vector<gte::Vector2<double>> pts{ {0, 0}, {4, 0}, {4, 4}, {0, 4}, {2, 2} };
        gte::Delaunay2<double> delaunay;
        delaunay(pts);
        std::printf("delaunay2_dimension %zu\n", static_cast<size_t>(delaunay.GetDimension()));
        std::printf("delaunay2_triangles %zu\n", static_cast<size_t>(delaunay.GetNumTriangles()));
        std::vector<size_t> hull;
        delaunay.GetHull(hull);
        std::printf("delaunay2_hull_size %zu\n", hull.size());
    }

    {
        std::vector<gte::Vector<3, double>> f0{ {0, 0, 0}, {1, 2, 3}, {0, 0, 0} };
        std::vector<double> times{ 0, 1, 2 };
        gte::NaturalCubicSpline<3, double> spline(true, f0, times);
        auto knot = spline.GetPosition(1.0);
        auto mid = spline.GetPosition(0.5);
        std::printf("spline_knot %.10g %.10g %.10g\n", knot[0], knot[1], knot[2]);
        std::printf("spline_mid %.10g %.10g %.10g\n", mid[0], mid[1], mid[2]);
    }

    {
        gte::STLBinaryFile<std::array<float, 3>> stl;
        bool ok = stl.Load(argv[1]);
        std::printf("stl_load %d\n", ok ? 1 : 0);
        std::printf("stl_triangles %zu\n", stl.triangles.size());
        if (stl.triangles.size() == 2)
        {
            auto const& t = stl.triangles[1];
            std::printf("stl_tri1_vertex1 %.6g %.6g %.6g\n", t.vertex[1][0], t.vertex[1][1], t.vertex[1][2]);
            std::printf("stl_tri1_attr %u\n", static_cast<unsigned>(t.attributeByteCount));
        }

        char path[] = "/tmp/gte-kat-XXXXXX";
        int fd = mkstemp(path);
        if (fd >= 0)
        {
            close(fd);
            ok = stl.Save(path);
            gte::STLBinaryFile<std::array<float, 3>> again;
            ok = ok && again.Load(path);
            unlink(path);
            std::printf("stl_roundtrip %d %zu\n", ok ? 1 : 0, again.triangles.size());
        }
    }
    return 0;
}
