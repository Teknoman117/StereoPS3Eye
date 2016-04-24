#include "camera.hpp"

#include <iostream>
#include <algorithm>
#include <cerrno>

extern "C"
{
    #include <libv4l2.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
    #include <sys/types.h>
    #include <sys/time.h>
    #include <sys/mman.h>
    #include <linux/videodev2.h>
}

namespace
{
    void xioctl(int fh, int request, void *arg)
    {
        int r;

        do 
        {
            r = v4l2_ioctl(fh, request, arg);
        } 
        while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

        if (r == -1) 
        {
            std::cerr << "error: " << errno << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

Camera::Camera(const std::string& path, uint32_t width, uint32_t height, uint32_t fps)
    : device(path)
{
    struct v4l2_format         format   = {};
    struct v4l2_streamparm     interval = {};
    struct v4l2_requestbuffers request  = {};
    struct v4l2_buffer         buffer   = {};

    // Open the camera
    fd = v4l2_open(path.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) 
    {
        std::cerr << "[" << path << "] FATAL: Could not open device" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Set the per-frame settings
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width       = width;
    format.fmt.pix.height      = height;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    format.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    xioctl(fd, VIDIOC_S_FMT, &format);

    if (format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) 
    {
        std::cerr << "[" << path << "] FATAL: libv4l2 didn\'t accept the format" << std::endl;
        exit(EXIT_FAILURE);
    }
    if ((format.fmt.pix.width != width) || (format.fmt.pix.height != height))
    {
        std::cerr << "[" << path << "] WARNING: driver is forcing resolution to (" << width << "," << height << ")" << std::endl;
    }

    // Set the capture interval of the camera
    interval.type                                  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    interval.parm.capture.timeperframe.numerator   = 1;
    interval.parm.capture.timeperframe.denominator = fps;
    xioctl(fd, VIDIOC_S_PARM, &interval);

    // Request memory buffers from the camera
    request.count  = 4;
    request.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, &request);

    buffers.resize(request.count);
    for (int i = 0; i < request.count; ++i) 
    {
        buffer = {};
        buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index  = i;
        xioctl(fd, VIDIOC_QUERYBUF, &buffer);

        buffers[i].start  = v4l2_mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);
        buffers[i].length = buffer.length;
        buffers[i].id = i;

        if (MAP_FAILED == buffers[i].start)
        {
            std::cerr << "[" << path << "] FATAL: Failed to mmap buffer" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Queue all the buffers for image capture
    for (int i = 0; i < buffers.size(); ++i)
    {
        buffer = {};
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;
        xioctl(fd, VIDIOC_QBUF, &buffer);
    }

    // Enable frame stream
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMON, &type);
}

Camera::~Camera()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type);

    for_each(buffers.begin(), buffers.end(), [&] (const Buffer& buffer)
    {
        v4l2_munmap(buffer.start, buffer.length);
    });

    v4l2_close(fd);
}

const Camera::Buffer& Camera::Capture(size_t *bytesUsed)
{
    struct v4l2_buffer buffer;

    buffer = {};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_DQBUF, &buffer);

    if(bytesUsed)
        *bytesUsed = buffer.bytesused;

    return buffers[buffer.index];
}

void Camera::Release(const Camera::Buffer& buf)
{
    struct v4l2_buffer buffer;

    buffer = {};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = buf.id;
    xioctl(fd, VIDIOC_QBUF, &buffer);
}

int Camera::GetDescriptor()
{
    return fd;
}
