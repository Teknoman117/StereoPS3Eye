#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__

#include <cinttypes>
#include <string>
#include <vector>

class Camera
{
public:
    struct Buffer 
    {
        void   *start;
        size_t  length;
        int     id;
    };

private:
    int                 fd;
    std::vector<Buffer> buffers;
    std::string         device;

public:
    Camera(const std::string& path, uint32_t width, uint32_t height, uint32_t fps);
    ~Camera();

    const Buffer& Capture(size_t *bytesUsed = NULL);
    void          Release(const Buffer& buf);
    int           GetDescriptor();
};

#endif
