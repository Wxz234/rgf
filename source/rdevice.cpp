#include "rdevice.h"
#include "rpass.h"
#include "descriptor.h"
#include "cmdList.h"

#include "D3D12MemAlloc.h"
#include "third/imgui/imgui.h"
#include "third/imgui/imgui_impl_win32.h"
#include "third/imgui/imgui_impl_dx12.h"

#include <stdexcept>
#include <array>

#define FRAME_COUNT 2

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
		scDesc.BufferCount = FRAME_COUNT;
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

			pdescriptorManager = createDescriptorManager(pDevice);

			for (int i = 0; i < FRAME_COUNT; ++i) {
				mFrameLists[i] = createCmdList(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
			}

			{
				D3D12_DESCRIPTOR_HEAP_DESC desc{};
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				desc.NumDescriptors = FRAME_COUNT;
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pFrameRTVHeap));

				SIZE_T rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pFrameRTVHeap->GetCPUDescriptorHandleForHeapStart();
				for (UINT i = 0; i < FRAME_COUNT; i++)
				{
					mFrameRTVHandle[i] = rtvHandle;
					rtvHandle.ptr += rtvDescriptorSize;

					ID3D12Resource* pBackBuffer = NULL;
					pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
					pDevice->CreateRenderTargetView(pBackBuffer, NULL, mFrameRTVHandle[i]);
					mFrameResource[i] = pBackBuffer;
				}
			}

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.NumDescriptors = 1;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pImguiHeap));

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			ImGui_ImplWin32_Init(pDesc->mHwnd);
			ImGui_ImplDX12_Init(pDevice, FRAME_COUNT,
				DXGI_FORMAT_R8G8B8A8_UNORM, pImguiHeap,
				pImguiHeap->GetCPUDescriptorHandleForHeapStart(),
				pImguiHeap->GetGPUDescriptorHandleForHeapStart());
		}

		~RDevice() {
			_wait(mGFenceValue, pGraphicsQueue, pGFence, mGFenceEvent);
			_wait(mCopyFenceValue, pCopyQueue, pCopyFence, mCopyFenceEvent);
			_wait(mComputeFenceValue, pComputeQueue, pComputeFence, mComputeFenceEvent);

			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			pImguiHeap->Release();

			for (int i = 0; i < FRAME_COUNT; ++i) {
				mFrameResource[i]->Release();
			}

			pFrameRTVHeap->Release();

			for (int i = 0; i < FRAME_COUNT; ++i) {
				removeObject(mFrameLists[i]);
			}

			removeObject(pdescriptorManager);
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

		void* getResourceAllocator() const {
			return pAllocator;
		}

		void* getDescriptorManager() const {
			return pdescriptorManager;
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

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			ImGui::Begin("123");
			ImGui::End();
			ImGui::Render();

			auto frameIndex = pSwapChain->GetCurrentBackBufferIndex();
			mFrameLists[frameIndex]->open(nullptr);

			auto pFrameList = mFrameLists[frameIndex]->getList();
			ID3D12GraphicsCommandList* pGraphicsFrameList;
			pFrameList->QueryInterface(&pGraphicsFrameList);
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = mFrameResource[frameIndex];
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			pGraphicsFrameList->ResourceBarrier(1, &barrier);

			pGraphicsFrameList->OMSetRenderTargets(1, &mFrameRTVHandle[frameIndex], FALSE, NULL);
			pGraphicsFrameList->SetDescriptorHeaps(1, &pImguiHeap);
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pGraphicsFrameList);
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			pGraphicsFrameList->ResourceBarrier(1, &barrier);

			mFrameLists[frameIndex]->close();

			pGraphicsQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&pGraphicsFrameList);

			pSwapChain->Present(1, 0);
			_wait(mGFenceValue, pGraphicsQueue, pGFence, mGFenceEvent);
			_wait(mCopyFenceValue, pCopyQueue, pCopyFence, mCopyFenceEvent);
			_wait(mComputeFenceValue, pComputeQueue, pComputeFence, mComputeFenceEvent);

			pGraphicsFrameList->Release();
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
		descriptorManager* pdescriptorManager;

		ID3D12DescriptorHeap* pImguiHeap;
		std::array<cmdList*, FRAME_COUNT> mFrameLists;
		ID3D12DescriptorHeap* pFrameRTVHeap;
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, FRAME_COUNT> mFrameRTVHandle;
		std::array<ID3D12Resource *, FRAME_COUNT> mFrameResource;
	};

	rdevice* create(rdeviceDesc* pDesc) {
		return new RDevice(pDesc);
	}
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

RGF_API LRESULT _TEST(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}