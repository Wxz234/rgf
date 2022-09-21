#include "rdevice.h"
#include "rpass.h"
#include "D3D12MemAlloc.h"

#include <stdexcept>

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

	IDXGISwapChain4* _getSwapChain(ID3D12CommandQueue* pQueue, HWND hwnd, uint32 w, uint32 h) {
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

	bool checkFeature(ID3D12Device* pDevice) {
		if (D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{ D3D_SHADER_MODEL_6_6 }; FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
			|| (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_6))
		{
			return false;
		}
		if (D3D12_FEATURE_DATA_D3D12_OPTIONS featureSupport{}; FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureSupport, sizeof(featureSupport))) 
			|| D3D12_RESOURCE_BINDING_TIER_3 != featureSupport.ResourceBindingTier) 
		{
			return false;
		}

		return true;
	}

	struct RDevice : public rdevice {
		RDevice(rdeviceDesc* pDesc) {
			pAdapter = getAdapter();
			pDevice = _getDevice(pAdapter);

			auto isSupported = checkFeature(pDevice);
			if (!isSupported) {
				throw std::runtime_error("Your hardware does not support this feature.");
			}

			pGraphicsQueue = getQueue(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
			pCopyQueue = getQueue(pDevice, D3D12_COMMAND_LIST_TYPE_COPY);
			pComputeQueue = getQueue(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
			pSwapChain = _getSwapChain(pGraphicsQueue, pDesc->mHwnd, pDesc->mWidth, pDesc->mHeight);
			mGFenceValue = 1;
			pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pGFence));
			mGFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			mCopyFenceValue = 1;
			pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pCopyFence));
			mCopyFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			mComputeFenceValue = 1;
			pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pComputeFence));
			mComputeFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			
			D3D12MA::ALLOCATOR_DESC allocatorDesc{};
			allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
			allocatorDesc.pAdapter = pAdapter;
			allocatorDesc.pAllocationCallbacks = nullptr;
			allocatorDesc.pDevice = pDevice;
			allocatorDesc.PreferredBlockSize = 0;
			D3D12MA::CreateAllocator(&allocatorDesc, &pAllocator);
		}

		~RDevice() {
			_wait(mGFenceValue, pGraphicsQueue, pGFence, mGFenceEvent);
			_wait(mCopyFenceValue, pCopyQueue, pCopyFence, mCopyFenceEvent);
			_wait(mComputeFenceValue, pComputeQueue, pComputeFence, mComputeFenceEvent);

			pAllocator->Release();
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
			pAdapter->Release();
		}

		void release() {
			delete this;
		}

		ID3D12Device* getDevice() const {
			return pDevice;
		}

		IDXGISwapChain* getSwapChain() const {
			return pSwapChain;
		}

		void executePass(rpass* pPass) {
			auto listType = pPass->getList()->GetType();
			if (listType == D3D12_COMMAND_LIST_TYPE_DIRECT) {
				ID3D12CommandList* pLists[] = { pPass->getList() };
				pGraphicsQueue->ExecuteCommandLists(1, pLists);
			}
			else if (listType == D3D12_COMMAND_LIST_TYPE_COPY) {
				ID3D12CommandList* pLists[] = { pPass->getList() };
				pCopyQueue->ExecuteCommandLists(1, pLists);
			}
			else if (listType == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
				ID3D12CommandList* pLists[] = { pPass->getList() };
				pComputeQueue->ExecuteCommandLists(1, pLists);
			}
		}

		void frame() {
			pSwapChain->Present(1, 0);
			_wait(mGFenceValue, pGraphicsQueue, pGFence, mGFenceEvent);
			_wait(mCopyFenceValue, pCopyQueue, pCopyFence, mCopyFenceEvent);
			_wait(mComputeFenceValue, pComputeQueue, pComputeFence, mComputeFenceEvent);
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

		IDXGIAdapter* pAdapter;
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

		D3D12MA::Allocator* pAllocator;
	};

	rdevice* create(rdeviceDesc* pDesc) {
		return new RDevice(pDesc);
	}
}