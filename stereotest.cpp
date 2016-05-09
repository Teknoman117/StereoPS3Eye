#include <iostream>
#include <chrono>
#include <cstdio>
#include <opencv2/opencv.hpp>

#include "stereo.hpp"

using namespace std;
using namespace std::chrono;
using namespace cv;

#define WIDTH  640
#define HEIGHT 480
#define FPS    30

int main(int argc, char **argv)
{
    high_resolution_clock::time_point previousTime;
    int i = 0;

    StereoCamera rig(WIDTH, HEIGHT, FPS);
    rig.SetCallback([&] (const Camera::Buffer& frameL, const Camera::Buffer& frameR, float delta)
    {
        std::cout << "Issued frame: delta = " << delta << std::endl;

        Mat imageL(HEIGHT, WIDTH, CV_8UC1, frameL.start);
        imshow("Image L", imageL);
        Mat imageR(HEIGHT, WIDTH, CV_8UC1, frameR.start);
        imshow("Image R", imageR);

        high_resolution_clock::time_point currentTime = high_resolution_clock::now();
        duration<float> deltaTime = duration_cast<duration<float>>(currentTime - previousTime);
        if(deltaTime.count() >= 1.0)
        {
            previousTime = currentTime;
            ostringstream lname;
            char name[256];
            sprintf(name, "left_%04d.ppm", i);
            imwrite(name, imageL);
            sprintf(name, "right_%04d.ppm", i++);
            imwrite(name, imageR);
        }
    });

    for(;;)
    {
        rig.Loop();
        if(waitKey(1) == 27) break;
    }

    return 0;
}
