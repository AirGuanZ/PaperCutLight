#include <agz/utility/string.h>

#include <pcl/langText.h>
#include <pcl/pcl.h>

PCL_BEGIN

namespace
{

    void drawSplitter(
        int splitVert, float thickness,
        float *size0, float *size1,
        float min_size0, float min_size1)
    {
        ImVec2 backupPos = ImGui::GetCursorPos();
        if(splitVert)
            ImGui::SetCursorPosY(backupPos.y + *size0);
        else
            ImGui::SetCursorPosX(backupPos.x + *size0 + 1);

        ImGui::PushStyleColor(
            ImGuiCol_Button, ImVec4(0.8f, 0.8f, 0.8f, 1));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive, ImVec4(0, 0.5f, 0.5f, 1));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered, ImVec4(0, 0.8f, 0.8f, 1));
        ImGui::Button(
            "##Splitter",
            ImVec2(
                !splitVert ? thickness : -1.0f,
                 splitVert ? thickness : -1.0f));
        ImGui::PopStyleColor(3);

        ImGui::SetItemAllowOverlap();

        if(ImGui::IsItemActive())
        {
            float mouseDelta =
                splitVert ? ImGui::GetIO().MouseDelta.y
                          : ImGui::GetIO().MouseDelta.x;

            if(mouseDelta < min_size0 - *size0)
                mouseDelta = min_size0 - *size0;
            if(mouseDelta > * size1 - min_size1)
                mouseDelta = *size1 - min_size1;

            *size0 += mouseDelta;
            *size1 -= mouseDelta;
        }
        ImGui::SetCursorPos(backupPos);
    }

    Image2D<Tracer::Texel> binaryToTracerTexels(const Image2D<uint8_t> &tex)
    {
        Image2D<Tracer::Texel> data(tex.height(), tex.width());
        for(int y = 0; y < data.height(); ++y)
        {
            for(int x = 0; x < data.width(); ++x)
            {
                auto &d = data(y, x);
                d.r = 255;
                d.g = 255;
                d.b = 255;
                d.binary = tex(y, x) > 0 ? 255 : 0;
            }
        }
        return data;
    }

    void showTip(const std::string &text)
    {
        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(text.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

} // namespace anonymous

PCL::PCL(const Int2 &paperSize)
    : loadAllFileBrowser_(ImGuiFileBrowserFlags_MultipleSelection)
{
    paperSize_ = paperSize;

    paperDistance_ = 10;
    paperWidth_    = 200;

    backLightDistance_  = 1;

    spp_           = 1;
    maxAccuFrames_ = 1024;

    perspectiveCamera_ = false;
    perspectiveCameraZ_ = 1;
    exposure_ = 1;

    envLight_ = { 0, 0, 0 };

    lightStatus_ = PaperRecord::Status::Nil;
    lightLayer_  = 0;
    lightIntensity_ = 1;

    selectedPaperIdx_ = 0;

    monitor_ = std::make_unique<LayerMonitor>();
    tracer_  = std::make_unique<Tracer>(
        paperSize,
        Int3(paperSize.x, paperSize.y, 1),
        paperDistance_ * paperSize_.x / paperWidth_,
        spp_);
    accumulator_ = std::make_unique<Accumulator>(paperSize_.x, paperSize_.y);
    toneMapper_ = std::make_unique<ToneMapper>(paperSize_.x, paperSize_.y);

    monitor_->attach<LayerModification>(this);
    monitor_->attach<LightModification>(this);

    tracer_->setEnvLight(envLight_);
    tracer_->setEyeZ(perspectiveCamera_ ? -perspectiveCameraZ_ : 1);

    toneMapper_->setExposure(exposure_);

    addNewPaper("");
}

void PCL::display(const d3d11::Window &window)
{
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize({
        static_cast<float>(window.getClientWidth()),
        static_cast<float>(window.getClientHeight())
    });
    ImGui::Begin("PCL", nullptr, ImGuiWindowFlags_NoDecoration);
    AGZ_SCOPE_GUARD({ ImGui::End(); });

    static float sizeLeft  = 0.3f * ImGui::GetContentRegionAvailWidth();
    static float sizeRight = ImGui::GetContentRegionAvailWidth() - sizeLeft;

    sizeLeft = agz::math::clamp(
        sizeLeft, 1.0f, ImGui::GetContentRegionAvailWidth() - 1);
    sizeRight = agz::math::clamp(
        sizeRight, 1.0f, ImGui::GetContentRegionAvailWidth() - 1);

    const float sizeVert  = ImGui::GetContentRegionAvail().y;

    drawSplitter(false, 6, &sizeLeft, &sizeRight, 1, 1);

    sizeRight = ImGui::GetContentRegionAvailWidth() - sizeLeft;

    ImGui::BeginChild("Left Panel", ImVec2(sizeLeft, sizeVert), true);
    displaySettingPanel();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("Right Panel", ImVec2(sizeRight, sizeVert), true);
    displayRenderPanel();
    ImGui::EndChild();

    monitor_->update();
}

bool PCL::isAccumulating() const noexcept
{
    return accumulator_->getAccumulatedFrameCount() < maxAccuFrames_;
}

void PCL::showStatusText(PaperRecord::Status status) const
{
    if(status == PaperRecord::Status::Nil)
        ImGui::TextColored({ 0.7f, 0.4f, 0, 1 }, PCL_LANG_FILE_NOT_SPECIFIED);
    else if(status == PaperRecord::Status::FailedToLoad)
        ImGui::TextColored({ 1, 0, 0, 1 }, PCL_LANG_FAILED_TO_LOAD);
    else if(status == PaperRecord::Status::SizeUnmatched)
        ImGui::TextColored({ 1, 0, 0, 1 }, PCL_LANG_UNMATCHED_SIZE);
    else
        ImGui::TextColored({ 0, 0.6f, 0, 1 }, PCL_LANG_SUCCESS_TO_LOAD);
}

void PCL::updatePaperBinary(size_t paperIndex)
{
    auto &paper = papers_[paperIndex];
    if(paper.status == PaperRecord::Status::Ok)
    {
        const auto data = binaryToTracerTexels(
            monitor_->getLayer(paper.layerID));

        tracer_->setPaperData(
            static_cast<int>(paperIndex), data.raw_data());
    }
    else
    {
        Image2D<Tracer::Texel> data(paperSize_.y, paperSize_.x, { 0, 0, 0, 0 });
        tracer_->setPaperData(
            static_cast<int>(paperIndex), data.raw_data());
    }
}

void PCL::updateMaterial()
{
    for(size_t i = 0; i < papers_.size(); ++i)
    {
        tracer_->setPaperJensen(
            static_cast<int>(i),
            jensenParams_.gf, jensenParams_.gb,
            jensenParams_.wf, 1 - jensenParams_.wf,
            jensenParams_.frontEta, jensenParams_.backEta,
            jensenParams_.frontM, jensenParams_.backM,
            jensenParams_.d, jensenParams_.sigmaS, jensenParams_.sigmaA,
            jensenParams_.diffusionAlbedo);
    }
    accumulator_->clearHistory();
}

void PCL::displaySettingPanel()
{
    if(ImGui::Button(PCL_LANG_ADD_LAYER))
        addNewPaper("");

    ImGui::SameLine();

    if(ImGui::Button(PCL_LANG_LOAD_ALL))
        loadAllFileBrowser_.Open();

    showTip(PCL_LANG_LOAD_ALL_TIPS);

    loadAllFileBrowser_.Display();

    if(loadAllFileBrowser_.HasSelected())
    {
        layerFileBrowser_.SetPwd(loadAllFileBrowser_.GetPwd());

        const auto selected = loadAllFileBrowser_.GetMultiSelected();
        loadAllFileBrowser_.ClearSelected();
        loadAllLayers(selected);
    }

    static int editPaperIdx = -1;

    bool needToRename = false;
    bool needToDelete = false;

    for(size_t i = 0; i < papers_.size(); ++i)
    {
        ImGui::PushID(int(i));
        AGZ_SCOPE_GUARD({ ImGui::PopID(); });

        const auto &paper = papers_[i];

        if(ImGui::Button(PCL_LANG_RENAME))
        {
            needToRename = true;
            editPaperIdx = int(i);
        }

        ImGui::SameLine();

        if(ImGui::Button(PCL_LANG_DELETE))
        {
            needToDelete = true;
            editPaperIdx = int(i);
        }

        ImGui::SameLine();

        if(ImGui::Button(PCL_LANG_BROWSE))
        {
            layerFileBrowser_.Open();
            editPaperIdx = int(i);
        }

        ImGui::SameLine();

        showStatusText(paper.status);

        if(ImGui::IsItemHovered() && !paper.filename.empty())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(paper.filename.c_str());
            ImGui::EndTooltip();
        }

        ImGui::SameLine();

        if(ImGui::Selectable(paper.name.c_str(), i == selectedPaperIdx_))
            selectedPaperIdx_ = i;

        if(ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("PAPER_LAYERS", &i, sizeof(i));
            ImGui::TextUnformatted(paper.name.c_str());
            ImGui::EndDragDropSource();
        }

        if(ImGui::BeginDragDropTarget())
        {
            if(auto payload = ImGui::AcceptDragDropPayload("PAPER_LAYERS"))
            {
                const size_t srcIdx = *static_cast<size_t *>(payload->Data);
                if(srcIdx < i)
                {
                    auto p = papers_[srcIdx];

                    for(size_t j = srcIdx; j < i; ++j)
                    {
                        papers_[j] = papers_[j + 1];
                        if(papers_[j].status != PaperRecord::Status::Nil)
                            layer2PaperIdx_[papers_[j].layerID] = j;
                        updatePaperBinary(j);
                    }

                    papers_[i] = p;
                    if(p.status != PaperRecord::Status::Nil)
                        layer2PaperIdx_[p.layerID] = i;
                    updatePaperBinary(i);
                }
                else if(srcIdx > i)
                {
                    auto p = papers_[srcIdx];
                    for(size_t j = srcIdx; j > i; --j)
                    {
                        papers_[j] = papers_[j - 1];
                        if(papers_[j].status != PaperRecord::Status::Nil)
                            layer2PaperIdx_[papers_[j].layerID] = j;
                        updatePaperBinary(j);
                    }

                    papers_[i] = p;
                    if(p.status != PaperRecord::Status::Nil)
                        layer2PaperIdx_[p.layerID] = i;
                    updatePaperBinary(i);
                }
                accumulator_->clearHistory();
            }
            ImGui::EndDragDropTarget();
        }
    }

    if(needToDelete)
        ImGui::OpenPopup(PCL_LANG_DELETE_WINDOW);
    if(ImGui::BeginPopup(PCL_LANG_DELETE_WINDOW))
    {
        if(ImGui::Button(PCL_LANG_OK))
        {
            tryRemoveLayer(editPaperIdx);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button(PCL_LANG_CANCEL))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if(needToRename)
        ImGui::OpenPopup(PCL_LANG_NEW_LAYER_NAME);
    if(ImGui::BeginPopup(PCL_LANG_NEW_LAYER_NAME))
    {
        static char newLayerName[512] = {};
        ImGui::InputText("", newLayerName, 512);

        if(ImGui::Button(PCL_LANG_OK))
        {
            papers_[editPaperIdx].name = findAvailName(newLayerName);
            newLayerName[0] = '\0';
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if(ImGui::Button(PCL_LANG_CANCEL))
        {
            newLayerName[0] = '\0';
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    layerFileBrowser_.Display();
    if(layerFileBrowser_.HasSelected() && editPaperIdx >= 0)
    {
        loadAllFileBrowser_.SetPwd(layerFileBrowser_.GetPwd());

        const auto filename = layerFileBrowser_.GetSelected();
        layerFileBrowser_.ClearSelected();
        setPaperFilename(
            static_cast<size_t>(editPaperIdx), filename.u8string());
    }

    // light

    ImGui::Separator();

    if(ImGui::Button(PCL_LANG_BROWSE_LIGHT))
    {
        editPaperIdx = -1;
        layerFileBrowser_.Open();
    }

    ImGui::SameLine();

    showStatusText(lightStatus_);

    if(ImGui::IsItemHovered() && !lightFilename_.empty())
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(lightFilename_.c_str());
        ImGui::EndTooltip();
    }

    if(layerFileBrowser_.HasSelected() && editPaperIdx < 0)
    {
        const auto filename = layerFileBrowser_.GetSelected();
        layerFileBrowser_.ClearSelected();
        setLightFilename(filename.u8string());
    }

    if(ImGui::SliderFloat(PCL_LANG_LIGHT_INTENSITY, &lightIntensity_, 0, 40))
        handle(LightModification{});

    if(ImGui::ColorEdit3(PCL_LANG_ENV_LIGHT, &envLight_.x))
    {
        tracer_->setEnvLight(envLight_.map([](float v)
        {
            return std::pow(v, 2.2f);
        }));
        accumulator_->clearHistory();
    }

    if(ImGui::SliderFloat(PCL_LANG_LIGHT_DISTANCE, &backLightDistance_, 1, 100))
    {
        tracer_->setBackLightDistance(
            backLightDistance_ * paperSize_.x / paperWidth_);
        accumulator_->clearHistory();
    }

    // global

    if(ImGui::InputFloat(PCL_LANG_PAPER_HORI_SIZE, &paperWidth_, 1, 10))
    {
        paperWidth_ = (std::max)(paperWidth_, 10.0f);
        tracer_->setPaperDistance(
            paperDistance_ * paperSize_.x / paperWidth_);
        tracer_->setBackLightDistance(
            backLightDistance_ *paperSize_.x / paperWidth_);
        accumulator_->clearHistory();
    }

    if(ImGui::SliderFloat(PCL_LANG_PAPER_DISTANCE, &paperDistance_, 1, 100))
    {
        tracer_->setPaperDistance(paperDistance_ * paperSize_.x / paperWidth_);
        accumulator_->clearHistory();
    }

    if(ImGui::Checkbox(PCL_LANG_USE_PERSPECTIVE, &perspectiveCamera_))
    {
        tracer_->setEyeZ(perspectiveCamera_ ? -(5 - perspectiveCameraZ_) : 1);
        accumulator_->clearHistory();
    }

    if(perspectiveCamera_ &&
       ImGui::SliderFloat(
           PCL_LANG_PERSPECTIVE_DISTANCE, &perspectiveCameraZ_, 0, 4.9f))
    {
        tracer_->setEyeZ(-(5 - perspectiveCameraZ_));
        accumulator_->clearHistory();
    }

    if(ImGui::SliderFloat(PCL_LANG_EXPOSURE, &exposure_, 0, 5))
    {
        toneMapper_->setExposure(exposure_);
        toneMapper_->render(accumulator_->getAccumulatedOutput());
    }

    // material

    if(ImGui::TreeNode(PCL_LANG_MATERIAL))
    {
        bool materialChanged = false;
        materialChanged |= ImGui::SliderFloat(
            PCL_LANG_FRONT_G, &jensenParams_.gf, -1, 1);
        materialChanged |= ImGui::SliderFloat(
            PCL_LANG_BACK_G, &jensenParams_.gb, -1, 1);
        materialChanged |= ImGui::SliderFloat(
            PCL_LANG_FRONT_G_WEIGHT, &jensenParams_.wf, 0, 1);
        materialChanged |= ImGui::SliderFloat(
            PCL_LANG_FRONT_IOR, &jensenParams_.frontEta, 1.01f, 3);
        materialChanged |= ImGui::SliderFloat(
            PCL_LANG_BACK_IOR, &jensenParams_.backEta, 1.01f, 3);
        materialChanged |= ImGui::SliderFloat(
            PCL_LANG_FRONT_ROUGH, &jensenParams_.frontM, 0, 1);
        materialChanged |= ImGui::SliderFloat(
            PCL_LANG_BACK_ROUGH, &jensenParams_.backM, 0, 1);
        materialChanged |= ImGui::InputFloat(
            PCL_LANG_PAPER_THICKNESS, &jensenParams_.d, 0.01f, 0.1f, 6);
        materialChanged |= ImGui::InputFloat(
            PCL_LANG_SIGMA_S, &jensenParams_.sigmaS, 1, 10, 6);
        materialChanged |= ImGui::InputFloat(
            PCL_LANG_SIGMA_A, &jensenParams_.sigmaA, 0.001f, 0.1f, 6);
        materialChanged |= ImGui::ColorEdit3(
            PCL_LANG_DIF_ALBEDO, &jensenParams_.diffusionAlbedo.x);

        if(materialChanged)
            updateMaterial();

        ImGui::TreePop();
    }

    // spp

    if(ImGui::TreeNode(PCL_LANG_SPP))
    {
        if(ImGui::SliderInt(PCL_LANG_GPU_PERFORMANCE, &spp_, 1, 8, ""))
        {
            tracer_->setSPP(spp_);
            accumulator_->clearHistory();
        }
        ImGui::SameLine();
        ImGui::Text("%d\n", spp_);

        ImGui::SliderInt(PCL_LANG_RENDER_QUALITY, &maxAccuFrames_, 1, 4096, "");
        ImGui::SameLine();
        ImGui::Text("%d\n", maxAccuFrames_);

        ImGui::TreePop();
    }
}

void PCL::displayRenderPanel()
{
    if(accumulator_->getAccumulatedFrameCount() < maxAccuFrames_)
    {
        tracer_->render();
        accumulator_->addNewFrame(tracer_->getOutput());
        toneMapper_->render(accumulator_->getAccumulatedOutput());
    }

    const auto [panelW, panelH] = ImGui::GetContentRegionAvail();

    ImVec2 size;
    if(paperSize_.x / panelW < paperSize_.y / panelH)
    {
        size.y = (std::min)(panelH, static_cast<float>(paperSize_.y));
        size.x = size.y * paperSize_.x / paperSize_.y;
    }
    else
    {
        size.x = (std::min)(panelW, static_cast<float>(paperSize_.x));
        size.y = size.x * paperSize_.y / paperSize_.x;
    }

    const ImVec2 pos = {
        (panelW - size.x) / 2,
        (panelH - size.y) / 2
    };

    ImGui::SetCursorPos(pos);
    ImGui::Image(toneMapper_->getOutput().Get(), size);
}

void PCL::handle(const LayerModification &event)
{
    const auto &tex = monitor_->getLayer(event.id);
    const size_t paperIdx = layer2PaperIdx_[event.id];
    auto &rcd = papers_[paperIdx];

    if(!tex.is_available())
        rcd.status = PaperRecord::Status::FailedToLoad;
    else if(tex.size() != paperSize_)
        rcd.status = PaperRecord::Status::SizeUnmatched;
    else
        rcd.status = PaperRecord::Status::Ok;

    updatePaperBinary(paperIdx);
    accumulator_->clearHistory();
}

void PCL::handle(const LightModification &event)
{
    const auto &tex = monitor_->getLight();

    if(!tex.is_available())
        lightStatus_ = PaperRecord::Status::FailedToLoad;
    else if(tex.size() != paperSize_)
        lightStatus_ = PaperRecord::Status::SizeUnmatched;
    else
        lightStatus_ = PaperRecord::Status::Ok;

    if(lightStatus_ == PaperRecord::Status::Ok)
    {
        Image2D<agz::math::color3f> data(tex.height(), tex.width());
        for(int y = 0; y < tex.height(); ++y)
        {
            for(int x = 0; x < tex.width(); ++x)
            {
                auto &t  = tex(y, x);
                auto &dt = data(y, x);
                dt.r = lightIntensity_ * std::pow(t.r / 255.0f, 2.2f);
                dt.g = lightIntensity_ * std::pow(t.g / 255.0f, 2.2f);
                dt.b = lightIntensity_ * std::pow(t.b / 255.0f, 2.2f);
            }
        }

        tracer_->setBackLightRadiance(data.raw_data());
        accumulator_->clearHistory();
    }
}

void PCL::addNewPaper(const std::string &name)
{
    papers_.emplace_back();
    auto &rcd = papers_.back();
    rcd.name    = findAvailName(name);
    rcd.layerID = 0;
    rcd.status  = PaperRecord::Status::Nil;

    tracer_->setPaperSize({
        paperSize_.x,
        paperSize_.y,
        static_cast<int>(papers_.size())
    });

    for(size_t i = 0; i < papers_.size(); ++i)
        updatePaperBinary(i);

    updateMaterial();
}

void PCL::setPaperFilename(size_t index, std::string filename)
{
    assert(index < papers_.size());

    auto &paper = papers_[index];

    if(paper.status != PaperRecord::Status::Nil)
    {
        monitor_->removePaperLayer(paper.layerID);
        layer2PaperIdx_.erase(paper.layerID);
    }

    paper.layerID = monitor_->addPaperLayer(filename);
    layer2PaperIdx_[paper.layerID] = index;

    paper.filename = std::move(filename);

    const Int2 texSize = monitor_->getLayer(paper.layerID).size();
    if(texSize != paperSize_)
    {
        paper.status = PaperRecord::Status::FailedToLoad;
        setPaperSize(texSize.x, texSize.y);
    }
    else
        handle(LayerModification{ paper.layerID });
}

void PCL::setLightFilename(std::string filename)
{
    monitor_->setLightLayer(filename);
    lightFilename_ = std::move(filename);

    const Int2 texSize = monitor_->getLight().size();
    if(texSize != paperSize_)
    {
        lightStatus_ = PaperRecord::Status::FailedToLoad;
        setPaperSize(texSize.x, texSize.y);
    }
    else
        handle(LightModification{});
}

std::string PCL::findAvailName(const std::string &name) const
{
    auto find = [&](const std::string &s)
    {
        for(auto &p : papers_)
        {
            if(p.name == s)
                return true;
        }
        return false;
    };

    if(!name.empty() && !find(name))
        return name;

    for(int i = 1;;)
    {
        const std::string s =
            name +
            "(" + agz::stdstr::align_right(std::to_string(i++), 2, '0') + ")";
        if(!find(s))
            return s;
    }
}

void PCL::tryRemoveLayer(size_t idx)
{
    if(papers_.size() == 1)
        return;

    auto &p = papers_[idx];
    if(p.status != PaperRecord::Status::Nil)
        monitor_->removePaperLayer(p.layerID);
    layer2PaperIdx_.erase(p.layerID);
    papers_.erase(papers_.begin() + idx);

    tracer_->setPaperSize({
        paperSize_.x,
        paperSize_.y,
        static_cast<int>(papers_.size())
        });

    if(selectedPaperIdx_ >= papers_.size())
        --selectedPaperIdx_;

    for(size_t i = 0; i < papers_.size(); ++i)
        updatePaperBinary(i);

    updateMaterial();
}

void PCL::loadAllLayers(const std::vector<std::filesystem::path> &all)
{
    for(auto &p : papers_)
    {
        if(p.status != PaperRecord::Status::Nil)
            monitor_->removePaperLayer(p.layerID);
    }
    papers_.clear();
    layer2PaperIdx_.clear();
    selectedPaperIdx_ = 0;

    tracer_->setPaperSize({
        paperSize_.x,
        paperSize_.y,
        static_cast<int>(all.size())
    });

    papers_.resize(all.size());
    for(size_t i = 0; i < papers_.size(); ++i)
    {
        papers_[i].name = findAvailName("");
        setPaperFilename(i, all[i].u8string());
    }

    updateMaterial();
}

void PCL::setPaperSize(int width, int height)
{
    Int2 oSize = { width, height };
    if(width > height)
    {
        oSize.x = (std::min)(width, 800);
        oSize.y = (std::max)(1, oSize.x * height / width);
    }
    else
    {
        oSize.y = (std::min)(height, 800);
        oSize.x = (std::max)(1, oSize.y * width / height);
    }

    paperSize_.x = width;
    paperSize_.y = height;

    tracer_->setPaperSize(
        {
            width, height, static_cast<int>(papers_.size())
        });
    tracer_->setOutputSize(oSize);
    accumulator_->setSize(oSize.x, oSize.y);
    toneMapper_->setSize(oSize.x, oSize.y);

    for(auto &p : papers_)
    {
        if(p.status != PaperRecord::Status::Nil)
            handle(LayerModification{ p.layerID });
    }

    if(lightStatus_ != PaperRecord::Status::Nil)
        handle(LightModification{});

    updateMaterial();
}

PCL_END
