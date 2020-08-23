#pragma once

#include <pcl/painter/canvas.h>

PCL_BEGIN

struct SceneData
{
    std::vector<Canvas<bool>>  paperBinaries;
    Canvas<agz::math::color3b> lightColor;
};

PCL_END
