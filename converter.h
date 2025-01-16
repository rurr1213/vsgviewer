#include <vsg/all.h>
#include <cstdio>
#include <cstdlib>

class Converter {
private:
    std::array<uint8_t, 256> yLookup;
    std::array<int, 256> uLookup;
    std::array<int, 256> vLookup;

public:
    Converter();
    void nv12ToRgba(const uint8_t* nv12Data, int width, int height, std::vector<uint8_t>& rgbaData);
    void rgbaToNv12(const uint8_t* rgbaData, int width, int height, std::vector<uint8_t>& nv12Data);
};

