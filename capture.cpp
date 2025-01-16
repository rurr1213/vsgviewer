#include <vsg/all.h>

//#ifdef vsgXchange_FOUND
#include <vsgXchange/all.h>
//#endif

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

#include "H264NVEncoder.h"
#include "PipeToFFmpeg.h"

#include "capture.h"

PipeToFFmpeg pipeToFFmpeg;
H264NVEncoder h264NVEncoder;

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();


class CaptureStats {
    int numCaptures = 0;
    std::chrono::_V2::steady_clock::time_point startTime;
    std::chrono::_V2::steady_clock::time_point lastStartTime;
    double totalCaptureTimeMSecs = 0;
    double totalRepeatTimeMSecs = -1;
public:
    void start() {
        numCaptures++;
        startTime = vsg::clock::now(); // Get the current time
    }
    void stop() {
        auto currentTime = vsg::clock::now(); // Get the current time
        auto captureTime = std::chrono::duration<double, std::milli>(currentTime - startTime).count(); // Calculate elapsed time
        totalCaptureTimeMSecs += captureTime;
        if (totalRepeatTimeMSecs<0) {
            lastStartTime = startTime;
            totalRepeatTimeMSecs = 0;
        } else {
            auto repeatTime = std::chrono::duration<double, std::milli>(startTime - lastStartTime).count(); // Calculate elapsed time
            totalRepeatTimeMSecs += repeatTime;
            lastStartTime = startTime;
    }
    }

    void report() {
        std::cout << "Performance report"   << std::endl;
        std::cout << "Captures: "  << numCaptures << std::endl;
        std::cout << "Average capture time: " << totalCaptureTimeMSecs / numCaptures << " ms" << std::endl;
        if (numCaptures>0) {
            double repeatTime = totalRepeatTimeMSecs / (numCaptures-1);
            double fps = 1000.0 / repeatTime;
            std::cout << "Average repeat time: " << totalRepeatTimeMSecs / (numCaptures-1) << "ms" << std::endl;
            std::cout << "fps: " << fps << std::endl;
        }
    }
};

CaptureStats captureStats;

Capture::Capture() {
}

Capture::~Capture() {
}

bool Capture::init(int width, int height)
{
    bool status = true;

    nWidth = width;
    nHeight = height;

    NvEncoderInitParam encodeCLIOptions;
    NV_ENC_BUFFER_FORMAT  eFormat = NV_ENC_BUFFER_FORMAT_NV12;

    bool eventDebugTest = false; // or true if you need the debug behavior
    status = status && h264NVEncoder.init(nWidth, nHeight, &encodeCLIOptions, eFormat);
    status = status && pipeToFFmpeg.init(nWidth, nHeight);

    return status;
}

bool Capture::deinit(void) {
    bool status = true;
    status = status && h264NVEncoder.deinit();
    status = status && pipeToFFmpeg.deinit();

    captureStats.report();

    return status;
}

void Capture::hexdump(const void* addr, size_t len) {
    const unsigned char* pc = static_cast<const unsigned char*>(addr);

    size_t display_len = std::min(len, (size_t)16); // Display up to 16 bytes

    for (size_t i = 0; i < display_len; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(pc[i]) << " ";
    }

    std::cout << std::dec << std::endl; // Reset to decimal output and newline
}

