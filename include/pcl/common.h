#pragma once

#include <agz/utility/graphics_api.h>
#include <agz/utility/texture.h>

#define PCL_BEGIN namespace pcl {
#define PCL_END   }

PCL_BEGIN

namespace d3d11 = agz::d3d11;

using d3d11::ComPtr;

using d3d11::Int2;
using d3d11::Int3;

using d3d11::Float2;
using d3d11::Float3;
using d3d11::Float4;
using d3d11::Color4;

template<typename T>
using Image2D = agz::texture::texture2d_t<T>;

class PCLException : public std::runtime_error
{
public:

    using runtime_error::runtime_error;
};

#define PCL_THROW_IF_FAILED_NOMSG(HR)                                           \
    do                                                                          \
    {                                                                           \
        if(FAILED(HR))                                                          \
            throw PCLException("FAILED(HR)");                                   \
    } while(false)

#define PCL_THROW_IF_FAILED(HR, MSG)                                            \
    do                                                                          \
    {                                                                           \
        if(FAILED(HR))                                                          \
            throw PCLException(MSG);                                            \
    } while(false)

PCL_END
