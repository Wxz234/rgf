#include "rdevice.h"
#include "D3D12MemAlloc.h"
#include <dxgi1_6.h>
#include <combaseapi.h>
#include <d3d12sdklayers.h>
namespace rgf {

	ID3D12Device4* _getDevice(IDXGIAdapter* pAdapter) {
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

	ID3D12CommandQueue* getQueue(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type) {
		D3D12_COMMAND_QUEUE_DESC queueDesc{};
		queueDesc.Type = type;
		ID3D12CommandQueue* pQueue;
		pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pQueue));
		return pQueue;
	}

	IDXGISwapChain4* getSwapChain(ID3D12CommandQueue* pQueue, HWND hwnd, uint32_t w, uint32_t h) {
		IDXGIFactory7* pFactory;
#ifdef _DEBUG
		CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&pFactory));
#else
		CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory));
#endif

		DXGI_SWAP_CHAIN_DESC1 scDesc{};
		scDesc.BufferCount = 2;
		scDesc.Width = w;
		scDesc.Height = h;
		scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

	IDXGIAdapter* getAdapter() {
		IDXGIFactory7* pFactory;
#ifdef _DEBUG
		CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&pFactory));
#else
		CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory));
#endif
		IDXGIAdapter* pAdapter;
		pFactory->EnumAdapters(0, &pAdapter);
		pFactory->Release();
		return pAdapter;
	}


	struct RDevice : public rdevice {
		RDevice(rdeviceDesc* pDesc) {
			pAdapter = getAdapter();
			pDevice = _getDevice(pAdapter);
			pGraphicsQueue = getQueue(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
			pCopyQueue = getQueue(pDevice, D3D12_COMMAND_LIST_TYPE_COPY);
			pComputeQueue = getQueue(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
			pSwapChain = getSwapChain(pGraphicsQueue, pDesc->mHwnd, pDesc->mWidth, pDesc->mHeight);
			mFenceValue = 1;
			pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));
			mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			//
			D3D12MA::ALLOCATOR_DESC allocatorDesc{};
			allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
			allocatorDesc.pAdapter = pAdapter;
			allocatorDesc.pAllocationCallbacks = nullptr;
			allocatorDesc.pDevice = pDevice;
			allocatorDesc.PreferredBlockSize = 0;
			D3D12MA::CreateAllocator(&allocatorDesc, &pAllocator);
		}

		~RDevice() {
			Wait();
			pAllocator->Release();
			CloseHandle(mFenceEvent);
			pFence->Release();
			pSwapChain->Release();
			pComputeQueue->Release();
			pCopyQueue->Release();
			pGraphicsQueue->Release();
			pDevice->Release();
			pAdapter->Release();
		}

		void release() {
			delete this;
		}

		ID3D12Device4* getDevice() const {
			return pDevice;
		}

		void frame() {
			pSwapChain->Present(1, 0);
			Wait();
		}

		void Wait() {
			const UINT64 fence = mFenceValue;
			pGraphicsQueue->Signal(pFence, fence);
			mFenceValue++;

			// Wait until the previous frame is finished.
			if (pFence->GetCompletedValue() < fence)
			{
				pFence->SetEventOnCompletion(fence, mFenceEvent);
				WaitForSingleObject(mFenceEvent, INFINITE);
			}
		}

		IDXGIAdapter* pAdapter;
		ID3D12Device4* pDevice;
		ID3D12CommandQueue* pGraphicsQueue;
		ID3D12CommandQueue* pCopyQueue;
		ID3D12CommandQueue* pComputeQueue;
		IDXGISwapChain4* pSwapChain;

		UINT64 mFenceValue;
		ID3D12Fence* pFence;
		HANDLE mFenceEvent;
		//
		D3D12MA::Allocator* pAllocator;
	};

	rdevice* create(rdeviceDesc* pDesc) {
		return new RDevice(pDesc);
	}
}