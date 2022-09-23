#include "rpass.h"
#include "d3dx12.h"
#include "D3D12MemAlloc.h"

#include "shader/vs/gbuffer.h"
#include "shader/ps/gbuffer.h"

#include <DirectXMath.h>

namespace rgf {
	struct GBufferPass : public gbufferPass {
		GBufferPass(gbufferPassDesc* pDesc) {
			pDevice = pDesc->pDevice->getDevice();

			CD3DX12_STATIC_SAMPLER_DESC StaticSamplers;
			StaticSamplers.Init(0, D3D12_FILTER_MIN_MAG_MIP_POINT);
			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSig;
			RootSig.Init_1_1(0, nullptr, 1, &StaticSamplers, ((D3D12_ROOT_SIGNATURE_FLAGS)0x400) | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
			ID3DBlob* pSerializedRootSig;
			D3D12SerializeVersionedRootSignature(&RootSig, &pSerializedRootSig, nullptr);
			pDevice->CreateRootSignature(0, pSerializedRootSig->GetBufferPointer(), pSerializedRootSig->GetBufferSize(), IID_PPV_ARGS(&pRootSignature));
			pSerializedRootSig->Release();

			D3D12_INPUT_ELEMENT_DESC gbufferInputDesc[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "ID", 0, DXGI_FORMAT_R32_UINT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
			psoDesc.InputLayout = { gbufferInputDesc ,4 };
			psoDesc.pRootSignature = pRootSignature;
			psoDesc.VS = { gbufferVS, sizeof(gbufferVS) };
			psoDesc.PS = { gbufferPS, sizeof(gbufferPS) };
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.SampleMask = 0xffffffff;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 2;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
			psoDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPSO));

			pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pGBufferAllocator));
			ID3D12Device4* tempDevice;
			pDevice->QueryInterface(&tempDevice);
			tempDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&pGBufferList));
			tempDevice->Release();
			pGBufferList->SetName(L"GBufferList");

			pAllocator = (D3D12MA::Allocator*)pDesc->pDevice->getResourceAllocator();

			mLightDirAndintensity = DirectX::XMFLOAT4(1.F, 1.F, 1.F, 1.F);

			D3D12MA::ALLOCATION_DESC allocDesc{};
			allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
			CD3DX12_RESOURCE_DESC MatrixDesc = CD3DX12_RESOURCE_DESC::Buffer(_getConstantBufferSize(sizeof(GBufferMatrix)));
			pAllocator->CreateResource(&allocDesc, &MatrixDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, &pGBufferMatrix, IID_NULL, nullptr);
			CD3DX12_RESOURCE_DESC LightDesc = CD3DX12_RESOURCE_DESC::Buffer(_getConstantBufferSize(sizeof(GBufferLight)));
			pAllocator->CreateResource(&allocDesc, &LightDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, &pGBufferLight, IID_NULL, nullptr);
			pGBufferMatrix->GetResource()->Map(0, 0, &pGBufferMatrixData);
			pGBufferLight->GetResource()->Map(0, 0, &pGBufferLightData);
			// todo
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
			cbvDesc.BufferLocation = pGBufferMatrix->GetResource()->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = _getConstantBufferSize(sizeof(GBufferMatrix));
			//pDevice->CreateConstantBufferView(&cbvDesc,)
		}

		~GBufferPass() {
			pGBufferLight->GetResource()->Unmap(0, nullptr);
			pGBufferMatrix->GetResource()->Unmap(0, nullptr);
			pGBufferAllocator->Release();
			pGBufferList->Release();
			pRootSignature->Release();
			pPSO->Release();
		}

		void release() {
			delete this;
		}

		ID3D12CommandList* getList() const {
			return pGBufferList;
		}

		void open() {
			pGBufferAllocator->Reset();
			pGBufferList->Reset(pGBufferAllocator, pPSO);
		}

		void close() {
			pGBufferList->Close();
		}

		void updateSkyLightDirection(float x, float y, float z) {
			mLightDirAndintensity.x = x;
			mLightDirAndintensity.y = y;
			mLightDirAndintensity.z = z;
		}

		void updateSkyLightIntensity(float intensity) {
			mLightDirAndintensity.w = intensity;
		}

		ID3D12Device* pDevice;

		ID3D12CommandAllocator* pGBufferAllocator;
		ID3D12GraphicsCommandList3* pGBufferList;
		ID3D12RootSignature* pRootSignature;
		ID3D12PipelineState* pPSO;

		D3D12MA::Allocator* pAllocator;

		D3D12MA::Allocation* pGBufferMatrix;
		D3D12MA::Allocation* pGBufferLight;
		void* pGBufferMatrixData;
		void* pGBufferLightData;

		DirectX::XMFLOAT4 mLightDirAndintensity;

		struct GBufferMatrix {
			DirectX::XMFLOAT4X4 MVP;
			DirectX::XMFLOAT4X4 WorldInvTranspose;
		};

		struct GBufferLight {
			DirectX::XMFLOAT4 DirectionAndIntensity;
		};

		uint64 _getConstantBufferSize(uint64 size) {
			if (size == 0) {
				return 256;
			}
			auto t = size / 256;
			if (size % 256 == 0) {
				return t * 256;
			}
			return (t + 1) * 256;
		}
	};

	gbufferPass* create(gbufferPassDesc* pDesc) {
		return new GBufferPass(pDesc);
	}
}