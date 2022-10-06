#include "device.h"
#include "rendertarget.h"
#include "cmdList.h"
#include "descriptor.h"
#include "shader/vs/device.h"
#include "shader/ps/device.h"

#include "D3D12MemAlloc.h"
#include "third/imgui/imgui.h"
#include "third/imgui/imgui_internal.h"
#include "third/imgui/imgui_impl_win32.h"
#include "third/imgui/imgui_impl_dx12.h"

#include <stdexcept>
#include <array>

#define FRAME_COUNT 2
#define FRAME_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

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

	D3D12MA::Allocator* getAllocator(IDXGIAdapter* pAdapter, ID3D12Device* pDevice) {
		D3D12MA::ALLOCATOR_DESC allocatorDesc{};
		allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
		allocatorDesc.pAdapter = pAdapter;
		allocatorDesc.pAllocationCallbacks = nullptr;
		allocatorDesc.pDevice = pDevice;
		allocatorDesc.PreferredBlockSize = 0;

		D3D12MA::Allocator* pAllocator;
		D3D12MA::CreateAllocator(&allocatorDesc, &pAllocator);

		return pAllocator;
	}

	struct RDevice : public device {
		RDevice(device_desc* pDesc) {
			pAdapter = getAdapter();
			pDevice = _getDevice(pAdapter);

			auto isSupported = checkFeature(pDevice);
			if (!isSupported) {
				throw std::runtime_error("Your hardware does not support this feature.");
			}

			mWidth = pDesc->mWidth;
			mHeight= pDesc->mHeight;

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
			
			pAllocator = getAllocator(pAdapter, pDevice);

			_createCmdList();
			_createSwapChainRes();
			_createRTV();
			_createDescriptorManager();
			_createPipeline();
			_createRT();
			_createImgui(pDesc->mHwnd);
		}

		~RDevice() {
			_wait(mGFenceValue, pGraphicsQueue, pGFence, mGFenceEvent);
			_wait(mCopyFenceValue, pCopyQueue, pCopyFence, mCopyFenceEvent);
			_wait(mComputeFenceValue, pComputeQueue, pComputeFence, mComputeFenceEvent);

			_destroyImgui();
			_destroyRT();
			_destroyPipeline();
			_destroyDescriptorManager();
			_destroyRTV();

			_destroySwapChainRes();
			_destroyCmdList();
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
			return pDescriptorManager;
		}

		uint32 getFrameIndex() const {
			return pSwapChain->GetCurrentBackBufferIndex();
		}

		void drawText(const std::string_view text, float fontSize, float x, float y, uint32 color) {
			if (!pImguiContext->bIsCalled) {
				ImGui_ImplDX12_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();

				ImGui::SetNextWindowPos(ImVec2(0, 0));
				ImGui::SetNextWindowSize(ImVec2(mWidth, mHeight));
				ImGui::Begin("Imgui", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);

				pImguiContext->bIsCalled = true;
			}
			ImGuiWindowFlags_NoDecoration;
			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			DrawList->AddText(nullptr, fontSize, ImVec2(x, y), color, text.data());
		}

		void setRenderTarget(rendertarget* pRT) {

		}

		void frame() {
			_updateState();
			_beginFrame();
			_endFrame();
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

		void _updateState() {
			if (pImguiContext->bIsCalled) {
				ImGui::End();
				ImGui::Render();
			}

			pDescriptorManager->update();
		}

		
		void _beginFrame() {
			auto frameIndex = getFrameIndex();

			mFrameGraphicsList[frameIndex]->open(nullptr);

			auto pGraphicsFrameList = mFrameGraphicsList[frameIndex]->getList();
			//pGraphicsFrameList->SetGraphicsRootSignature(pRoot);

			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = mSwapChainRes[frameIndex];
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			pGraphicsFrameList->ResourceBarrier(1, &barrier);
			pGraphicsFrameList->OMSetRenderTargets(1, &mFrameRTVHandle[frameIndex], FALSE, NULL);
		}

		void _endFrame() {
			auto frameIndex = pSwapChain->GetCurrentBackBufferIndex();

			auto pGraphicsFrameList = mFrameGraphicsList[frameIndex]->getList();
			if (pImguiContext->bIsCalled) {
				pGraphicsFrameList->SetDescriptorHeaps(1, &pImguiContext->pImguiHeap);
				ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pGraphicsFrameList);

				pImguiContext->bIsCalled = false;
			}

			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = mSwapChainRes[frameIndex];
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			pGraphicsFrameList->ResourceBarrier(1, &barrier);

			mFrameGraphicsList[frameIndex]->close();
			
			ID3D12CommandList* pLists[1] = { 
				mFrameGraphicsList[frameIndex]->getList(),
			};

			pGraphicsQueue->ExecuteCommandLists(1, pLists);

			pSwapChain->Present(1, 0);
			_wait(mGFenceValue, pGraphicsQueue, pGFence, mGFenceEvent);
			_wait(mCopyFenceValue, pCopyQueue, pCopyFence, mCopyFenceEvent);
			_wait(mComputeFenceValue, pComputeQueue, pComputeFence, mComputeFenceEvent);
		}

		void _createCmdList() {
			for (auto &list : mFrameGraphicsList) {
				list = createCmdList(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
			}
		}

		void _destroyCmdList() {
			for (auto &list : mFrameGraphicsList) {
				removeObject(list);
			}
		}

		void _createSwapChainRes() {
			for (UINT i = 0; i < FRAME_COUNT; ++i)
			{
				ID3D12Resource* pBackBuffer = NULL;
				pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
				mSwapChainRes[i] = pBackBuffer;
			}
		}

		void _destroySwapChainRes() {
			for (auto res : mSwapChainRes) {
				res->Release();
			}
		}

		void _createImgui(HWND hwnd) {
			pImguiContext = new ImguiContext;
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.NumDescriptors = 1;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pImguiContext->pImguiHeap));

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			ImGui_ImplWin32_Init(hwnd);
			ImGui_ImplDX12_Init(pDevice, FRAME_COUNT, FRAME_FORMAT, pDescriptorManager->pDescriptorHeap, pImguiContext->pImguiHeap->GetCPUDescriptorHandleForHeapStart(), pImguiContext->pImguiHeap->GetGPUDescriptorHandleForHeapStart());
		}

		void _destroyImgui() {
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			pImguiContext->pImguiHeap->Release();
			delete pImguiContext;
		}

		void _createRTV() {

			D3D12_DESCRIPTOR_HEAP_DESC desc{};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			desc.NumDescriptors = FRAME_COUNT;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pFrameRTVHeap));

			SIZE_T rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pFrameRTVHeap->GetCPUDescriptorHandleForHeapStart();

			for (UINT i = 0; i < FRAME_COUNT; ++i)
			{
				mFrameRTVHandle[i] = rtvHandle;
				rtvHandle.ptr += rtvDescriptorSize;
				pDevice->CreateRenderTargetView(mSwapChainRes[i], NULL, mFrameRTVHandle[i]);
			}
		}

		void _destroyRTV() {
			pFrameRTVHeap->Release();
		}

		void _createDescriptorManager() {
			pDescriptorManager = createDescriptorManager(pDevice);
		}

		void _destroyDescriptorManager() {
			removeObject(pDescriptorManager);
		}

		void _createPipeline() {
			pDevice->CreateRootSignature(0, deviceVS, sizeof(deviceVS), IID_PPV_ARGS(&pRoot));

			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
            psoDesc.pRootSignature = pRoot;
            psoDesc.VS = { deviceVS, sizeof(deviceVS) };
            psoDesc.PS = { devicePS, sizeof(devicePS) };
            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            psoDesc.SampleMask = 0xffffffff;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.SampleDesc.Count = 1;
            pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPSO));
		}

		void _destroyPipeline() {
			pRoot->Release();
			pPSO->Release();
		}

		void _createRT() {
			rendertarget_desc rt_desc{};
			rt_desc.mWidth = mWidth;
			rt_desc.mHeight = mHeight;
			rt_desc.pDevice = this;
			rt_desc.mFormat = FRAME_FORMAT;
			pSelfRT = create(&rt_desc);
		}

		void _destroyRT() {
			removeObject(pSelfRT);
		}

		uint32 mWidth;
		uint32 mHeight;

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

		std::array<cmdList*, FRAME_COUNT> mFrameGraphicsList;

		std::array<ID3D12Resource*, FRAME_COUNT> mSwapChainRes;

		ID3D12DescriptorHeap* pFrameRTVHeap;
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, FRAME_COUNT> mFrameRTVHandle;

		struct ImguiContext {
			ID3D12DescriptorHeap* pImguiHeap = nullptr;
			bool bIsCalled = false;
		};
		ImguiContext* pImguiContext;

		descriptorManager* pDescriptorManager;

		ID3D12PipelineState* pPSO;
		ID3D12RootSignature* pRoot;

		rendertarget* pRT;
		rendertarget* pSelfRT;
	};

	device* create(device_desc* pDesc) {
		return new RDevice(pDesc);
	}
}