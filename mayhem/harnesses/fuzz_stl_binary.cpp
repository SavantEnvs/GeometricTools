// Fuzzes gte::STLBinaryFile::Load, the binary STL reader, and the Save path that writes back what Load
// accepted. Both only take a file name, so each input is written to a private scratch file (TMPDIR +
// mkstemp) that is removed before returning. When Load accepts the input, the object is saved over the
// scratch file and loaded again: Save's own contract ("populate header and triangles, Save writes them")
// means the second Load must succeed and give back the same header and triangles, so any difference aborts.
#include <Mathematics/STLBinaryFile.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

using STL = gte::STLBinaryFile<std::array<float, 3>>;

static bool SameBytes(STL const& a, STL const& b)
{
    if (a.header != b.header || a.triangles.size() != b.triangles.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.triangles.size(); ++i)
    {
        auto const& ta = a.triangles[i];
        auto const& tb = b.triangles[i];
        if (std::memcmp(&ta.normal, &tb.normal, sizeof(ta.normal)) != 0 ||
            std::memcmp(ta.vertex.data(), tb.vertex.data(), sizeof(ta.vertex)) != 0 ||
            ta.attributeByteCount != tb.attributeByteCount)
        {
            return false;
        }
    }
    return true;
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
    char const* dir = std::getenv("TMPDIR");
    std::string path = std::string((dir != nullptr && *dir != '\0') ? dir : "/tmp") + "/stl-XXXXXX";
    int fd = mkstemp(&path[0]);
    if (fd < 0)
    {
        return 0;
    }

    size_t written = 0;
    while (written < size)
    {
        ssize_t n = write(fd, data + written, size - written);
        if (n <= 0)
        {
            break;
        }
        written += static_cast<size_t>(n);
    }
    close(fd);

    STL stl;
    bool loaded = stl.Load(path);
    bool roundTripOk = true;
    if (loaded)
    {
        STL again;
        roundTripOk = stl.Save(path) && again.Load(path) && SameBytes(stl, again);
    }
    unlink(path.c_str());

    if (!roundTripOk)
    {
        std::abort();
    }
    return loaded ? 0 : -1;
}
