#include "d3dx12.h"
#include "rdevice.h"
#include "D3D12MemAlloc.h"
#include <combaseapi.h>
#include <d3d12sdklayers.h>
#include <vector>
#include <stdexcept>
#include <DirectXMath.h>

#include "post_vs_shader.h"
#include "post_ps_shader.h"
#include "gbuffer_vs.h"
#include "gbuffer_ps.h"
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
		DescriptorManager(ID3D12Device* pDevice) {
			Init(pDevice);
		}

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

		void allocStatic(uint32_t &index) {
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

		void allocDynamic(uint32_t& index) {
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

		void update() {
			for (auto &state : mState) {
				if (state.IsDynamic) {
					state.IsAssigned = false;
				}
			}
		}

		void bindSRV(uint32_t index, ID3D12Resource* pRes, const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc) {
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			pDevice->CreateShaderResourceView(pRes, pDesc, handle);
			pDevice->CopyDescriptorsSimple(1, pVisableDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		void bindCBV(uint32_t index, const D3D12_CONSTANT_BUFFER_VIEW_DESC* pDesc) {
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			pDevice->CreateConstantBufferView(pDesc, handle);
			pDevice->CopyDescriptorsSimple(1, pVisableDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		void bindUAV(uint32_t index, ID3D12Resource* pRes, ID3D12Resource* pCounterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc) {
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
			pDevice->CreateUnorderedAccessView(pRes, pCounterResource, pDesc, handle);
			pDevice->CopyDescriptorsSimple(1, pVisableDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE getVisableDescriptor(uint32_t index) const {
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
	private:
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
	};

	DescriptorManager* createDescriptorManager(ID3D12Device *pDevice) {
		DescriptorManager* d = new DescriptorManager(pDevice);
		return d;
	}

	struct GBufferResource : public robject {
		GBufferResource(D3D12MA::Allocator* pAllocator, uint32_t w, uint32_t h) {
			Init(pAllocator, w, h);
		}

		~GBufferResource() {
			pGBufferLight->GetResource()->Unmap(0, nullptr);
			pGBufferMatrix->GetResource()->Unmap(0, nullptr);
			pGBufferA->Release();
			pGBufferMatrix->Release();
			pGBufferLight->Release();
		}

		void release() {
			delete this;
		}

		D3D12MA::Allocation* pGBufferMatrix = nullptr;
		D3D12MA::Allocation* pGBufferLight = nullptr;
		D3D12MA::Allocation* pGBufferA = nullptr;

		void* pGBufferMatrixData;
		void* pGBufferLightData;
	private:

		struct GBufferMatrix{
			DirectX::XMFLOAT4X4 MVP;
			DirectX::XMFLOAT4X4 WorldInvTranspose;
		};

		struct GBufferLight {
			DirectX::XMFLOAT4 DirectionAndIntensity;
		};

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
			CD3DX12_RESOURCE_DESC MatrixDesc = CD3DX12_RESOURCE_DESC::Buffer(_getConstantBufferSize(sizeof(GBufferMatrix)));
			allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
			pAllocator->CreateResource(&allocDesc, &MatrixDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, &pGBufferMatrix, IID_NULL, nullptr);
			CD3DX12_RESOURCE_DESC LightDesc = CD3DX12_RESOURCE_DESC::Buffer(_getConstantBufferSize(sizeof(GBufferLight)));
			pAllocator->CreateResource(&allocDesc, &LightDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, &pGBufferLight, IID_NULL, nullptr);

			pGBufferMatrix->GetResource()->Map(0, 0, &pGBufferMatrixData);
			pGBufferLight->GetResource()->Map(0, 0, &pGBufferLightData);
		}

		uint64_t _getConstantBufferSize(uint64_t size) {
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

	GBufferResource* createGBufferResource(D3D12MA::Allocator* pAllocator, uint32_t w, uint32_t h) {
		GBufferResource* g = new GBufferResource(pAllocator, w, h);
		return g;
	}

	struct Pass : public robject {
		virtual void reset() = 0;
		virtual void close() = 0;
	};

	struct GBufferPass : public Pass {

		GBufferPass(ID3D12Device4* pDevice, D3D12MA::Allocator* pAllocator, DescriptorManager* pManager, uint32_t w, uint32_t h) {
			_Init(pDevice);
			_BindDescriptorManager(pManager);
			this->pAllocator = pAllocator;
			pGBufferResource = createGBufferResource(pAllocator, w, h);
		}

		~GBufferPass() {
			removeObject(pGBufferResource);
			pGBufferAllocator->Release();
			pGBufferList->Release();
			pRootSignature->Release();
			pPSO->Release();
		}

		void reset() {
			pGBufferAllocator->Reset();
			pGBufferList->Reset(pGBufferAllocator, nullptr);
		}

		void close() {
			pGBufferList->Close();
		}

		void release() {
			delete this;
		}

		D3D12MA::Allocator* pAllocator;
		DescriptorManager* pManager;
		ID3D12Device* pDevice;
		ID3D12CommandAllocator* pGBufferAllocator;
		ID3D12GraphicsCommandList3* pGBufferList;
		ID3D12RootSignature* pRootSignature;
		ID3D12PipelineState* pPSO;

		GBufferResource* pGBufferResource;

		uint32_t mMatrixIndex;
		uint32_t mLightIndex;
	private:
		void _Init(ID3D12Device4* pDevice) {
			this->pDevice = pDevice;
			pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pGBufferAllocator));
			pDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&pGBufferList));
			pGBufferList->SetName(L"GBufferList");

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
		}

		void _BindDescriptorManager(DescriptorManager* pManager) {
			this->pManager = pManager;
			pManager->allocStatic(mMatrixIndex);
			pManager->allocStatic(mLightIndex);
		}
	};

	GBufferPass* createGBufferPass(ID3D12Device4* pDevice, D3D12MA::Allocator* pAllocator, DescriptorManager* pManager, uint32_t w, uint32_t h) {
		GBufferPass* g = new GBufferPass(pDevice, pAllocator, pManager, w, h);
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

			mLightDirAndIntensity = DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);

			pGraphicsQueue = getQueue(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
			pCopyQueue = getQueue(pDevice, D3D12_COMMAND_LIST_TYPE_COPY);
			pComputeQueue = getQueue(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
			pSwapChain = _getSwapChain(pGraphicsQueue, pDesc->mHwnd, pDesc->mWidth, pDesc->mHeight);
			mFenceValue = 1;
			pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));
			mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			
			D3D12MA::ALLOCATOR_DESC allocatorDesc{};
			allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
			allocatorDesc.pAdapter = pAdapter;
			allocatorDesc.pAllocationCallbacks = nullptr;
			allocatorDesc.pDevice = pDevice;
			allocatorDesc.PreferredBlockSize = 0;
			D3D12MA::CreateAllocator(&allocatorDesc, &pAllocator);
			
			pManager = createDescriptorManager(pDevice);
			
			pGBufferPass = createGBufferPass(pDevice, pAllocator, pManager, pDesc->mWidth, pDesc->mHeight);
			mLists.push_back(pGBufferPass->pGBufferList);
		}

		~RDevice() {
			_wait();

			removeObject(pGBufferPass);
			removeObject(pManager);
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

		void setSkyLightDirection(float x, float y, float z) {
			mLightDirAndIntensity = DirectX::XMFLOAT4(x, y, z, mLightDirAndIntensity.z);
		}

		void setSkyLightIntensity(float intensity) {
			mLightDirAndIntensity = DirectX::XMFLOAT4(mLightDirAndIntensity.x, mLightDirAndIntensity.y, mLightDirAndIntensity.z, intensity);
		}

		void frame() {
			pGBufferPass->reset();
			pGBufferPass->close();
			
			_submit();
			pSwapChain->Present(1, 0);
			_wait();
		}

		void _wait() {
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

		void _submit() {
			pGraphicsQueue->ExecuteCommandLists(mLists.size(), mLists.data());
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

		std::vector<ID3D12CommandList*> mLists;
		D3D12MA::Allocator* pAllocator;

		DescriptorManager* pManager;

		GBufferPass* pGBufferPass;

		DirectX::XMFLOAT4 mLightDirAndIntensity;
	};

	rdevice* create(rdeviceDesc* pDesc) {
		return new RDevice(pDesc);
	}
}