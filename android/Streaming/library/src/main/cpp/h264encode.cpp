//
// Created by wlanjie on 2017/6/4.
//

#include <wels/codec_app_def.h>
#include <string.h>
#include <cstdint>
#include "libyuv.h"
#include "h264encode.h"
#include "log.h"

extern void logEncode(void *context, int level, const char *message);

void logEncode(void *context, int level, const char *message) {
    LOGD("%s", message);
}

wlanjie::H264Encoder::H264Encoder() {

}

wlanjie::H264Encoder::~H264Encoder() {

}

void wlanjie::H264Encoder::setVideoParameter(wlanjie::VideoParameter videoParameter) {
    this->parameter = videoParameter;
}


bool wlanjie::H264Encoder::openH264Encoder() {
    LOGD("h264 encode open");
    WelsCreateSVCEncoder(&encoder_);
    SEncParamExt encoder_params = createEncoderParams();
    int ret = 0;
    if ((ret = encoder_->InitializeExt(&encoder_params)) != 0) {
        LOGE("initial h264 error = %d", ret);
        return false;
    }

    int level = WELS_LOG_DETAIL;
    encoder_->SetOption(ENCODER_OPTION_TRACE_LEVEL, &level);
    void (*func)(void *, int, const char *) = &logEncode;
    encoder_->SetOption(ENCODER_OPTION_TRACE_CALLBACK, &func);
    int video_format = videoFormatI420;
    encoder_->SetOption(ENCODER_OPTION_DATAFORMAT, &video_format);

    memset(&_sourcePicture, 0, sizeof(Source_Picture_s));
    uint8_t *data_ = NULL;
    _sourcePicture.iPicWidth = parameter.frameWidth;
    _sourcePicture.iPicHeight = parameter.frameHeight;
    _sourcePicture.iStride[0] = parameter.frameWidth;
    _sourcePicture.iStride[1] = _sourcePicture.iStride[2] = parameter.frameWidth >> 1;
    data_ = static_cast<uint8_t  *> (realloc(data_, parameter.frameWidth * parameter.frameHeight * 3 / 2));
    _sourcePicture.pData[0] = data_;
    _sourcePicture.pData[1] = _sourcePicture.pData[0] + parameter.frameWidth * parameter.frameHeight;
    _sourcePicture.pData[2] = _sourcePicture.pData[1] + (parameter.frameWidth * parameter.frameHeight >> 2);
    _sourcePicture.iColorFormat = videoFormatI420;

    memset(&_scaleSourcePicture, 0, sizeof(Source_Picture_s));
    uint8_t *scale_data_ = NULL;
    _scaleSourcePicture.iPicWidth = parameter.videoWidth;
    _scaleSourcePicture.iPicHeight = parameter.videoHeight;
    _scaleSourcePicture.iStride[0] = parameter.videoWidth;
    _scaleSourcePicture.iStride[1] = _scaleSourcePicture.iStride[2] = parameter.videoWidth >> 1;
    scale_data_ = static_cast<uint8_t  *> (realloc(scale_data_, parameter.videoWidth * parameter.videoHeight * 3 / 2));
    _scaleSourcePicture.pData[0] = scale_data_;
    _scaleSourcePicture.pData[1] = _scaleSourcePicture.pData[0] + parameter.videoWidth * parameter.videoHeight;
    _scaleSourcePicture.pData[2] = _scaleSourcePicture.pData[1] + (parameter.videoWidth * parameter.videoHeight >> 2);
    _scaleSourcePicture.iColorFormat = videoFormatI420;

    memset(&_rotationSourcePicture, 0, sizeof(Source_Picture_s));
    uint8_t *rotation_data_ = NULL;
    _rotationSourcePicture.iPicWidth = parameter.frameWidth;
    _rotationSourcePicture.iPicHeight = parameter.frameHeight;
    _rotationSourcePicture.iStride[0] = parameter.frameWidth;
    _rotationSourcePicture.iStride[1] = _rotationSourcePicture.iStride[2] = parameter.frameWidth >> 1;
    rotation_data_ = static_cast<uint8_t  *> (realloc(data_, parameter.frameWidth * parameter.frameHeight * 3 / 2));
    _rotationSourcePicture.pData[0] = rotation_data_;
    _rotationSourcePicture.pData[1] = _rotationSourcePicture.pData[0] + parameter.frameWidth * parameter.frameHeight;
    _rotationSourcePicture.pData[2] = _rotationSourcePicture.pData[1] + (parameter.frameWidth * parameter.frameHeight >> 2);
    _rotationSourcePicture.iColorFormat = videoFormatI420;

    memset(&info, 0, sizeof(SFrameBSInfo));
    _outputStream.open("/sdcard/wlanjie.h264", std::ios_base::binary | std::ios_base::out);
    return false;
}

void wlanjie::H264Encoder::closeH264Encoder() {
    if (encoder_) {
        encoder_->Uninitialize();
        WelsDestroySVCEncoder(encoder_);
        encoder_ = nullptr;
    }
    _outputStream.close();
}

