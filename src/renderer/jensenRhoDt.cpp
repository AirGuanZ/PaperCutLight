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

    ComPtr<ID3D11Texture3D> tex;
    if(FAILED(d3d11::gDevice->CreateTexture3D(
        &texDesc, &subrscData, tex.GetAddressOf())))
        throw PCLException("failed to create jensen rho dt texture");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Texture3D.MipLevels       = 1;
    srvDesc.Texture3D.MostDetailedMip = 0;

    ComPtr<ID3D11ShaderResourceView> srv;
    if(FAILED(d3d11::gDevice->CreateShaderResourceView(
        tex.Get(), &srvDesc, srv.GetAddressOf())))
        throw PCLException("failed to create jensen rho dt srv");

    return srv;
}

PCL_END