vsg::ref_ptr<vsg::ubvec4Array2D> Capture::captureScreenshot(vsg::ref_ptr<vsg::Window> window, int targetWidth, int targetHeight) // Add event and eventDebugTest parameters
{
    // printInfo(window);
    auto width = window->extent2D().width;
    auto height = window->extent2D().height;

    auto device = window->getDevice();
    auto physicalDevice = window->getPhysicalDevice();
    auto swapchain = window->getSwapchain();

    // get the colour buffer image of the previous rendered frame as the current frame hasn't been rendered yet.  The 1 in window->imageIndex(1) means image from 1 frame ago.
    auto sourceImage = window->imageView(window->imageIndex(1))->image;

    VkFormat sourceImageFormat = swapchain->getImageFormat();
    VkFormat targetImageFormat = sourceImageFormat;

    //
    // 1) Check to see if Blit is supported.
    //
    VkFormatProperties srcFormatProperties;
    vkGetPhysicalDeviceFormatProperties(*(physicalDevice), sourceImageFormat, &srcFormatProperties);

    VkFormatProperties destFormatProperties;
    vkGetPhysicalDeviceFormatProperties(*(physicalDevice), VK_FORMAT_R8G8B8A8_UNORM, &destFormatProperties);

    bool supportsBlit = ((srcFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0) &&
                        ((destFormatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0);

    if (supportsBlit)
    {
        // we can automatically convert the image format when blit, so take advantage of it to ensure RGBA
        targetImageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    }

    //
    // 2) create image to write to
    //
    auto destinationImage = vsg::Image::create();
    destinationImage->imageType = VK_IMAGE_TYPE_2D;
    destinationImage->format = targetImageFormat;
    destinationImage->extent.width = targetWidth;  // Use target dimensions
    destinationImage->extent.height = targetHeight; // Use target dimensions
    destinationImage->extent.depth = 1;
    destinationImage->arrayLayers = 1;
    destinationImage->mipLevels = 1;
    destinationImage->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    destinationImage->samples = VK_SAMPLE_COUNT_1_BIT;
    destinationImage->tiling = VK_IMAGE_TILING_LINEAR;
    destinationImage->usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (supportsBlit)
    {
    //Set destinationImage size to targetWidth, targetHeight if we're going to scale while blitting.
        destinationImage->extent.width = targetWidth;
        destinationImage->extent.height = targetHeight;
    }


    destinationImage->compile(device);

    auto deviceMemory = vsg::DeviceMemory::create(device, destinationImage->getMemoryRequirements(device->deviceID), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    destinationImage->bind(deviceMemory, 0);

    //
    // 3) create command buffer and submit to graphics queue
    //
    auto commands = vsg::Commands::create();

    if (event)
    {
        vsg::info("Using vsg::Event/vkEvent");
        commands->addChild(vsg::WaitEvents::create(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, event));
        commands->addChild(vsg::ResetEvent::create(event, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT));
    }

    // 3.a) transition destinationImage to transfer destination initialLayout
    auto transitionDestinationImageToDestinationLayoutBarrier = vsg::ImageMemoryBarrier::create(
        0,                                                             // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,                                  // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                                     // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                          // newLayout
        VK_QUEUE_FAMILY_IGNORED,                                       // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                                       // dstQueueFamilyIndex
        destinationImage,                                              // image
        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} // subresourceRange
    );

    // 3.b) transition swapChainImage from present to transfer source initialLayout
    auto transitionSourceImageToTransferSourceLayoutBarrier = vsg::ImageMemoryBarrier::create(
        VK_ACCESS_MEMORY_READ_BIT,                                     // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,                                   // dstAccessMask
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                               // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                          // newLayout
        VK_QUEUE_FAMILY_IGNORED,                                       // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                                       // dstQueueFamilyIndex
        sourceImage,                                                   // image
        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} // subresourceRange
    );

    auto cmd_transitionForTransferBarrier = vsg::PipelineBarrier::create(
        VK_PIPELINE_STAGE_TRANSFER_BIT,                       // srcStageMask
        VK_PIPELINE_STAGE_TRANSFER_BIT,                       // dstStageMask
        0,                                                    // dependencyFlags
        transitionDestinationImageToDestinationLayoutBarrier, // barrier
        transitionSourceImageToTransferSourceLayoutBarrier    // barrier
    );

    commands->addChild(cmd_transitionForTransferBarrier);

    if (supportsBlit)
    {
        // 3.c.1) if blit using vkCmdBlitImage
        VkImageBlit region{};
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.layerCount = 1;
        region.srcOffsets[0] = VkOffset3D{0, 0, 0};
        region.srcOffsets[1] = VkOffset3D{static_cast<int32_t>(width), static_cast<int32_t>(height), 1};
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.layerCount = 1;
        region.dstOffsets[0] = {0, 0, 0};
        region.dstOffsets[1] = {static_cast<int32_t>(targetWidth), static_cast<int32_t>(targetHeight), 1}; // Target dimensions

        auto blitImage = vsg::BlitImage::create();
        blitImage->srcImage = sourceImage;
        blitImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitImage->dstImage = destinationImage;
        blitImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    // *** SCALING HAPPENS HERE ***
        // Destination offsets and extent for scaling:
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.layerCount = 1;
        region.dstOffsets[0] = VkOffset3D{0, 0, 0};
        region.dstOffsets[1] = VkOffset3D{static_cast<int32_t>(targetWidth), static_cast<int32_t>(targetHeight), 1}; // Target dimensions

        blitImage->regions.push_back(region);
        blitImage->filter = VK_FILTER_LINEAR;

        commands->addChild(blitImage);
    }
    else
    {
        // 3.c.2) else use vkCmdCopyImage

        VkImageCopy region{};
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.layerCount = 1;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.layerCount = 1;
        region.extent.width = width;
        region.extent.height = height;
        region.extent.depth = 1;

        auto copyImage = vsg::CopyImage::create();
        copyImage->srcImage = sourceImage;
        copyImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        copyImage->dstImage = destinationImage;
        copyImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copyImage->regions.push_back(region);

        commands->addChild(copyImage);
    }

    // 3.d) transition destination image from transfer destination layout to general layout to enable mapping to image DeviceMemory
    auto transitionDestinationImageToMemoryReadBarrier = vsg::ImageMemoryBarrier::create(
        VK_ACCESS_TRANSFER_WRITE_BIT,                                  // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                                     // dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                          // oldLayout
        VK_IMAGE_LAYOUT_GENERAL,                                       // newLayout
        VK_QUEUE_FAMILY_IGNORED,                                       // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                                       // dstQueueFamilyIndex
        destinationImage,                                              // image
        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} // subresourceRange
    );

    // 3.e) transition swap chain image back to present
    auto transitionSourceImageBackToPresentBarrier = vsg::ImageMemoryBarrier::create(
        VK_ACCESS_TRANSFER_READ_BIT,                                   // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                                     // dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                          // oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                               // newLayout
        VK_QUEUE_FAMILY_IGNORED,                                       // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                                       // dstQueueFamilyIndex
        sourceImage,                                                   // image
        VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} // subresourceRange
    );

    auto cmd_transitionFromTransferBarrier = vsg::PipelineBarrier::create(
        VK_PIPELINE_STAGE_TRANSFER_BIT,                // srcStageMask
        VK_PIPELINE_STAGE_TRANSFER_BIT,                // dstStageMask
        0,                                             // dependencyFlags
        transitionDestinationImageToMemoryReadBarrier, // barrier
        transitionSourceImageBackToPresentBarrier      // barrier
    );

    commands->addChild(cmd_transitionFromTransferBarrier);

    auto fence = vsg::Fence::create(device);
    auto queueFamilyIndex = physicalDevice->getQueueFamily(VK_QUEUE_GRAPHICS_BIT);
    auto commandPool = vsg::CommandPool::create(device, queueFamilyIndex);
    auto queue = device->getQueue(queueFamilyIndex);

    vsg::submitCommandsToQueue(commandPool, fence, 100000000000, queue, [&](vsg::CommandBuffer& commandBuffer) {
        commands->record(commandBuffer);
    });

    //
    // 4) Create the final imageData as a ubvec4Array2D
    //
    auto imageData = vsg::ubvec4Array2D::create(targetWidth, targetHeight, vsg::Data::Properties{targetImageFormat});

