#include "NvEncoder/NvEncoderCuda.h"
#include "../Utils/Logger.h"
#include "../Utils/NvEncoderCLIOptions.h"
#include "../Utils/NvCodecUtils.h"

class H264NVEncoder
{
    int nFrameSize;
    //NvEncoderCuda& enc;
    // std::unique_ptr<uint8_t[]> pHostFrame;
    std::unique_ptr<NvEncoderCuda> penc;
    const NvEncInputFrame* encoderInputFrame = nullptr;  // Store the input frame pointer
    NV_ENC_PIC_PARAMS picParams = {}; // Important: Initialize picParams

public:
    CUcontext cuContext = NULL;

    H264NVEncoder() {

    }

    ~H264NVEncoder()
    {
        deinit();
    }

    bool init(int nWidth, int nHeight, NvEncoderInitParam *pEncodeCLIOptions = nullptr,
        NV_ENC_BUFFER_FORMAT eFormat = NV_ENC_BUFFER_FORMAT_IYUV) {

//        NV_ENC_BUFFER_FORMAT eFormat = NV_ENC_BUFFER_FORMAT_IYUV;

        // -----------------------------------------------------------------------
        int iGpu = 0;
        ck(cuInit(0));
        int nGpu = 0;
        ck(cuDeviceGetCount(&nGpu));
        if (iGpu < 0 || iGpu >= nGpu) {
            std::cout << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]" << std::endl;
            return false;
        }
        std::cout << "GPUs [" << nGpu << "]" << std::endl;
        std::cout << "Using GPU [" << iGpu << "]" << std::endl;

        CUdevice cuDevice = 0;
        ck(cuDeviceGet(&cuDevice, iGpu));
        char szDeviceName[80];
        ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
        std::cout << "GPU in use: " << szDeviceName << std::endl;
        ck(cuCtxCreate(&cuContext, 0, cuDevice));

        // -----------------------------------------------------------------------
        penc = std::make_unique<NvEncoderCuda>(cuContext, nWidth, nHeight, eFormat, 0, false);

        NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
        NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };

        initializeParams.encodeConfig = &encodeConfig;


        penc->CreateDefaultEncoderParams(&initializeParams, NV_ENC_CODEC_H264_GUID, NV_ENC_PRESET_HQ_GUID);

        initializeParams.encodeConfig->gopLength = 1;                      // Set GOP length to 1
        initializeParams.encodeConfig->frameIntervalP = 1;

        initializeParams.encodeConfig->rcParams.vbvBufferSize = 0;   //Or some small value

        picParams.inputWidth = penc->GetEncodeWidth();
        picParams.inputHeight = penc->GetEncodeHeight();

//        pEncodeCLIOptions->SetInitParams(&initializeParams, eFormat);
        penc->CreateEncoder(&initializeParams);
        encoderInputFrame = penc->GetNextInputFrame();

        std::vector<uint8_t> seqParams;
        penc->GetSequenceParams(seqParams);
        picParams.version = NV_ENC_PIC_PARAMS_VER;   // Crucial! Must be after encoder creation!

        nFrameSize = penc->GetFrameSize();

        std::cout << "H264NVEncoder: initialized" << std::endl;

        return true;
    }

    int encode(std::unique_ptr<uint8_t[]>& pHostFrame, int nsize, std::vector<std::vector<uint8_t>> &vPacket) {
        vPacket.clear();

        int nRead = nsize;
        if (nRead == nFrameSize) {

            encoderInputFrame = penc->GetNextInputFrame();
            NvEncoderCuda::CopyToDeviceFrame(cuContext, pHostFrame.get(), 0, (CUdeviceptr)encoderInputFrame->inputPtr,
                (int)encoderInputFrame->pitch,
                penc->GetEncodeWidth(),
                penc->GetEncodeHeight(),
                CU_MEMORYTYPE_HOST,
                encoderInputFrame->bufferFormat,
                encoderInputFrame->chromaOffsets,
                encoderInputFrame->numChromaPlanes);

            picParams.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR; //Set flag
            picParams.inputBuffer = encoderInputFrame->inputPtr;
            penc->EncodeFrame(vPacket, &picParams);
        } else {
            penc->EndEncode(vPacket);
        }
        return vPacket.size();
    }

    int getFrameSize() {return penc->GetFrameSize();}

    bool deinit() {
        if (!penc) return false;
        penc->DestroyEncoder();
        penc = 0;

        std::cout << "H264NVEncoder: deinitialized" << std::endl;

        return true;
    }
};
