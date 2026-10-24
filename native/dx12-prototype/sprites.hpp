// Original Moonfall characters, bear, creeps and Titans rendered with Direct3D 12.
// Image bytes are prepared from the embedded Moonfall assets at build time.
// No frame generation, NVIDIA SDK or changes to the original artwork.
#pragma once
#include <windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace moonfall {
using Microsoft::WRL::ComPtr;
inline void RequireDx(HRESULT result, const char* label) {
    if(FAILED(result))throw std::runtime_error(std::string(label)+": HRESULT "+
                                             std::to_string(static_cast<unsigned>(result)));
}
struct OriginalHeroImage {
    ComPtr<ID3D12Resource> texture;
    ComPtr<ID3D12Resource> upload;
    UINT width=0,height=0;
};
struct SpriteVertex { float x,y,u,v; };

class OriginalHeroSprites {
    static constexpr UINT MaxSpritesPerFrame=128;
    UINT drawCursor=0;
    ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> pipeline;
    ComPtr<ID3D12DescriptorHeap> descriptors;
    ComPtr<ID3D12Resource> vertices;
    std::array<OriginalHeroImage,8> heroes{};
    UINT stride=0;

    static D3D12_RESOURCE_DESC BufferDescription(UINT64 size) {
        D3D12_RESOURCE_DESC desc={};
        desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width=size;
        desc.Height=1;
        desc.DepthOrArraySize=1;
        desc.MipLevels=1;
        desc.Format=DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count=1;
        desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        return desc;
    }
    static std::vector<std::uint8_t> ReadPng(const wchar_t* name,UINT& width,UINT& height){
        ComPtr<IWICImagingFactory> factory;
        RequireDx(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&factory)),"WIC imaging factory");
        ComPtr<IWICBitmapDecoder> decoder;
        RequireDx(factory->CreateDecoderFromFilename(name,nullptr,GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,&decoder),"Decode original character PNG");
        ComPtr<IWICBitmapFrameDecode> frame;
        RequireDx(decoder->GetFrame(0,&frame),"Original art first frame");
        RequireDx(frame->GetSize(&width,&height),"Original art size");
        if(width==0||height==0||width>8192||height>8192)
            throw std::runtime_error("Original hero image size outside supported bounds");
        ComPtr<IWICFormatConverter> converter;
        RequireDx(factory->CreateFormatConverter(&converter),"WIC format converter");
        RequireDx(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,
             WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom),
             "Convert original hero PNG to RGBA8 pixels");
        const UINT pitch=width*4;
        std::vector<std::uint8_t> pixels(size_t(pitch)*height);
        RequireDx(converter->CopyPixels(nullptr,pitch,static_cast<UINT>(pixels.size()),
                                       pixels.data()),"Read unmodified hero pixels");
        return pixels;
    }
    static ComPtr<ID3DBlob> Compile(const char* source,const char* entry,const char* target){
        ComPtr<ID3DBlob> code,errors;
        HRESULT hr=D3DCompile(source,std::strlen(source),nullptr,nullptr,nullptr,entry,target,
                              D3DCOMPILE_ENABLE_STRICTNESS,0,&code,&errors);
        if(FAILED(hr)){
            const std::string message=errors?
              std::string(static_cast<const char*>(errors->GetBufferPointer()),errors->GetBufferSize()):
              std::string("HLSL compilation failed");
            throw std::runtime_error(message);
        }
        return code;
    }
    void CreateShaders(ID3D12Device* device){
        static const char* shader=R"(
            Texture2D heroArt:register(t0);
            SamplerState clampSampler:register(s0);
            struct VSInput { float2 pos:POSITION; float2 uv:TEXCOORD0; };
            struct PSInput { float4 pos:SV_POSITION; float2 uv:TEXCOORD0; };
            PSInput vsMain(VSInput input){
                PSInput result;
                result.pos=float4(input.pos,0,1);
                result.uv=input.uv;
                return result;
            }
            float4 psMain(PSInput input):SV_Target0{
                return heroArt.Sample(clampSampler,input.uv);
            }
        )";
        const auto vs=Compile(shader,"vsMain","vs_5_0");
        const auto ps=Compile(shader,"psMain","ps_5_0");
        D3D12_DESCRIPTOR_RANGE range={};
        range.RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors=1;
        range.BaseShaderRegister=0;
        D3D12_ROOT_PARAMETER parameter={};
        parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameter.DescriptorTable.NumDescriptorRanges=1;
        parameter.DescriptorTable.pDescriptorRanges=&range;
        parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC sampler={};
        sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
        sampler.MaxLOD=D3D12_FLOAT32_MAX;
        sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC signature={};
        signature.NumParameters=1;
        signature.pParameters=&parameter;
        signature.NumStaticSamplers=1;
        signature.pStaticSamplers=&sampler;
        signature.Flags=D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        ComPtr<ID3DBlob> signatureBlob, errors;
        RequireDx(D3D12SerializeRootSignature(&signature,D3D_ROOT_SIGNATURE_VERSION_1,
                 &signatureBlob,&errors),"Serialize sprite signature");
        RequireDx(device->CreateRootSignature(0,signatureBlob->GetBufferPointer(),
            signatureBlob->GetBufferSize(),IID_PPV_ARGS(&root)),"Create sprite signature");
        const D3D12_INPUT_ELEMENT_DESC elements[]={
            {"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,8,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
        };
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc={};
        desc.pRootSignature=root.Get();
        desc.VS={vs->GetBufferPointer(),vs->GetBufferSize()};
        desc.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
        desc.InputLayout={elements,2};
        desc.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;
        desc.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
        desc.RasterizerState.FrontCounterClockwise=FALSE;
        desc.RasterizerState.DepthClipEnable=TRUE;
        auto& blend=desc.BlendState.RenderTarget[0];
        blend.BlendEnable=TRUE;
        blend.SrcBlend=D3D12_BLEND_SRC_ALPHA;
        blend.DestBlend=D3D12_BLEND_INV_SRC_ALPHA;
        blend.BlendOp=D3D12_BLEND_OP_ADD;
        blend.SrcBlendAlpha=D3D12_BLEND_ONE;
        blend.DestBlendAlpha=D3D12_BLEND_INV_SRC_ALPHA;
        blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;
        blend.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
        desc.SampleMask=UINT_MAX;
        desc.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets=1;
        desc.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count=1;
        RequireDx(device->CreateGraphicsPipelineState(&desc,IID_PPV_ARGS(&pipeline)),
                  "Create alpha-blended original art pipeline");
    }
    void LoadImage(ID3D12Device* device,ID3D12GraphicsCommandList* command,
                   int index,const wchar_t* filename){
        auto& image=heroes.at(size_t(index));
        auto bytes=ReadPng(filename,image.width,image.height);
        D3D12_RESOURCE_DESC tex={};
        tex.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        tex.Width=image.width;
        tex.Height=image.height;
        tex.DepthOrArraySize=1;
        tex.MipLevels=1;
        tex.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        tex.SampleDesc.Count=1;
        tex.Layout=D3D12_TEXTURE_LAYOUT_UNKNOWN;
        D3D12_HEAP_PROPERTIES gpuHeap={};
        gpuHeap.Type=D3D12_HEAP_TYPE_DEFAULT;
        RequireDx(device->CreateCommittedResource(&gpuHeap,D3D12_HEAP_FLAG_NONE,
              &tex,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&image.texture)),
              "Create original hero texture");
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout={};
        UINT64 uploadBytes=0;
        device->GetCopyableFootprints(&tex,0,1,0,&layout,nullptr,nullptr,&uploadBytes);
        D3D12_HEAP_PROPERTIES uploadHeap={};
        uploadHeap.Type=D3D12_HEAP_TYPE_UPLOAD;
        const auto uploadDescription=BufferDescription(uploadBytes);
        RequireDx(device->CreateCommittedResource(&uploadHeap,D3D12_HEAP_FLAG_NONE,
             &uploadDescription,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,
             IID_PPV_ARGS(&image.upload)),"Allocate hero texture staging");
        std::uint8_t* mapped=nullptr;
        RequireDx(image.upload->Map(0,nullptr,reinterpret_cast<void**>(&mapped)),"Map hero upload");
        for(UINT row=0;row<image.height;row++){
            std::memcpy(mapped+layout.Offset+size_t(row)*layout.Footprint.RowPitch,
                        bytes.data()+size_t(row)*image.width*4,size_t(image.width)*4);
        }
        image.upload->Unmap(0,nullptr);
        D3D12_TEXTURE_COPY_LOCATION dest={},source={};
        dest.pResource=image.texture.Get();
        dest.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dest.SubresourceIndex=0;
        source.pResource=image.upload.Get();
        source.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        source.PlacedFootprint=layout;
        command->CopyTextureRegion(&dest,0,0,0,&source,nullptr);
        D3D12_RESOURCE_BARRIER barrier={};
        barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource=image.texture.Get();
        barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        command->ResourceBarrier(1,&barrier);
        D3D12_SHADER_RESOURCE_VIEW_DESC view={};
        view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view.Format=tex.Format;
        view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Texture2D.MipLevels=1;
        auto handle=descriptors->GetCPUDescriptorHandleForHeapStart();
        handle.ptr+=size_t(index)*stride;
        device->CreateShaderResourceView(image.texture.Get(),&view,handle);
    }
