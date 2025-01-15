#include <vsg/all.h>

//#ifdef vsgXchange_FOUND
//#include <vsgXchange/all.h>
//#endif

#include <cstdio>
#include <cstdlib>
//#include <algorithm>
//#include <chrono>
//#include <iostream>
//#include <thread>

//#include "H264NVEncoder.h"
//#include "PipeToFFmpeg.h"

const int DEFAULT_SCREEN_WIDTH = 1920;
const int DEFAULT_SCREEN_HEIGHT = 1080;

class Capture {

    int nWidth = 0;  // or whatever dimensions you need
    int nHeight = 0; // or whatever dimensions you need

    vsg::ref_ptr<vsg::Event> event; //  Make sure this is declared if the screenshot function uses it.

    void hexdump(const void* addr, size_t len);
    vsg::ref_ptr<vsg::ubvec4Array2D> captureScreenshot(vsg::ref_ptr<vsg::Window> window, vsg::ref_ptr<vsg::Options> options, vsg::ref_ptr<vsg::Event> event, int targetWidth, int targetHeight, bool eventDebugTest = false);
    void rgbaToNv12(const uint8_t* rgbaData, int width, int height, std::vector<uint8_t>& nv12Data);
    void nv12ToRgba(const uint8_t* nv12Data, int width, int height, std::vector<uint8_t>& rgbaData);

    public:

    Capture(int width = DEFAULT_SCREEN_WIDTH, int height = DEFAULT_SCREEN_HEIGHT);

    void captureAndSave(vsg::ref_ptr<vsg::Window> window, vsg::ref_ptr<vsg::Options> _options);

    bool init(void);
    bool deinit(void);
};
