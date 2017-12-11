#include "d3d11_blend.h"
#include "d3d11_device.h"

namespace dxvk {
  
  D3D11BlendState::D3D11BlendState(
          D3D11Device*      device,
    const D3D11_BLEND_DESC& desc)
  : m_device(device), m_desc(desc) {
    // If Independent Blend is disabled, we must ignore the
    // blend modes for render target 1 to 7. In Vulkan, all
    // blend modes need to be identical in that case.
    for (uint32_t i = 0; i < m_blendModes.size(); i++) {
      m_blendModes.at(i) = DecodeBlendMode(
        desc.IndependentBlendEnable
          ? desc.RenderTarget[i]
          : desc.RenderTarget[0]);
    }
    
    // Multisample state is part of the blend state in D3D11
    m_msState.sampleMask            = 0; // Set during bind
    m_msState.enableAlphaToCoverage = desc.AlphaToCoverageEnable;
    m_msState.enableAlphaToOne      = VK_FALSE;
    m_msState.enableSampleShading   = VK_FALSE;
    m_msState.minSampleShading      = 0.0f;
  }
  
  
  D3D11BlendState::~D3D11BlendState() {
    
  }
  
  
  HRESULT D3D11BlendState::QueryInterface(REFIID riid, void** ppvObject) {
    COM_QUERY_IFACE(riid, ppvObject, IUnknown);
    COM_QUERY_IFACE(riid, ppvObject, ID3D11DeviceChild);
    COM_QUERY_IFACE(riid, ppvObject, ID3D11BlendState);
    
    Logger::warn("D3D11BlendState::QueryInterface: Unknown interface query");
    return E_NOINTERFACE;
  }
  
  
  void D3D11BlendState::GetDevice(ID3D11Device** ppDevice) {
    *ppDevice = ref(m_device);
  }
  
  
  void D3D11BlendState::GetDesc(D3D11_BLEND_DESC* pDesc) {
    *pDesc = m_desc;
  }
  
  
  void D3D11BlendState::BindToContext(
    const Rc<DxvkContext>&  ctx,
          uint32_t          sampleMask) const {
    // We handled Independent Blend during object creation
    // already, so if it is disabled, all elements in the
    // blend mode array will be identical
    for (uint32_t i = 0; i < m_blendModes.size(); i++)
      ctx->setBlendMode(i, m_blendModes.at(i));
    
    // The sample mask is dynamic state in D3D11
    DxvkMultisampleState msState = m_msState;
    msState.sampleMask = sampleMask;
    ctx->setMultisampleState(msState);
  }
  
  
  DxvkBlendMode D3D11BlendState::DecodeBlendMode(
    const D3D11_RENDER_TARGET_BLEND_DESC& blendDesc) {
    DxvkBlendMode mode;
    mode.enableBlending   = blendDesc.BlendEnable;
    mode.colorSrcFactor   = DecodeBlendFactor(blendDesc.SrcBlend, false);
    mode.colorDstFactor   = DecodeBlendFactor(blendDesc.DestBlend, false);
    mode.colorBlendOp     = DecodeBlendOp(blendDesc.BlendOp);
    mode.alphaSrcFactor   = DecodeBlendFactor(blendDesc.SrcBlendAlpha, true);
    mode.alphaDstFactor   = DecodeBlendFactor(blendDesc.DestBlendAlpha, true);
    mode.alphaBlendOp     = DecodeBlendOp(blendDesc.BlendOpAlpha);
    // TODO find out if D3D11 wants us to apply the write mask if blending
    // is disabled as well. This is standard behaviour in Vulkan.
    mode.writeMask        = blendDesc.RenderTargetWriteMask;
    return mode;
  }
  
  
  VkBlendFactor D3D11BlendState::DecodeBlendFactor(D3D11_BLEND blendFactor, bool isAlpha) {
    switch (blendFactor) {
      case D3D11_BLEND_ZERO:              return VK_BLEND_FACTOR_ZERO;
      case D3D11_BLEND_ONE:               return VK_BLEND_FACTOR_ONE;
      case D3D11_BLEND_SRC_COLOR:         return VK_BLEND_FACTOR_SRC_COLOR;
      case D3D11_BLEND_INV_SRC_COLOR:     return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
      case D3D11_BLEND_SRC_ALPHA:         return VK_BLEND_FACTOR_SRC_ALPHA;
      case D3D11_BLEND_INV_SRC_ALPHA:     return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      case D3D11_BLEND_DEST_ALPHA:        return VK_BLEND_FACTOR_DST_ALPHA;
      case D3D11_BLEND_INV_DEST_ALPHA:    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
      case D3D11_BLEND_DEST_COLOR:        return VK_BLEND_FACTOR_DST_COLOR;
      case D3D11_BLEND_INV_DEST_COLOR:    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
      case D3D11_BLEND_SRC_ALPHA_SAT:     return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
      case D3D11_BLEND_BLEND_FACTOR:      return isAlpha ? VK_BLEND_FACTOR_CONSTANT_ALPHA : VK_BLEND_FACTOR_CONSTANT_COLOR;
      case D3D11_BLEND_INV_BLEND_FACTOR:  return isAlpha ? VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA : VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
      case D3D11_BLEND_SRC1_COLOR:        return VK_BLEND_FACTOR_SRC1_COLOR;
      case D3D11_BLEND_INV_SRC1_COLOR:    return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
      case D3D11_BLEND_SRC1_ALPHA:        return VK_BLEND_FACTOR_SRC1_ALPHA;
      case D3D11_BLEND_INV_SRC1_ALPHA:    return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    }
    
    Logger::err(str::format("D3D11: Invalid blend factor: ", blendFactor));
    return VK_BLEND_FACTOR_ZERO;
  }
  
  
  VkBlendOp D3D11BlendState::DecodeBlendOp(D3D11_BLEND_OP blendOp) {
    switch (blendOp) {
      case D3D11_BLEND_OP_ADD:            return VK_BLEND_OP_ADD;
      case D3D11_BLEND_OP_SUBTRACT:       return VK_BLEND_OP_SUBTRACT;
      case D3D11_BLEND_OP_REV_SUBTRACT:   return VK_BLEND_OP_REVERSE_SUBTRACT;
      case D3D11_BLEND_OP_MIN:            return VK_BLEND_OP_MIN;
      case D3D11_BLEND_OP_MAX:            return VK_BLEND_OP_MAX;
    }
    
    Logger::err(str::format("D3D11: Invalid blend op: ", blendOp));
    return VK_BLEND_OP_ADD;
  }
  
}
