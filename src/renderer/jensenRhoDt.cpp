#include <pcl/renderer/jensenRhoDt.h>

PCL_BEGIN

ComPtr<ID3D11ShaderResourceView> loadJensenRhoDt()
{
    using real = float;

    static const float data[] = {
#include "paper_rho_dt.txt"
    };

    D3D11_TEXTURE3D_DESC texDesc;
    texDesc.Width          = 64;
    texDesc.Height         = 64;
    texDesc.Depth          = 64;
    texDesc.MipLevels      = 1;
    texDesc.Format         = DXGI_FORMAT_R32_FLOAT;
    texDesc.Usage          = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags      = 0;

    D3D11_SUBRESOURCE_DATA subrscData;
    subrscData.pSysMem          = data;
    subrscData.SysMemPitch      = 64 * sizeof(float);
    subrscData.SysMemSlicePitch = 64 * 64 * sizeof(float);

    auto tex = d3d11::device.createTex3D(texDesc, &subrscData);
    
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Texture3D.MipLevels       = 1;
    srvDesc.Texture3D.MostDetailedMip = 0;

    auto srv = d3d11::device.createSRV(tex, srvDesc);
    
    return srv;
}

PCL_END