SEncParamExt wlanjie::H264Encoder::createEncoderParams() const {
    SEncParamExt encoder_params;
    encoder_->GetDefaultParams(&encoder_params);
    encoder_params.iUsageType = CAMERA_VIDEO_REAL_TIME;
    encoder_params.iPicWidth = parameter.videoWidth;
    encoder_params.iPicHeight = parameter.videoHeight;
    // uses bit/s kbit/s
    encoder_params.iTargetBitrate = parameter.bitrate * 1000;
    // max bit/s
    encoder_params.iMaxBitrate = parameter.bitrate * 1000;
    encoder_params.iRCMode = RC_OFF_MODE;
    encoder_params.fMaxFrameRate = parameter.frameRate;

    encoder_params.bEnableFrameSkip = true;
    encoder_params.bEnableDenoise = false;
    encoder_params.bEnableLongTermReference = false;
    encoder_params.bEnableSceneChangeDetect = true;
    encoder_params.bPrefixNalAddingCtrl = false;

    encoder_params.uiIntraPeriod = 60;
    encoder_params.uiMaxNalSize = 0;
    encoder_params.iTemporalLayerNum = 1;
    encoder_params.iSpatialLayerNum = 1;
    encoder_params.iMultipleThreadIdc = 1;

    encoder_params.sSpatialLayers[0].iVideoWidth = parameter.videoWidth;
    encoder_params.sSpatialLayers[0].iVideoHeight = parameter.videoHeight;
    encoder_params.sSpatialLayers[0].fFrameRate = parameter.frameRate;
    encoder_params.sSpatialLayers[0].iSpatialBitrate = parameter.bitrate * 1000;
    encoder_params.sSpatialLayers[0].iMaxSpatialBitrate = 0;
//    encoder_params.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_AUTO_SLICE;
    encoder_params.eSpsPpsIdStrategy = CONSTANT_ID;
    LOGE("frameWidth = %d frameHeight = %d videoWidth = %d videoHeight = %d frameRate = %d bitrate = %d",
         parameter.frameWidth, parameter.frameHeight, parameter.videoWidth, parameter.videoHeight, parameter.frameRate, parameter.bitrate);
    return encoder_params;
}

void wlanjie::H264Encoder::encoder(char *rgba, long pts, int *h264_length, uint8_t **h264) {
    if (encoder_ == NULL) {
        return;
    }
    _sourcePicture.uiTimeStamp = pts;
    libyuv::RGBAToI420((const uint8 *) rgba, parameter.frameWidth * 4,
                   _sourcePicture.pData[0], _sourcePicture.iStride[0],
                   _sourcePicture.pData[1], _sourcePicture.iStride[1],
                   _sourcePicture.pData[2], _sourcePicture.iStride[2],
                   parameter.frameWidth, parameter.frameHeight);
    libyuv::I420Rotate(
            _sourcePicture.pData[0], _sourcePicture.iStride[0],
            _sourcePicture.pData[1], _sourcePicture.iStride[1],
            _sourcePicture.pData[2], _sourcePicture.iStride[2],
            _rotationSourcePicture.pData[0], _rotationSourcePicture.iStride[0],
            _rotationSourcePicture.pData[1], _rotationSourcePicture.iStride[1],
            _rotationSourcePicture.pData[2], _rotationSourcePicture.iStride[2],
            parameter.frameWidth, parameter.frameHeight,
            libyuv::RotationMode::kRotate180
    );
    if (parameter.videoWidth != parameter.frameWidth || parameter.videoHeight != parameter.frameHeight) {
        libyuv::I420Scale(
                _rotationSourcePicture.pData[0], _rotationSourcePicture.iStride[0],
                _rotationSourcePicture.pData[1], _rotationSourcePicture.iStride[1],
                _rotationSourcePicture.pData[2], _rotationSourcePicture.iStride[2],
                parameter.frameWidth, parameter.frameHeight,
                _scaleSourcePicture.pData[0], _scaleSourcePicture.iStride[0],
                _scaleSourcePicture.pData[1], _scaleSourcePicture.iStride[1],
                _scaleSourcePicture.pData[2], _scaleSourcePicture.iStride[2],
                parameter.videoWidth, parameter.videoHeight,
                libyuv::FilterMode::kFilterNone
        );
    }

    int ret = encoder_->EncodeFrame(parameter.videoWidth != parameter.frameWidth || parameter.videoHeight != parameter.frameHeight ? &_scaleSourcePicture : &_sourcePicture, &info);
    if (!ret) {
        if (info.eFrameType != videoFrameTypeSkip) {
            int len = 0;
            for (int layer = 0; layer < info.iLayerNum; ++layer) {
                const SLayerBSInfo &layerInfo = info.sLayerInfo[layer];
                for (int nal = 0; nal < layerInfo.iNalCount; ++nal) {
                    len += layerInfo.pNalLengthInByte[nal];
                }
            }
            uint8_t *encoded_image_buffer = new uint8_t[len];
            *h264_length = len;
            int image_length = 0;
            for (int layer = 0; layer < info.iLayerNum; ++layer) {
                SLayerBSInfo layerInfo = info.sLayerInfo[layer];
                int layerSize = 0;
                for (int nal = 0; nal < layerInfo.iNalCount; ++nal) {
                    layerSize += layerInfo.pNalLengthInByte[nal];
                }
                _outputStream.write((const char *) layerInfo.pBsBuf, layerSize);
                memcpy(encoded_image_buffer + image_length, layerInfo.pBsBuf, layerSize);
                image_length += layerSize;
            }
            *h264 = encoded_image_buffer;
        }
    }
}