void* mappedData;
    VkResult result = vkMapMemory(*device, *deviceMemory, 0, destinationImage->getMemoryRequirements(device->deviceID).size, 0, &mappedData);
    if (result != VK_SUCCESS) {
        throw vsg::Exception{"Failed to map memory for screenshot.", result};
    }

    VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
    VkSubresourceLayout subResourceLayout;

    vkGetImageSubresourceLayout(*device, destinationImage->vk(device->deviceID), &subResource, &subResourceLayout);

    size_t destRowWidth = targetWidth * sizeof(vsg::ubvec4); // Use targetWidth for correct size
    if (destRowWidth == subResourceLayout.rowPitch)
    {
        memcpy(imageData->dataPointer(), mappedData, imageData->dataSize());
    }
    else
    {
        for (uint32_t row = 0; row < targetHeight; ++row)
        {
            memcpy(imageData->dataPointer(row * targetWidth), static_cast<uint8_t*>(mappedData) + row * subResourceLayout.rowPitch, destRowWidth);
        }
    }

    deviceMemory->unmap();

    return vsg::ref_ptr<vsg::ubvec4Array2D>(imageData); // Return a ref_ptr
}


void Capture::captureAndSave(vsg::ref_ptr<vsg::Window> window)
{
    static int times = 0;
    times++;

    if ((times%2)==0) {
        /* *********** ************************************************
        this will scale the image to the target width! and distort the axpect ratio, but good for transmission.
        Adjust source image to have the correct aspect ratio as well. */

        int nCaptured = 1;
        captureStats.start();
        if (auto imageData = captureScreenshot(window, nWidth, nHeight))
        {
            // Determine correct initial size. Scaling will occur after this.

            std::vector<uint8_t> nv12Data;

            converter.rgbaToNv12(reinterpret_cast<const uint8_t*>(imageData->data()), nWidth, nHeight, nv12Data);  // Convert to NV12

            std::vector<std::vector<uint8_t>> encodedPackets;
            int numPackets = h264NVEncoder.encode(nv12Data, nv12Data.size(), encodedPackets); // Use nv12Data, correct size, and store packets

            for (auto& packet : encodedPackets) {
                pipeToFFmpeg.encodeAndStream(packet);
            }

        } else {
            std::cout << "Failed to capture screenshot." << std::endl;
        }
        captureStats.stop();
    }
}

bool Capture::writeToFile(vsg::ref_ptr<vsg::ubvec4Array2D> image, std::string filePathAndName) {

    // Writing to PNG
    vsg::Path filename = filePathAndName;

    if (vsg::write(image, filename))
    {
        std::cout << "Screenshot saved to " << filename << std::endl;
        return true;
    }
    else
    {
        std::cout << "Failed to save screenshot." << std::endl;
        return false;
    }
}

