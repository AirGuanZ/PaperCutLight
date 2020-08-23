#pragma once

#include <agz/utility/graphics_api.h>

#include <pcl/renderer/accumulator.h>
#include <pcl/renderer/toneMapper.h>
#include <pcl/renderer/tracer.h>
#include <pcl/layerMonitor.h>

PCL_BEGIN

class PCL :
    public agz::event::receiver_t<LayerModification>,
    public agz::event::receiver_t<LightModification>
{
public:

    explicit PCL(const Int2 &paperSize);

    void display(const d3d11::Window &window);

    bool isAccumulating() const noexcept;

private:

    struct PaperRecord
    {
        enum class Status
        {
            Nil,
            Ok,
            FailedToLoad,
            SizeUnmatched
        };

        std::string name;
        std::string filename;

        Status status   = Status::Nil;
        LayerID layerID = 0;
    };

    struct JensenParams
    {
        float gf               = 0.335f;
        float gb               = -0.841f;
        float wf               = 0.997f;
        float frontEta         = 1.29f;
        float backEta          = 1.55f;
        float frontM           = 0.419f;
        float backM            = 0.892f;
        float d                = 0.262f;
        float sigmaS           = 81.38f;
        float sigmaA           = 0.001f;
        Float3 diffusionAlbedo = { 0.54f, 0.54f, 0.54f };
    };

    void showStatusText(PaperRecord::Status status) const;

    void updatePaperBinary(size_t paperIndex);

    void updateMaterial();

    void displaySettingPanel();

    void displayRenderPanel();

    void handle(const LayerModification &event) override;

    void handle(const LightModification &event) override;

    void addNewPaper(const std::string &name);

    void setPaperFilename(size_t index, std::string filename);

    void setLightFilename(std::string filename);

    std::string findAvailName(const std::string &name) const;

    void tryRemoveLayer(size_t idx);

    void loadAllLayers(const std::vector<std::filesystem::path> &all);

    void setPaperSize(int width, int height);

    Int2 paperSize_;

    JensenParams jensenParams_;

    float paperDistance_;
    float paperWidth_;
    float backLightDistance_;

    int spp_;
    int maxAccuFrames_;

    bool perspectiveCamera_;
    float perspectiveCameraZ_;
    float exposure_;

    Float3 envLight_;

    PaperRecord::Status lightStatus_;
    std::string lightFilename_;
    LayerID lightLayer_;

    float lightIntensity_;

    size_t selectedPaperIdx_;
    std::vector<PaperRecord> papers_;
    std::map<LayerID, size_t> layer2PaperIdx_;

    std::unique_ptr<LayerMonitor> monitor_;

    std::unique_ptr<Tracer>      tracer_;
    std::unique_ptr<Accumulator> accumulator_;
    std::unique_ptr<ToneMapper>  toneMapper_;

    ImGui::FileBrowser loadAllFileBrowser_;
    ImGui::FileBrowser layerFileBrowser_;
};

PCL_END