public:
    void Initialize(ID3D12Device* device,ID3D12GraphicsCommandList* command,
                    const std::array<std::wstring,8>& originalArt){
        CreateShaders(device);
        D3D12_DESCRIPTOR_HEAP_DESC heap={};
        heap.NumDescriptors=static_cast<UINT>(heroes.size());
        heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        RequireDx(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&descriptors)),
                  "Original hero sprite descriptors");
        stride=device->GetDescriptorHandleIncrementSize(heap.Type);
        D3D12_HEAP_PROPERTIES memory={};
        memory.Type=D3D12_HEAP_TYPE_UPLOAD;
        auto desc=BufferDescription(sizeof(SpriteVertex)*6*MaxSpritesPerFrame);
        RequireDx(device->CreateCommittedResource(&memory,D3D12_HEAP_FLAG_NONE,
            &desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&vertices)),
            "Allocate dynamic sprite vertices");
        for(size_t i=0;i<originalArt.size();++i)
            LoadImage(device,command,static_cast<int>(i),originalArt[i].c_str());
    }
    void StartFrame(){drawCursor=0;}
    void Draw(ID3D12GraphicsCommandList* cmd,int selected,float x,float y,
              D3D12_CPU_DESCRIPTOR_HANDLE renderTarget,float size=94.f){
        if(drawCursor>=MaxSpritesPerFrame)throw std::runtime_error("Sprite vertex draw capacity exceeded");
        const int hero=std::max(0,std::min(7,selected));
        const auto& source=heroes[size_t(hero)];
        const float ratio=static_cast<float>(source.width)/static_cast<float>(source.height);
        const float width=ratio>1.f?size:size*ratio;
        const float height=ratio>1.f?size/ratio:size;
        const float l=x-width*.5f,r=x+width*.5f,t=y-height*.5f,b=y+height*.5f;
        const auto clipX=[](float px){return px/512.f-1.f;};
        const auto clipY=[](float py){return 1.f-py/288.f;};
        const SpriteVertex quad[6]={
            {clipX(l),clipY(t),0,0},{clipX(r),clipY(t),1,0},{clipX(l),clipY(b),0,1},
            {clipX(r),clipY(t),1,0},{clipX(r),clipY(b),1,1},{clipX(l),clipY(b),0,1}
        };
        const size_t offset=size_t(drawCursor++)*sizeof(quad);
        void* mapped=nullptr;
        RequireDx(vertices->Map(0,nullptr,&mapped),"Map original art quad");
        std::memcpy(static_cast<std::uint8_t*>(mapped)+offset,quad,sizeof(quad));
        vertices->Unmap(0,nullptr);
        D3D12_VERTEX_BUFFER_VIEW view={};
        view.BufferLocation=vertices->GetGPUVirtualAddress()+offset;
        view.SizeInBytes=sizeof(quad);
        view.StrideInBytes=sizeof(SpriteVertex);
        ID3D12DescriptorHeap* heap=descriptors.Get();
        cmd->SetDescriptorHeaps(1,&heap);
        cmd->SetGraphicsRootSignature(root.Get());
        cmd->SetPipelineState(pipeline.Get());
        auto gpuHandle=descriptors->GetGPUDescriptorHandleForHeapStart();
        gpuHandle.ptr+=UINT64(hero)*stride;
        cmd->SetGraphicsRootDescriptorTable(0,gpuHandle);
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->IASetVertexBuffers(0,1,&view);
        const D3D12_VIEWPORT viewport={0,0,1024,576,0,1};
        const D3D12_RECT scissor={0,0,1024,576};
        cmd->RSSetViewports(1,&viewport);
        cmd->RSSetScissorRects(1,&scissor);
        cmd->OMSetRenderTargets(1,&renderTarget,FALSE,nullptr);
        cmd->DrawInstanced(6,1,0,0);
    }
};
} // namespace moonfall
