#ifndef __STEREO_HPP__
#define __STEREO_HPP__

#include <chrono>
#include <memory>
#include <functional>

#include "camera.hpp"

extern "C"
{
    #include <sys/select.h>
    #include <sys/time.h>
}

class StereoCamera
{
    std::unique_ptr<Camera>     cameraL;
    std::unique_ptr<Camera>     cameraR;
    float                       interval;

    const Camera::Buffer*       frameL;
    const Camera::Buffer*       frameR;

    std::chrono::high_resolution_clock::time_point frameLTime;
    std::chrono::high_resolution_clock::time_point frameRTime;
    std::chrono::high_resolution_clock::time_point previousTime;

    std::function<void (const Camera::Buffer&,const Camera::Buffer&,float)> callback;

public:
    StereoCamera(uint32_t width, uint32_t height, uint32_t fps);

    void SetCallback(std::function<void (const Camera::Buffer&,const Camera::Buffer&,float)>&& c);
    void Loop();
};

#endif
