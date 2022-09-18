#include "d3dx12.h"
#include "rdevice.h"
#include "D3D12MemAlloc.h"
#include <combaseapi.h>
#include <d3d12sdklayers.h>
#include <vector>
#include <stdexcept>

#include "post_vs_shader.h"
#include "post_ps_shader.h"
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

	IDXGISwapChain4* _getSwapChain(ID3D12CommandQueue* pQueue, HWND hwnd, uint32_t w, uint32_t h) {
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

	struct DescriptorManager : public robject {
		DescriptorManager() {}
		DescriptorManager(const DescriptorManager& r) = delete;
		DescriptorManager(DescriptorManager&& r) = delete;
		DescriptorManager& operator=(const DescriptorManager& r) = delete;
		DescriptorManager& operator=(DescriptorManager&& r) = delete;

		~DescriptorManager() {
			if (pDescriptorHeap) {
				pDescriptorHeap->Release();
			}
			if (pVisableDescriptorHeap) {
				pVisableDescriptorHeap->Release();
			}
			pDescriptorHeap = nullptr;
			pVisableDescriptorHeap = nullptr;
		}

		void release() {
			delete this;
		}

		void Init(ID3D12Device* pDevice) {

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

		void AllocStatic(uint32_t &index) {
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

		void AllocDynamic(uint32_t& index) {
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

		void Update() {
			for (auto &state : mState) {
				if (state.IsDynamic) {
					state.IsAssigned = false;
				}
			}
		}

		void BindSRV(uint32_t index, ID3D12Resource* pRes, const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc) {
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			pDevice->CreateShaderResourceView(pRes, pDesc, handle);
			pDevice->CopyDescriptorsSimple(1, pVisableDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		void BindCBV(uint32_t index, const D3D12_CONSTANT_BUFFER_VIEW_DESC* pDesc) {
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			pDevice->CreateConstantBufferView(pDesc, handle);
			pDevice->CopyDescriptorsSimple(1, pVisableDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		void BindUAV(uint32_t index, ID3D12Resource* pRes, ID3D12Resource* pCounterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc) {
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			pDevice->CreateUnorderedAccessView(pRes, pCounterResource, pDesc, handle);
			pDevice->CopyDescriptorsSimple(1, pVisableDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE GetVisableDescriptor(uint32_t index) const {
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pVisableDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			return handle;
		}

		ID3D12Device* pDevice = nullptr;
		ID3D12DescriptorHeap* pDescriptorHeap = nullptr;
		ID3D12DescriptorHeap* pVisableDescriptorHeap = nullptr;

		struct DescriptorState {
			bool IsDynamic = false;
			bool IsAssigned = false;
		};
		std::vector<DescriptorState> mState;

		uint32_t mNumDescriptors = 0;
		uint32_t mDescriptorSize = 0;
	};

	DescriptorManager* createDescriptorManager(ID3D12Device *pDevice) {
		DescriptorManager* d = new DescriptorManager;
		d->Init(pDevice);
		return d;
	}

	struct GBufferResource : public robject {
		GBufferResource() {}
		GBufferResource(const GBufferResource& r) = delete;
		GBufferResource(GBufferResource&& r) = delete;
		GBufferResource& operator=(const GBufferResource& r) = delete;
		GBufferResource& operator=(GBufferResource&& r) = delete;
		~GBufferResource() {
			if (pGBufferA) {
				pGBufferA->Release();
			}
			pGBufferA = nullptr;
		}
		void release() {
			delete this;
		}

		void Init(D3D12MA::Allocator* pAllocator, uint32_t w, uint32_t h) {
			D3D12MA::ALLOCATION_DESC allocDesc{};
			allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

			D3D12_RESOURCE_DESC BufferADesc{};
			BufferADesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			BufferADesc.Width = w;
			BufferADesc.Height = h;
			BufferADesc.DepthOrArraySize = 1;
			BufferADesc.MipLevels = 1;
			BufferADesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			BufferADesc.SampleDesc.Count = 1;

			pAllocator->CreateResource(&allocDesc, &BufferADesc, D3D12_RESOURCE_STATE_COMMON, nullptr, &pGBufferA, IID_NULL, nullptr);
		}
		
		D3D12MA::Allocation* pGBufferA = nullptr;
	};

	GBufferResource* createGBufferResource(D3D12MA::Allocator* pAllocator, uint32_t w, uint32_t h) {
		GBufferResource* g = new GBufferResource;
		g->Init(pAllocator, w, h);
		return g;
	}

	struct Pass : public robject {
		virtual void Init(ID3D12Device* pDevice) = 0;
	};

	struct GBufferPass : public Pass {
		~GBufferPass() {

		}

		void Init(ID3D12Device* pDevice) {
			this->pDevice = pDevice;
		}

		void release() {
			delete this;
		}
		ID3D12Device* pDevice = nullptr;
		ID3D12CommandAllocator* pGBufferAllocator = nullptr;
		ID3D12GraphicsCommandList3* pGBufferList = nullptr;
	};

	GBufferPass* createGBufferPass(ID3D12Device* pDevice) {
		GBufferPass* g = new GBufferPass;
		g->Init(pDevice);
		return g;
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
			//
			//pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pPostProcessAllocator));
			//pDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&pPostProcessList));
			//pPostProcessRoot = getPostprocessRootSignature(pDevice);
			//pPostProcessPipeline = getPostprocessPipeline(pDevice, pPostProcessRoot);

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

		ID3D12Device* getDevice() const {
			return pDevice;
		}

		IDXGISwapChain* getSwapChain() const {
			return pSwapChain;
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
		// PostProcess pass

		// Descriptor Heap
		//DescriptorManager mGBufferDescriptor;
		//GBufferResource mGBufferResource;

		//uint32_t mGBufferASrvIndex;

	};

	rdevice* create(rdeviceDesc* pDesc) {
		return new RDevice(pDesc);
	}
}