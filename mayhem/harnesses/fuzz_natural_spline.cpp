// Fuzzes gte::NaturalCubicSpline / gte::NaturalQuinticSpline in 3D. The first input byte picks the
// variant (free/closed/clamped cubic, free/closed quintic); the rest is a sequence of records
// (time step byte, 3 x int16 position, and for the clamped cubic and quintic variants a 3 x int16
// first derivative, of which the clamped cubic uses the first and last). The time
// steps are accumulated as 1 + byte, so times are strictly increasing as the class requires.
#include <Mathematics/NaturalCubicSpline.h>
#include <Mathematics/NaturalQuinticSpline.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace
{
    gte::Vector<3, double> ReadVector3(uint8_t const* p)
    {
        int16_t v[3];
        std::memcpy(v, p, sizeof(v));
        return gte::Vector<3, double>{ static_cast<double>(v[0]), static_cast<double>(v[1]), static_cast<double>(v[2]) };
    }

    void Probe(gte::ParametricCurve<3, double> const& curve)
    {
        double const tmin = curve.GetTMin(), tmax = curve.GetTMax();
        for (int i = 0; i <= 4; ++i)
        {
            double t = tmin + (tmax - tmin) * i / 4.0;
            std::array<gte::Vector<3, double>, 4> jet{};
            curve.Evaluate(t, 3, jet.data());
            volatile double v = jet[0][0] + jet[1][1] + jet[2][2] + jet[3][0];
            (void)v;
        }
    }
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
    if (size < 1)
    {
        return -1;
    }

    unsigned const variant = data[0] % 5;
    bool const wide = (variant >= 2);
    size_t const recordSize = wide ? 13 : 7;
    ++data;
    --size;

    size_t const numRecords = size / recordSize;
    if (numRecords == 0)
    {
        return -1;
    }

    std::vector<gte::Vector<3, double>> f0(numRecords), f1(numRecords);
    std::vector<double> times(numRecords);
    double t = 0.0;
    for (size_t i = 0; i < numRecords; ++i)
    {
        uint8_t const* rec = data + i * recordSize;
        t += 1.0 + static_cast<double>(rec[0]);
        times[i] = t;
        f0[i] = ReadVector3(rec + 1);
        if (wide)
        {
            f1[i] = ReadVector3(rec + 7);
        }
    }

    try
    {
        int32_t const n = static_cast<int32_t>(numRecords);
        switch (variant)
        {
        case 0:
        {
            gte::NaturalCubicSpline<3, double> spline(true, n, f0.data(), times.data());
            Probe(spline);
            break;
        }
        case 1:
        {
            gte::NaturalCubicSpline<3, double> spline(false, n, f0.data(), times.data());
            Probe(spline);
            break;
        }
        case 2:
        {
            gte::NaturalCubicSpline<3, double> spline(n, f0.data(), times.data(), f1[0], f1[numRecords - 1]);
            Probe(spline);
            break;
        }
        case 3:
        {
            gte::NaturalQuinticSpline<3, double> spline(true, n, f0.data(), f1.data(), times.data());
            Probe(spline);
            break;
        }
        default:
        {
            gte::NaturalQuinticSpline<3, double> spline(false, n, f0.data(), f1.data(), times.data());
            Probe(spline);
            break;
        }
        }
    }
    catch (std::runtime_error const&)
    {
        return -1;
    }
    return 0;
}
