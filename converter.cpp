#include <vsg/all.h>
#include <cstdio>
#include <cstdlib>
#include "converter.h"

Converter::Converter() {
    // Initialize lookup tables
    for (int i = 0; i < 256; ++i) {
        yLookup[i] = static_cast<uint8_t>(0.2126f * i);
        uLookup[i] = static_cast<int>(-0.0999f * i);
        vLookup[i] = static_cast<int>(0.6150f * i);
    }
}

void Converter::rgbaToNv12_fast(const uint8_t* rgbaData, int width, int height, std::vector<uint8_t>& nv12Data)
{
    nv12Data.resize(width * height * 3 / 2); // Allocate memory for NV12

    uint8_t* yPlane = nv12Data.data();
    uint8_t* uvPlane = yPlane + width * height;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int rgbaIndex = (y * width + x) * 4;
            uint8_t r = rgbaData[rgbaIndex];
            uint8_t g = rgbaData[rgbaIndex + 1];
            uint8_t b = rgbaData[rgbaIndex + 2];

            // YUV conversion using lookup tables and integer arithmetic
            int yVal = yLookup[r] + yLookup[g] + yLookup[b];
            yPlane[y * width + x] = static_cast<uint8_t>(std::clamp(yVal, 0, 255));


            if (x % 2 == 0 && y % 2 == 0)
            {
                int uVal = uLookup[r] + uLookup[g] + uLookup[b];
                int vVal = vLookup[r] + vLookup[g] + vLookup[b];

                uvPlane[(y / 2) * width + x] = static_cast<uint8_t>(std::clamp(uVal + 128, 0, 255));
                uvPlane[(y / 2) * width + x + 1] = static_cast<uint8_t>(std::clamp(vVal + 128, 0, 255));

            }
        }
    }
}

void Converter::rgbaToNv12(const uint8_t* rgbaData, int width, int height, std::vector<uint8_t>& nv12Data)
{
    nv12Data.resize(width * height * 3 / 2); // Allocate memory for NV12

    uint8_t* yPlane = nv12Data.data();
    uint8_t* uvPlane = yPlane + width * height;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int rgbaIndex = (y * width + x) * 4;
            uint8_t r = rgbaData[rgbaIndex];
            uint8_t g = rgbaData[rgbaIndex + 1];
            uint8_t b = rgbaData[rgbaIndex + 2];

            // YUV conversion (BT.709)
            int yVal = 0.2126f * r + 0.7152f * g + 0.0722f * b;
            yPlane[y * width + x] = static_cast<uint8_t>(std::clamp(yVal, 0, 255));

            // Chroma subsampling (average of 2x2 block) - only for even coordinates
            if (x % 2 == 0 && y % 2 == 0)
            {
                int uVal = -0.0999f * r - 0.3360f * g + 0.4360f * b;
                int vVal = 0.6150f * r - 0.5586f * g - 0.0563f * b;


                uvPlane[(y / 2) * width + x] = static_cast<uint8_t>(std::clamp(uVal+128, 0, 255)); // U
                uvPlane[(y / 2) * width + x + 1] = static_cast<uint8_t>(std::clamp(vVal+128, 0, 255)); // V

            }
        }
    }
}

void Converter::nv12ToRgba(const uint8_t* nv12Data, int width, int height, std::vector<uint8_t>& rgbaData) {
    rgbaData.resize(width * height * 4); // Allocate space for RGBA data

    const uint8_t* yPlane = nv12Data;
    const uint8_t* uvPlane = yPlane + width * height;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int yIndex = y * width + x;
            int uvIndex = (y / 2) * width + (x / 2) * 2; // Adjust for interleaved UV
            int rgbaIndex = (y * width + x) * 4;


            // Correctly extract U and V values
            uint8_t u = uvPlane[uvIndex];
            uint8_t v = uvPlane[uvIndex + 1];
            uint8_t yy = yPlane[yIndex];



            // YUV to RGB conversion (BT.709)
            int c = yy - 16;
            int d = u - 128;
            int e = v - 128;

            int r = (298 * c + 409 * e + 128) >> 8;
            int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
            int b = (298 * c + 516 * d + 128) >> 8;



            // Clamp and assign RGB values
            rgbaData[rgbaIndex] = static_cast<uint8_t>(std::clamp(r, 0, 255));
            rgbaData[rgbaIndex + 1] = static_cast<uint8_t>(std::clamp(g, 0, 255));
            rgbaData[rgbaIndex + 2] = static_cast<uint8_t>(std::clamp(b, 0, 255));
            rgbaData[rgbaIndex + 3] = 255; // Alpha (fully opaque)
        }
    }
}



