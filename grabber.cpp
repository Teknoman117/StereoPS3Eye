#include <iostream>
#include <chrono>
#include <cstdio>

#include "camera.hpp"

extern "C"
{
    #include <sys/select.h>
    #include <sys/time.h>
}

#define max(a,b) (a > b ? a : b)

using namespace std;
using namespace std::chrono;

int main(int argc, char **argv)
{
    high_resolution_clock::time_point previousTime = high_resolution_clock::now();
    Camera                            cameraL("/dev/video0", 320, 240, 120);
    Camera                            cameraR("/dev/video1", 320, 240, 120);
    fd_set                            cameras;

    for (int i = 0; i < 200; i++)
    {
        // Wait for a frame to become available
        int r;
        do 
        {
            FD_ZERO(&cameras);
            FD_SET(cameraL.GetDescriptor(), &cameras);
            FD_SET(cameraR.GetDescriptor(), &cameras);

            struct timeval tv;
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            r = select(max(cameraL.GetDescriptor(), cameraR.GetDescriptor()) + 1, &cameras, NULL, NULL, &tv);
        } 
        while ((r == -1 && (errno = EINTR)));
        
        if (r == -1) 
        {
            std::cerr << "Error occurred in select" << std::endl;
            return errno;
        }

        // Measure the drift between the two cameras
        high_resolution_clock::time_point currentTime = high_resolution_clock::now();
        duration<float> delta = duration_cast<duration<float>>(currentTime - previousTime);
        previousTime = currentTime;

        if(FD_ISSET(cameraL.GetDescriptor(), &cameras))
        {
            const Camera::Buffer& frame = cameraL.Capture();
            printf("cameraL %d - %.03f\n", i, delta.count()); 
            cameraL.Release(frame);
        }

        if(FD_ISSET(cameraR.GetDescriptor(), &cameras))
        {
            const Camera::Buffer& frame = cameraR.Capture();
            printf("cameraR %d - %.03f\n", i, delta.count()); 
            cameraR.Release(frame);
        }
    }

    return 0;
}
