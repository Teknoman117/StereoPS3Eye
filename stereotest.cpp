#include <iostream>
#include <opencv2/opencv.hpp>

#include "stereo.hpp"

using namespace std;
using namespace cv;

int main(int argc, char **argv)
{
    StereoCamera rig(320, 240, 100);
    rig.SetCallback([&] (const Camera::Buffer& frameL, const Camera::Buffer& frameR, float delta)
    {
        std::cout << "Issued frame: delta = " << delta << std::endl;

        Mat imageL(240, 320, CV_8UC1, frameL.start);
        imshow("Image L", imageL);
        Mat imageR(240, 320, CV_8UC1, frameR.start);
        imshow("Image R", imageR);
    });

    for(;;)
    {
        rig.Loop();
        if(waitKey(1) == 27) break;
    }

    return 0;
}
