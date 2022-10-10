#include "rgf/device.h"

#include <dxgi1_6.h>

#define FRAME_COUNT 2
#define FRAME_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

namespace rgf {

	ID3D12Device4* _getDevice(IDXGIAdapter* pAdapter = nullptr) {
#ifdef _DEBUG
		ID3D12Debug* debugController = nullptr;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
		debugController->Release();
#endif
		ID3D12Device4* pDevice;
		D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&pDevice));
		return pDevice;
	}
	ID3D12CommandQueue* _getQueue(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type) {
		D3D12_COMMAND_QUEUE_DESC queueDesc{};
		queueDesc.Type = type;
		ID3D12CommandQueue* pQueue;
		pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pQueue));
		return pQueue;
	}

	IDXGISwapChain4* _getSwapChain(ID3D12CommandQueue* pQueue, HWND hwnd, uint32 w, uint32 h) {
		IDXGIFactory7* pFactory;
#ifdef _DEBUG
		CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&pFactory));
#else
		CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory));
#endif
		DXGI_SWAP_CHAIN_DESC1 scDesc{};
		scDesc.BufferCount = FRAME_COUNT;
		scDesc.Width = w;
		scDesc.Height = h;
		scDesc.Format = FRAME_FORMAT;
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scDesc.SampleDesc.Count = 1;
		scDesc.SampleDesc.Quality = 0;
		scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		scDesc.Scaling = DXGI_SCALING_STRETCH;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc{};
		fsSwapChainDesc.Windowed = TRUE;
		IDXGISwapChain1* pSwapChain;
		pFactory->CreateSwapChainForHwnd(pQueue, hwnd, &scDesc, &fsSwapChainDesc, nullptr, &pSwapChain);
		pFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
		IDXGISwapChain4* pSwapChain4;
		pSwapChain->QueryInterface(&pSwapChain4);

		pSwapChain->Release();
		pFactory->Release();
		return pSwapChain4;
	}

	struct Device : public IDevice {

		Device(DeviceDesc* pDesc) {
			pDevice = _getDevice();
			pGraphicsQueue = _getQueue(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
			pCopyQueue = _getQueue(pDevice, D3D12_COMMAND_LIST_TYPE_COPY);
			pComputeQueue = _getQueue(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
			pSwapChain = _getSwapChain(pGraphicsQueue, pDesc->mHwnd, pDesc->mWidth, pDesc->mHeight);
			mGFenceValue = 1;
			pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pGFence));
			mGFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
			mCopyFenceValue = 1;
			pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pCopyFence));
			mCopyFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
			mComputeFenceValue = 1;
			pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pComputeFence));
			mComputeFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

			_wait(mGFenceValue, pGraphicsQueue, pGFence, mGFenceEvent);
			_wait(mCopyFenceValue, pCopyQueue, pCopyFence, mCopyFenceEvent);
			_wait(mComputeFenceValue, pComputeQueue, pComputeFence, mComputeFenceEvent);

		}
		~Device() {
			_wait(mGFenceValue, pGraphicsQueue, pGFence, mGFenceEvent);
			_wait(mCopyFenceValue, pCopyQueue, pCopyFence, mCopyFenceEvent);
			_wait(mComputeFenceValue, pComputeQueue, pComputeFence, mComputeFenceEvent);

			CloseHandle(mComputeFenceEvent);
			pComputeFence->Release();
			CloseHandle(mCopyFenceEvent);
			pCopyFence->Release();
			CloseHandle(mGFenceEvent);
			pGFence->Release();
			pSwapChain->Release();
			pComputeQueue->Release();
			pCopyQueue->Release();
			pGraphicsQueue->Release();
			pDevice->Release();
		}

		void _wait(uint64& fenceValue, ID3D12CommandQueue* pQueue, ID3D12Fence* pFence, HANDLE fenceEvent) {
			const uint64 fence = fenceValue;
			pQueue->Signal(pFence, fence);
			fenceValue++;

			if (pFence->GetCompletedValue() < fence)
			{
				pFence->SetEventOnCompletion(fence, fenceEvent);
				WaitForSingleObject(fenceEvent, INFINITE);
			}
		}

		virtual ID3D12Device* getDevice() const {
			return pDevice;
		}
		virtual ID3D12CommandQueue* getQueue(D3D12_COMMAND_LIST_TYPE type) const {
			if (type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
				return pGraphicsQueue;
			}
			if (type == D3D12_COMMAND_LIST_TYPE_COPY) {
				return pCopyQueue;
			}
			if (type == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
				return pComputeQueue;
			}
			return nullptr;
		}
		virtual void frame() {
			pSwapChain->Present(1, 0);
			_wait(mGFenceValue, pGraphicsQueue, pGFence, mGFenceEvent);
			_wait(mCopyFenceValue, pCopyQueue, pCopyFence, mCopyFenceEvent);
			_wait(mComputeFenceValue, pComputeQueue, pComputeFence, mComputeFenceEvent);
		}

		ID3D12Device4* pDevice;
		ID3D12CommandQueue* pGraphicsQueue;
		ID3D12CommandQueue* pCopyQueue;
		ID3D12CommandQueue* pComputeQueue;
		IDXGISwapChain4* pSwapChain;

		uint64 mGFenceValue;
		ID3D12Fence* pGFence;
		HANDLE mGFenceEvent;
		uint64 mCopyFenceValue;
		ID3D12Fence* pCopyFence;
		HANDLE mCopyFenceEvent;
		uint64 mComputeFenceValue;
		ID3D12Fence* pComputeFence;
		HANDLE mComputeFenceEvent;
	};

	IDevice* create(DeviceDesc* pDesc) {
		return new Device(pDesc);
	}

	void destroy(IDevice* pDevice) {
		auto d = (Device*)pDevice;
		delete d;
	}
}