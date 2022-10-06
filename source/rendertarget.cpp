#include "rendertarget.h"
#include "descriptor.h"

#include "D3D12MemAlloc.h"
#include "d3dx12.h"

namespace rgf {

	struct RenderTarget : public rendertarget {
		RenderTarget(rendertarget_desc* pDesc) {
			pDevice = pDesc->pDevice;
			mFormat = pDesc->mFormat;

			D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(mFormat, pDesc->mWidth, pDesc->mHeight);
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			D3D12MA::ALLOCATION_DESC allocDesc{};
			allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
			auto pAllocator = (D3D12MA::Allocator*)pDevice->getResourceAllocator();
			pAllocator->CreateResource(&allocDesc, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, NULL, &pAllocation, IID_PPV_ARGS(&pResource));

			pDescriptorManager = (descriptorManager*)pDevice->getDescriptorManager();
			pDescriptorManager->allocStatic(mSRV_Index);
			pDescriptorManager->bindSRV(mSRV_Index, pResource, nullptr);

			// RTV
			D3D12_DESCRIPTOR_HEAP_DESC desc{};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			desc.NumDescriptors = 1;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			pDevice->getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pRTV));

			SIZE_T rtvDescriptorSize = pDevice->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRTV->GetCPUDescriptorHandleForHeapStart();
			pDevice->getDevice()->CreateRenderTargetView(pResource, NULL, pRTV->GetCPUDescriptorHandleForHeapStart());
		}

		~RenderTarget() {
			pAllocation->Release();
			pResource->Release();
		}

		void release() {
			delete this;
		}

		ID3D12Resource* getResource() const {
			return pResource;
		}

		DXGI_FORMAT getFormat() const {
			return mFormat;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE getSRV() const {
			return pDescriptorManager->getVisableDescriptor(mSRV_Index);
		}
		D3D12_CPU_DESCRIPTOR_HANDLE getRTV() const {
			return  pRTV->GetCPUDescriptorHandleForHeapStart();
		}

		DXGI_FORMAT mFormat;

		device* pDevice;

		D3D12MA::Allocation* pAllocation;
		ID3D12Resource* pResource;

		descriptorManager* pDescriptorManager;
		uint32 mSRV_Index;

		ID3D12DescriptorHeap* pRTV;
	};

	rendertarget* create(rendertarget_desc* pDesc) {
		return new RenderTarget(pDesc);
	}
}