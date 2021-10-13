#include <iostream>

#include <agz-utils/graphics_api.h>

#include <pcl/pcl.h>

void run()
{
    using namespace agz::d3d11;

    WindowDesc windowDesc;
    windowDesc.title = L"Paper Cut Light Designer";

    Window window(windowDesc, true);
    window.useDefaultRTVAndDSV();
    window.useDefaultViewport();

    ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = { 0.854f, 0.854f, 0.854f, 1 };
    ImGui::GetIO().Fonts->AddFontFromFileTTF(
        "./asset/font.ttf", 18, nullptr,
        ImGui::GetIO().Fonts->GetGlyphRangesChineseFull());

    pcl::PCL pclProg({ 640, 480 });

    while(!window.getCloseFlag())
    {
        window.doEvents();
        window.newImGuiFrame();

        pclProg.display(window);
        window.setVSync(!pclProg.isAccumulating());

        window.clearDefaultRenderTarget({ 0, 1, 1, 0 });
        window.renderImGui();
        window.swapBuffers();
    }
}

int main()
{
    try
    {
        run();
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
