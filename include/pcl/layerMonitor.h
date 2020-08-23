#pragma once

#include <agz/utility/texture.h>
#include <FileWatcher/FileWatcher.h>

#include <pcl/common.h>

PCL_BEGIN

using LayerID = uint32_t;

struct LayerModification { LayerID id; };
struct LightModification { };

class LayerMonitor :
    public agz::event::sender_t<LayerModification, LightModification>,
    public FW::FileWatchListener
{
public:

    LayerMonitor();

    LayerID addPaperLayer(const std::string &filename);

    void removePaperLayer(LayerID id);

    void setLightLayer(const std::string &filename);

    void removeLightLayer();

    void update();

    const Image2D<uint8_t>            &getLayer(LayerID id) const noexcept;
    const Image2D<agz::math::color3b> &getLight() const noexcept;

private:

    void reinitWatcher();

    void handleFileAction(
        FW::WatchID       watchid,
        const FW::String &dir,
        const FW::String &filename,
        FW::Action        action) override;

    void reloadLayer(LayerID id);

    void reloadLight();

    struct Record
    {
        LayerID id = 0;
        std::filesystem::path path;
        Image2D<uint8_t> paper;
    };

    LayerID nextLayerID_;

    std::filesystem::path lightPath_;
    Image2D<agz::math::color3b> light_;

    std::map<std::filesystem::path, Record>  path2Paper_;
    std::map<LayerID, std::filesystem::path> id2Path_;

    FW::FileWatcher watcher_;
};

PCL_END
