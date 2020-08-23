#include <agz/utility/image.h>

#include <pcl/layerMonitor.h>

PCL_BEGIN

namespace
{
    std::filesystem::path toStdPath(const std::filesystem::path &p)
    {
        return absolute(p).lexically_normal();
    }
}

LayerMonitor::LayerMonitor()
    : nextLayerID_(0)
{

}

LayerID LayerMonitor::addPaperLayer(const std::string &filename)
{
    const LayerID newID = nextLayerID_++;

    const std::filesystem::path path =
        toStdPath(std::filesystem::u8path(filename));

    path2Paper_[path] = { newID, path, {} };
    id2Path_[newID]  = path;

    reinitWatcher();
    reloadLayer(newID);

    return newID;
}

void LayerMonitor::removePaperLayer(LayerID id)
{
    auto it = id2Path_.find(id);
    assert(it != id2Path_.end());

    path2Paper_.erase(it->second);
    id2Path_.erase(it);

    reinitWatcher();
}

void LayerMonitor::setLightLayer(const std::string &filename)
{
    lightPath_ = toStdPath(std::filesystem::u8path(filename));
    reinitWatcher();
    reloadLight();
}

void LayerMonitor::removeLightLayer()
{
    lightPath_ = std::filesystem::path();
    reinitWatcher();
    reloadLight();
}

void LayerMonitor::update()
{
    watcher_.update();
}

const Image2D<uint8_t> &LayerMonitor::getLayer(LayerID id) const noexcept
{
    auto it = id2Path_.find(id);
    assert(it != id2Path_.end());
    auto it2 = path2Paper_.find(it->second);
    assert(it2 != path2Paper_.end());
    return it2->second.paper;
}

const Image2D<agz::math::color3b> &LayerMonitor::getLight() const noexcept
{
    return light_;
}

void LayerMonitor::reinitWatcher()
{
    watcher_ = FW::FileWatcher();

    std::set<std::filesystem::path> watchedPaths;

    auto addDir = [&](const std::filesystem::path &dir)
    {
        if(watchedPaths.find(dir) != watchedPaths.end())
            return;

        create_directories(dir);

        watchedPaths.insert(dir);
        watcher_.addWatch(dir.wstring(), this, false);
    };

    for(auto &p : path2Paper_)
        addDir(p.first.parent_path());

    if(!lightPath_.empty())
        addDir(lightPath_.parent_path());
}

void LayerMonitor::handleFileAction(
    FW::WatchID       watchid,
    const FW::String &dir,
    const FW::String &filename,
    FW::Action        action)
{
    const auto path = toStdPath(std::filesystem::path(dir) / filename);
    if(path == lightPath_)
    {
        reloadLight();
        send(LightModification{});
    }

    for(auto &p : path2Paper_)
    {
        if(path == p.first)
        {
            reloadLayer(p.second.id);
            send(LayerModification{ p.second.id });
        }
    }
}

void LayerMonitor::reloadLayer(LayerID id)
{
    auto it = id2Path_.find(id);
    assert(it != id2Path_.end());

    const auto &path = it->second;
    auto it2 = path2Paper_.find(path);
    assert(it2 != path2Paper_.end());

    try
    {
        auto newData = agz::img::load_gray_from_file(path.string());
        it2->second.paper = Image2D<uint8_t>(std::move(newData));
    }
    catch(...)
    {
        it2->second.paper = Image2D<uint8_t>();
    }
}

void LayerMonitor::reloadLight()
{
    if(lightPath_.empty())
    {
        light_ = Image2D<agz::math::color3b>();
        return;
    }

    try
    {
        auto newData = agz::img::load_rgb_from_file(lightPath_.string());
        light_ = Image2D<agz::math::color3b>(std::move(newData));
    }
    catch(...)
    {
        light_ = Image2D<agz::math::color3b>();
    }
}

PCL_END
