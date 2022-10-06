#pragma once
#include "define.h"
#include "object.h"

#include "d3dx12.h"

#include <d3d12.h>
#include <vector>

namespace rgf {
	struct descriptorManager : public object {
		descriptorManager(ID3D12Device* pDevice) {
			_init(pDevice);
		}

		~descriptorManager() {
			pDescriptorHeap->Release();
			pVisableDescriptorHeap->Release();
		}

		void release() {
			delete this;
		}

		void allocStatic(uint32& index) {
			index = 0;
			for (auto& state : mState) {
				if (!state.IsAssigned) {
					state.IsDynamic = false;
					state.IsAssigned = true;
					return;
				}
				++index;
			}

			DescriptorState state;
			state.IsAssigned = true;
			mState.emplace_back(state);

			if (mNumDescriptors != mState.capacity()) {
				ID3D12DescriptorHeap* pTempHeap;
				D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				heapDesc.NumDescriptors = mState.capacity();
				pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pTempHeap));
				heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				ID3D12DescriptorHeap* pTempVisableHeap;
				pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pTempVisableHeap));

				pDevice->CopyDescriptorsSimple(mNumDescriptors, pTempHeap->GetCPUDescriptorHandleForHeapStart(), pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				pDevice->CopyDescriptorsSimple(mNumDescriptors, pTempVisableHeap->GetCPUDescriptorHandleForHeapStart(), pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

				pDescriptorHeap->Release();
				pDescriptorHeap = pTempHeap;
				pVisableDescriptorHeap->Release();
				pVisableDescriptorHeap = pTempVisableHeap;

				mNumDescriptors = mState.capacity();
			}
		}

		void allocDynamic(uint32& index) {
			index = 0;
			for (auto& state : mState) {
				if (!state.IsAssigned) {
					state.IsDynamic = true;
					state.IsAssigned = true;
					return;
				}
				++index;
			}

			DescriptorState state;
			state.IsAssigned = true;
			mState.emplace_back(state);

			if (mNumDescriptors != mState.capacity()) {
				ID3D12DescriptorHeap* pTempHeap;
				D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				heapDesc.NumDescriptors = mState.capacity();
				pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pTempHeap));
				heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				ID3D12DescriptorHeap* pTempVisableHeap;
				pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pTempVisableHeap));

				pDevice->CopyDescriptorsSimple(mNumDescriptors, pTempHeap->GetCPUDescriptorHandleForHeapStart(), pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				pDevice->CopyDescriptorsSimple(mNumDescriptors, pTempVisableHeap->GetCPUDescriptorHandleForHeapStart(), pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

				pDescriptorHeap->Release();
				pDescriptorHeap = pTempHeap;
				pVisableDescriptorHeap->Release();
				pVisableDescriptorHeap = pTempVisableHeap;

				mNumDescriptors = mState.capacity();
			}
		}

		void remove(uint32 index) {
			mState[index].IsAssigned = false;
		}

		void update() {
			for (auto& state : mState) {
				if (state.IsDynamic) {
					state.IsAssigned = false;
				}
			}
		}

		void bindSRV(uint32 index, ID3D12Resource* pRes, const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc) {
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			CD3DX12_CPU_DESCRIPTOR_HANDLE visablehandle(pVisableDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			pDevice->CreateShaderResourceView(pRes, pDesc, handle);
			pDevice->CopyDescriptorsSimple(1, visablehandle, handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		void bindCBV(uint32 index, const D3D12_CONSTANT_BUFFER_VIEW_DESC* pDesc) {
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			CD3DX12_CPU_DESCRIPTOR_HANDLE visablehandle(pVisableDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			pDevice->CreateConstantBufferView(pDesc, handle);
			pDevice->CopyDescriptorsSimple(1, visablehandle, handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		void bindUAV(uint32 index, ID3D12Resource* pRes, ID3D12Resource* pCounterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc) {
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			CD3DX12_CPU_DESCRIPTOR_HANDLE visablehandle(pVisableDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			pDevice->CreateUnorderedAccessView(pRes, pCounterResource, pDesc, handle);
			pDevice->CopyDescriptorsSimple(1, visablehandle, handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE getVisableDescriptor(uint32 index) const {
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pVisableDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			return handle;
		}

		//D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescriptor(uint32 index) const {
		//	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
		//	return handle;
		//}

		//D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescriptor(uint32 index) const {
		//	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(pDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
		//	return handle;
		//}

		//void copy(uint32 index) {

		//}


		void _init(ID3D12Device* pDevice) {

			mNumDescriptors = 10;

			this->pDevice = pDevice;
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.NumDescriptors = mNumDescriptors;
			pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pDescriptorHeap));
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pVisableDescriptorHeap));

			mState.resize(mNumDescriptors);
			mState.shrink_to_fit();

			mDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		ID3D12Device* pDevice = nullptr;
		ID3D12DescriptorHeap* pDescriptorHeap = nullptr;
		ID3D12DescriptorHeap* pVisableDescriptorHeap = nullptr;

		struct DescriptorState {
			bool IsDynamic = false;
			bool IsAssigned = false;
		};
		std::vector<DescriptorState> mState;

		uint32 mNumDescriptors = 0;
		uint32 mDescriptorSize = 0;
	};

	inline descriptorManager* createDescriptorManager(ID3D12Device* pDevice) {
		return new descriptorManager(pDevice);
	}
}

