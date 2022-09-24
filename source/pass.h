#pragma once
#include "rpass.h"

#include <d3d12.h>

namespace rgf {

	struct commandList {
		commandList(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type) {
			pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&pAllocator));
			ID3D12Device4* tempDevice;
			pDevice->QueryInterface(&tempDevice);
			tempDevice->CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&pList));
			tempDevice->Release();
		}
		~commandList() {
			pAllocator->Release();
			pList->Release();
		}

		void open(ID3D12PipelineState* pInitialState) {
			pAllocator->Reset();
			pList->Reset(pAllocator, pInitialState);
		}

		void close() {
			pList->Close();
		}

		ID3D12CommandList* getList() const {
			return pList;
		}

	private:
		ID3D12CommandAllocator* pAllocator;
		ID3D12GraphicsCommandList3* pList;
	};

}