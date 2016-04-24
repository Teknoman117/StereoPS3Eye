#include <iostream>
#include "stereo.hpp"

using namespace std;
using namespace std::chrono;

#define max(a,b) (a > b ? a : b)

StereoCamera::StereoCamera(uint32_t width, uint32_t height, uint32_t fps)
    : frameL(NULL), frameR(NULL)
{
    cameraL = make_unique<Camera>("/dev/video0", width, height, fps);
    cameraR = make_unique<Camera>("/dev/video1", width, height, fps);
    interval = 1.f / static_cast<float>(fps);

    callback = [] (const Camera::Buffer&, const Camera::Buffer&, float) {};
}

void StereoCamera::SetCallback(std::function<void (const Camera::Buffer&,const Camera::Buffer&,float)>&& c)
{
    this->callback = std::move(c);
}


void StereoCamera::Loop()
{
    fd_set fds;        
    int r;

    // Fetch an image from the cameras
    do 
    {
        FD_ZERO(&fds);
        FD_SET(cameraL->GetDescriptor(), &fds);
        FD_SET(cameraR->GetDescriptor(), &fds);

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        r = select(max(cameraL->GetDescriptor(), cameraR->GetDescriptor()) + 1, &fds, NULL, NULL, &tv);
    } 
    while ((r == -1 && (errno = EINTR)));
    
    if (r == -1) 
    {
        std::cerr << "Error occurred in select" << std::endl;
        exit(errno);
    }

    // Figure out which image we received
    if(FD_ISSET(cameraL->GetDescriptor(), &fds))
    {
        frameLTime = high_resolution_clock::now();
        duration<float> delta = duration_cast<duration<float>>(frameLTime - previousTime);
        previousTime = frameLTime;

        std::cout << "frameL delta = " << delta.count() << std::endl;

        if(frameL)
            cameraL->Release(*frameL);
        frameL = &(cameraL->Capture());

        // Check if we should issue the frame pair
        if(delta.count() <= interval/2.f && frameL && frameR)
        {
            callback(*frameL, *frameR, duration_cast<duration<float>>(frameLTime - frameRTime).count());
        }
    }

    if(FD_ISSET(cameraR->GetDescriptor(), &fds))
    {   
        frameRTime = high_resolution_clock::now();
        duration<float> delta = duration_cast<duration<float>>(frameRTime - previousTime);
        previousTime = frameRTime;

        std::cout << "frameR delta = " << delta.count() << std::endl;

        if(frameR)
            cameraR->Release(*frameR);
        frameR = &(cameraR->Capture());

        // Check if we should issue the frame pair
        if(delta.count() <= interval/2.f && frameL && frameR)
        {
            callback(*frameL, *frameR, duration_cast<duration<float>>(frameRTime - frameLTime).count());
        }
    }
}
