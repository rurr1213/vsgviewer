#include <vsg/all.h>
#include <cstdio>
#include <cstdlib>
#include "converter.h"

const int DEFAULT_SCREEN_WIDTH = 1920;
const int DEFAULT_SCREEN_HEIGHT = 1080;

class Capture {

    int nWidth = 0;  // or whatever dimensions you need
    int nHeight = 0; // or whatever dimensions you need

    vsg::ref_ptr<vsg::Event> event; //  Make sure this is declared if the screenshot function uses it.
    Converter converter;

    void hexdump(const void* addr, size_t len);
    vsg::ref_ptr<vsg::ubvec4Array2D> captureScreenshot(vsg::ref_ptr<vsg::Window> window, int targetWidth, int targetHeight);
    void rgbaToNv12(const uint8_t* rgbaData, int width, int height, std::vector<uint8_t>& nv12Data);
    void nv12ToRgba(const uint8_t* nv12Data, int width, int height, std::vector<uint8_t>& rgbaData);
    bool writeToFile(vsg::ref_ptr<vsg::ubvec4Array2D> image, std::string filePathAndName);

    public:

        Capture();
        ~Capture();

        void captureAndSave(vsg::ref_ptr<vsg::Window> window);

        bool init(int width = DEFAULT_SCREEN_WIDTH, int height = DEFAULT_SCREEN_HEIGHT);
        bool deinit(void);
};
