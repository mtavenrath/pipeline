// Copyright NVIDIA Corporation 2011
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include "Viewer.h"
#include <dp/DP.h>
#include <dp/math/Vecnt.h>
#include <dp/sg/core/CoreTypes.h>
#include <dp/sg/core/EffectData.h>
#include <dp/sg/core/FrustumCamera.h>
#include <dp/fx/EffectLibrary.h>
#include <dp/sg/gl/TextureGL.h>
#include "SceneRendererPipeline.h"

using namespace dp::fx;
using std::string;
using std::vector;

dp::util::SmartPtr<SceneRendererPipeline::MonoViewStateProvider> SceneRendererPipeline::MonoViewStateProvider::create()
{
  return new SceneRendererPipeline::MonoViewStateProvider();
}

// This does nothing except for providing the setSceneRenderer() override which 
// allows to set the scene renderer before any OpenGL resources have been allocated.
SceneRendererPipeline::SceneRendererPipeline()
: m_highlighting( false )
, m_backdropEnabled( GetApp()->isBackdropEnabled() )
, m_tonemapperEnabled( false ) 
, m_tonemapperValuesChanged( false )
{
  m_monoViewStateProvider = MonoViewStateProvider::create();
  
  // Tonemapper GUI neutral defaults:
  m_tonemapperValues.gamma          = 1.0f;
  m_tonemapperValues.whitePoint     = 1.0f;
  m_tonemapperValues.brightness     = 1.0f;
  m_tonemapperValues.saturation     = 1.0f;
  m_tonemapperValues.crushBlacks    = 0.0f;
  m_tonemapperValues.burnHighlights = 1.0f;
}

SceneRendererPipeline::~SceneRendererPipeline()
{
  // The ViewerRendererWidget destructor makes the context current to allow cleanup of OpenGL resources!
  m_tonemapper.reset();
  m_environmentBackdrop.reset();
  m_rendererHighlight.reset();
  m_rendererStencilToColor.reset();
  m_sceneRendererHighlight.reset();
  m_highlightFBO.reset();
}

// This is called from initializeGL().
bool SceneRendererPipeline::init(const dp::gl::SmartRenderContext &renderContext,
                                 const dp::gl::SmartRenderTarget  &renderTarget)
{
  m_renderTarget = renderTarget;

  vector<string> searchPaths;
  searchPaths.push_back( dp::home() + "/apps/Viewer/res" );
  dp::fx::EffectLibrary::instance()->loadEffects( "viewerEffects.xml", searchPaths );

  // Create an FBO with 2D texture color attachment and depth stencil render buffers.
  // This one remains monoscopic, SceneRendererPipeline::doRender() works per eye.
  m_highlightFBO = dp::gl::RenderTargetFBO::create( renderContext );

  // Set the defaults for the render pass.
  // This clear color actually doesn't take effect when using a SceneRenderer. The scene background color has precedence.
  m_highlightFBO->setClearColor(0.0f, 0.0f, 0.0f, 0.0f); 
  m_highlightFBO->setClearDepth(1.0);
  m_highlightFBO->setClearStencil(0);

  // Make the OpenGL context on the renderContext current. It's needed for the create() operations.
  dp::gl::RenderContextStack rcglstack;
  rcglstack.push(renderContext);

  // Render to 2D texture.
  m_highlightFBO->setAttachment(dp::gl::RenderTargetFBO::COLOR_ATTACHMENT0,
                                dp::gl::Texture2D::create(GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE));
  // Depth and Stencil are Renderbuffers.
  dp::gl::SmartRenderbuffer depthStencil(dp::gl::Renderbuffer::create(GL_DEPTH24_STENCIL8)); // Shared depth stencil buffer between the tonemap and hightlight FBOs.
  m_highlightFBO->setAttachment(dp::gl::RenderTargetFBO::DEPTH_ATTACHMENT,   depthStencil);
  m_highlightFBO->setAttachment(dp::gl::RenderTargetFBO::STENCIL_ATTACHMENT, depthStencil);

  std::vector<GLenum> drawBuffers;
  drawBuffers.push_back(dp::gl::RenderTargetFBO::COLOR_ATTACHMENT0);
  m_highlightFBO->setDrawBuffers(drawBuffers);

  rcglstack.pop();

  // If there hasn't been a setSceneRender() we cannot reuse the m_sceneRenderer for the highlight rasterization.
  // Create the SceneRenderer used to render the highlighted objects of the scene into the highlightFBO stencil buffer.
  if (!m_sceneRendererHighlight)
  {
    Viewer * viewer = GetApp();
    m_sceneRendererHighlight = dp::sg::renderer::rix::gl::SceneRenderer::create( viewer->getRenderEngine().c_str(),
                                                                                 viewer->getShaderManagerType(),
                                                                                 viewer->getCullingMode(),
                                                                                 dp::sg::renderer::rix::gl::TM_SORTED_BLENDED );
  }

  if ( GetApp()->isTonemapperEnabled() )
  {
    initTonemapper();
  }
  initBackdrop();

  // Create a full screen quad renderer for the stencil buffer to color attachment migration.
  m_rendererStencilToColor = dp::sg::renderer::rix::gl::FSQRenderer::create(m_highlightFBO);
  m_rendererStencilToColor->setEffect( dp::sg::core::EffectData::create( EffectLibrary::instance()->getEffectSpec( std::string("stencilToColor") ) ) );

  // Create a full screen quad renderer for the final texture to framebuffer operation rendering the highlight outline.
  m_rendererHighlight = dp::sg::renderer::rix::gl::FSQRenderer::create(renderTarget);
  m_rendererHighlight->setEffect( dp::sg::core::EffectData::create( EffectLibrary::instance()->getEffectSpec( std::string("highlight") ) ) );

  // m_rendererHighlight uses the previously rendered texture rectangle as input for the shader.
  const dp::gl::RenderTargetFBO::SmartAttachment &attachment = m_highlightFBO->getAttachment(dp::gl::RenderTargetFBO::COLOR_ATTACHMENT0);
  const dp::gl::RenderTargetFBO::SmartAttachmentTexture &texAtt = dp::util::smart_cast<dp::gl::RenderTargetFBO::AttachmentTexture>(attachment);
  if (texAtt)
  {
    const dp::sg::gl::TextureGLSharedPtr texGL = dp::sg::gl::TextureGL::create( texAtt->getTexture() );
    dp::sg::core::SamplerSharedPtr sampler = dp::sg::core::Sampler::create( texGL );
    sampler->setWrapModes( dp::sg::core::TWM_CLAMP_TO_EDGE, dp::sg::core::TWM_CLAMP_TO_EDGE, dp::sg::core::TWM_CLAMP_TO_EDGE );
    sampler->setMagFilterMode( dp::sg::core::TFM_MAG_NEAREST );
    sampler->setMinFilterMode( dp::sg::core::TFM_MIN_NEAREST );

    m_rendererHighlight->setSamplerByName( "selection", sampler );
  }

  return true;
}

// Mind, this is called for left and right eye independently.
void SceneRendererPipeline::doRender(dp::sg::ui::ViewStateSharedPtr const& viewState, dp::ui::SmartRenderTarget const& renderTarget)
{
  if ( m_tonemapperEnabled )
  {
    doRenderTonemap( viewState, renderTarget );
  }
  else
  {
    doRenderStandard( viewState, renderTarget );
  }
  if (m_highlighting)
  {
    doRenderHighlight( viewState, renderTarget );
  }
}

void SceneRendererPipeline::doRenderBackdrop(dp::sg::ui::ViewStateSharedPtr const& viewState)
{
  // must not be called if backdrop is deactivated
  DP_ASSERT(m_backdropEnabled);
    
  // texture coord setup for mapping the backdrop
  //
  dp::sg::core::FrustumCameraSharedPtr const& theCamera = viewState->getCamera().staticCast<dp::sg::core::FrustumCamera>();

  dp::math::Vec3f lookdir = theCamera->getDirection();
  DP_ASSERT( isNormalized( lookdir ) );

  dp::math::Vec3f up = theCamera->getUpVector();
  DP_ASSERT( isNormalized( up ) );

  dp::math::Vec3f camera_u = lookdir ^ up;
  dp::math::Vec3f camera_v = camera_u ^ lookdir;
  normalize(camera_u);
  normalize(camera_v);

  float focusDistance = theCamera->getFocusDistance();
  dp::math::Vec2f wo = theCamera->getWindowOffset();  // default (0, 0)
  dp::math::Vec2f ws = theCamera->getWindowSize();    // default (1, 1)
  dp::math::Box2f wr = theCamera->getWindowRegion();
  //theCamera->getWindowRegion(ll, ur); // defaults (0, 0) and (1, 1)
  dp::math::Vec2f ll = wr.getLower();
  dp::math::Vec2f ur = wr.getUpper();

  // This is the window into the world at focus distance.
  float l = wo[0] - 0.5f * ws[0];
  float b = wo[1] - 0.5f * ws[1];
      
  //  adjust the l/r/b/t values to the window region to view
  float r = l + ur[0] * ws[0];
  l += ll[0] * ws[0];
      
  float t = b + ur[1] * ws[1];
  b += ll[1] * ws[1];
      
  // The vector to the window region center.
  lookdir = lookdir * focusDistance;
  lookdir += camera_u * 0.5f * (l + r);
  lookdir += camera_v * 0.5f * (b + t);

  // Half sized vector of window region u and v directions.
  camera_u *= 0.5f * (r - l);
  camera_v *= 0.5f * (t - b);

  // Mind, lookdir and camera_u, camera_v are not normalized!

  // Seeding the texture coordinate 2 of the FSQ with the frustum corner vectors.
  std::vector<dp::math::Vec4f> worldFrustum;
  worldFrustum.push_back(dp::math::Vec4f(lookdir - camera_u - camera_v, 0.0f)); // lower left
  worldFrustum.push_back(dp::math::Vec4f(lookdir + camera_u - camera_v, 0.0f)); // lower right
  worldFrustum.push_back(dp::math::Vec4f(lookdir + camera_u + camera_v, 0.0f)); // upper right
  worldFrustum.push_back(dp::math::Vec4f(lookdir - camera_u + camera_v, 0.0f)); // upper left

  m_environmentBackdrop->setTexCoords(2, worldFrustum);

  m_environmentBackdrop->render();
}

void SceneRendererPipeline::doRenderStandard(dp::sg::ui::ViewStateSharedPtr const& viewState, dp::ui::SmartRenderTarget const& renderTarget)
{
  dp::gl::SmartRenderTarget const & renderTargetGL = dp::util::smart_cast<dp::gl::RenderTarget>(renderTarget);
  dp::gl::TargetBufferMask clearMask = renderTargetGL->getClearMask();

  if (m_backdropEnabled)
  {
    // Instead of glClear the viewport before rendering the scene, 
    // render a full screen quad with the environment backdrop effect applied
    //
    // We render the color buffer, depth and stencil might still be cleared.
    renderTargetGL->setClearMask( clearMask & ~dp::gl::TBM_COLOR_BUFFER );

    // render the backdrop instead of clearing the color buffer
    doRenderBackdrop( viewState );
  }
  else
  {
    // ensure to clear the color buffer for no backdrop
    renderTargetGL->setClearMask( clearMask | dp::gl::TBM_COLOR_BUFFER );
  }

  // Call the current scene renderer and render the whole scene into the main render target (tonemapFBO).
  DP_ASSERT( viewState->getTraversalMask() == ~0 );

  // This renders only one eye even if the renderTarget is stereoscopic.
  // Important: The m_sceneRenderer holds a NOP StereoViewStateProvider to use the already adjusted stereo ViewState camera!
  m_sceneRenderer->render(viewState, renderTarget, renderTarget->getStereoTarget());
}

void SceneRendererPipeline::doRenderTonemap(dp::sg::ui::ViewStateSharedPtr const& viewState, dp::ui::SmartRenderTarget const& renderTarget)
{
  dp::gl::SmartRenderTarget const & renderTargetGL = dp::util::smart_cast<dp::gl::RenderTarget>(renderTarget);
  dp::gl::TargetBufferMask clearMask = renderTargetGL->getClearMask();

  // Match the size of the tonemapFBO to the destination renderTarget.
  unsigned int width;
  unsigned int height;
  renderTarget->getSize(width, height);
  m_tonemapFBO->setSize(width, height);

  if (m_backdropEnabled)
  {
    // Instead of glClear the viewport before rendering the scene, 
    // render a full screen quad with the environment backdrop effect applied
    //
    // We render the color buffer, depth and stencil might still be cleared.
    m_tonemapFBO->setClearMask( clearMask & ~dp::gl::TBM_COLOR_BUFFER );

    // render the backdrop instead of clearing the color buffer
    doRenderBackdrop( viewState );
  }
  else
  {
    // ensure to clear the color buffer for no backdrop
    m_tonemapFBO->setClearMask( clearMask | dp::gl::TBM_COLOR_BUFFER );
  }

  // Call the current scene renderer and render the whole scene into the main render target (tonemapFBO).
  DP_ASSERT( viewState->getTraversalMask() == ~0 );

  // This renders only one eye even if the renderTarget is stereoscopic.
  // Important: The m_sceneRenderer holds a NOP StereoViewStateProvider to use the already adjusted stereo ViewState camera!
  m_sceneRenderer->render(viewState, m_tonemapFBO, renderTarget->getStereoTarget());

  if ( m_tonemapperValuesChanged )
  {
    // Set the tonemapper parameters:
    const dp::sg::core::ParameterGroupDataSharedPtr& parameterGroupData = m_tonemapperData->findParameterGroupData( std::string( "tonemapParameters" ) );
    DP_ASSERT( parameterGroupData );

    DP_VERIFY( parameterGroupData->setParameter( "invGamma", 1.0f / m_tonemapperValues.gamma ) );
    DP_VERIFY( parameterGroupData->setParameter( "invWhitePoint", m_tonemapperValues.brightness / m_tonemapperValues.whitePoint) );
    DP_VERIFY( parameterGroupData->setParameter( "saturation", m_tonemapperValues.saturation ) );
    DP_VERIFY( parameterGroupData->setParameter( "crushBlacks", m_tonemapperValues.crushBlacks + m_tonemapperValues.crushBlacks + 1.0f) ); // Note, the default if the shader variable crushBlacks is 1.0!
    DP_VERIFY( parameterGroupData->setParameter( "burnHighlights", m_tonemapperValues.burnHighlights ) );

    m_tonemapperValuesChanged = false;
  }

  // No need to clear anything. This tonemapping pass just copies pixels and ignores depth.
  renderTargetGL->setClearMask( 0 ); 
  m_tonemapper->render();
  renderTargetGL->setClearMask( clearMask );
}

void SceneRendererPipeline::doRenderHighlight(dp::sg::ui::ViewStateSharedPtr const& viewState, dp::ui::SmartRenderTarget const& renderTarget)
{
  // only call this if objects need to be rendered highlighted
  DP_ASSERT(m_highlighting);

  // Highlight pass:
  // Match the size of the highlightFBO to the destination renderTarget.
  unsigned int width;
  unsigned int height;
  renderTarget->getSize(width, height);
  m_highlightFBO->setSize(width, height);

  unsigned int originalTraversalMask = viewState->getTraversalMask();
  viewState->setTraversalMask(2); // Render only the highlighted objects.

  glPushAttrib( GL_STENCIL_BUFFER_BIT );

  // If an object is highlighted, render the highlighted object into the stencil buffer of the FBO.

  // Setup the proper stencil state. 
  // Write a 1 for every rendered fragment into the stencil buffer
  glStencilFunc(GL_NEVER, 1, ~0);
  glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
  glEnable(GL_STENCIL_TEST);

  // This is always using a SceneRendererGL2 to render the highlighted objects.
  m_highlightFBO->setClearMask( dp::gl::TBM_COLOR_BUFFER
                              | dp::gl::TBM_DEPTH_BUFFER
                              | dp::gl::TBM_STENCIL_BUFFER); // Clear all.

  // A SceneRenderer always uses the scene background color to clear. Temporarily change it to black here.
  dp::math::Vec4f backgroundColor = viewState->getScene()->getBackColor();
  viewState->getScene()->setBackColor( dp::math::Vec4f(0.0f, 0.0f, 0.0f, 0.0f) );
 
  m_sceneRendererHighlight->render(viewState, m_highlightFBO);

  viewState->getScene()->setBackColor(backgroundColor);

  viewState->setTraversalMask(originalTraversalMask); // Reset the traversal mask.

  // Highlight post-processing:
  // Migrate the stencil bit contents as white color into the texture rectangle.
  m_highlightFBO->setClearMask( dp::gl::TBM_COLOR_BUFFER ); // Do not clear the stencil! Don't care for depth.

  // set the stencil as needed for the stencil to color pass
  glStencilFunc( GL_EQUAL, 1, ~0 );
  glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

  m_rendererStencilToColor->render();

  glPopAttrib();

  // Render the outline around the highlighted object onto the main renderTarget (framebuffer).
  dp::gl::SmartRenderTarget const & renderTargetGL = dp::util::smart_cast<dp::gl::RenderTarget>(renderTarget);
  dp::gl::TargetBufferMask clearMask = renderTargetGL->getClearMask();
  
  // keep the following render call from clearing the previous rendered content
  renderTargetGL->setClearMask( 0 ); 
  m_rendererHighlight->render();
  // restore the clear mask
  renderTargetGL->setClearMask( clearMask );
}

// This switches the renderer for the main pass between rasterizer and raytracer.
void SceneRendererPipeline::setSceneRenderer(const dp::sg::ui::SmartSceneRenderer &sceneRenderer)
{
  m_sceneRenderer = sceneRenderer;
  // Do not separate the ViewState camera another time during the render() call issued inside the SceneRendererPipeline.
  m_sceneRenderer->setStereoViewStateProvider(m_monoViewStateProvider);

  // If the renderer is a SceneRendererGL2 reuse it for the highlighting to keep the number of RenderLists small.
  if (dp::util::isSmartPtrOf<dp::sg::renderer::rix::gl::SceneRenderer>(m_sceneRenderer))
  {
    m_sceneRendererHighlight = m_sceneRenderer;
  }
  else
  {
    Viewer* viewer = GetApp();
    m_sceneRendererHighlight = dp::sg::renderer::rix::gl::SceneRenderer::create( viewer->getRenderEngine().c_str(),
                                                                                 viewer->getShaderManagerType(),
                                                                                 viewer->getCullingMode(),
                                                                                 dp::sg::renderer::rix::gl::TM_SORTED_BLENDED );
  }
}

dp::sg::ui::SmartSceneRenderer SceneRendererPipeline::getSceneRenderer() const 
{
  return m_sceneRenderer;
}

void SceneRendererPipeline::enableHighlighting(bool onOff)
{
  m_highlighting = onOff;
}

void SceneRendererPipeline::onEnvironmentSamplerChanged()
{
  DP_ASSERT( m_sceneRenderer );
  m_sceneRenderer->setEnvironmentSampler( getEnvironmentSampler() );
}

void SceneRendererPipeline::updateEnvironment()
{
  if ( m_environmentBackdrop )
  {
    m_environmentBackdrop->setSamplerByName( "environment", GetApp()->getEnvironmentSampler() );
  }
}

std::map<dp::fx::Domain,std::string> SceneRendererPipeline::getShaderSources( const dp::sg::core::GeoNodeSharedPtr & geoNode, bool depthPass ) const
{
  DP_ASSERT( m_sceneRenderer );
  return( m_sceneRenderer->getShaderSources( geoNode, depthPass ) );
}

void SceneRendererPipeline::setCullingEnabled( bool enabled )
{
  DP_ASSERT( m_sceneRenderer );
  m_sceneRenderer->setCullingEnabled( enabled );
}

bool SceneRendererPipeline::isCullingEnabled() const
{
  DP_ASSERT( m_sceneRenderer );
  return( m_sceneRenderer->isCullingEnabled() );
}

void SceneRendererPipeline::setCullingMode( dp::culling::Mode mode )
{
  DP_ASSERT( m_sceneRenderer );
  m_sceneRenderer->setCullingMode( mode );
}

dp::culling::Mode SceneRendererPipeline::getCullingMode( ) const
{
  DP_ASSERT( m_sceneRenderer );
  return( m_sceneRenderer->getCullingMode() );
}

void SceneRendererPipeline::setShaderManager( dp::fx::Manager manager )
{
  DP_ASSERT( m_sceneRenderer );
  m_sceneRenderer->setShaderManager( manager );
}

dp::fx::Manager SceneRendererPipeline::getShaderManager( ) const
{
  DP_ASSERT( m_sceneRenderer );
  return( m_sceneRenderer->getShaderManager() );
}

dp::sg::renderer::rix::gl::TransparencyMode SceneRendererPipeline::getTransparencyMode() const
{
  DP_ASSERT( m_sceneRenderer );
  DP_ASSERT( dp::util::isSmartPtrOf<dp::sg::renderer::rix::gl::SceneRenderer>(m_sceneRenderer) );
  return( dp::util::smart_cast<dp::sg::renderer::rix::gl::SceneRenderer>(m_sceneRenderer)->getTransparencyMode() );
}

void SceneRendererPipeline::setTransparencyMode( dp::sg::renderer::rix::gl::TransparencyMode mode )
{
  DP_ASSERT( mode != getTransparencyMode() );
  DP_ASSERT( m_sceneRenderer );
  DP_ASSERT( dp::util::isSmartPtrOf<dp::sg::renderer::rix::gl::SceneRenderer>(m_sceneRenderer) );
  dp::util::smart_cast<dp::sg::renderer::rix::gl::SceneRenderer>(m_sceneRenderer)->setTransparencyMode( mode );
}

void SceneRendererPipeline::setTonemapperEnabled( bool enabled )
{
  if ( m_tonemapperEnabled != enabled )
  {
    if ( m_tonemapperEnabled )
    {
      m_tonemapFBO.reset();
      m_tonemapper.reset();
      m_tonemapperData.reset();
      m_tonemapperEnabled = false;
    }
    else
    {
      initTonemapper();
      setTonemapperValues( getTonemapperValues() );
    }
    initBackdrop();
  }
}

bool SceneRendererPipeline::isTonemapperEnabled() const
{
  return( m_tonemapperEnabled );
}

TonemapperValues SceneRendererPipeline::getTonemapperValues() const
{
  return m_tonemapperValues;
}

void SceneRendererPipeline::setTonemapperValues( const TonemapperValues& values )
{
  m_tonemapperValues = values;
  m_tonemapperValuesChanged = true;
}

void SceneRendererPipeline::initBackdrop()
{
  // initialize everything required to render the backdrop
  if ( m_tonemapperEnabled )
  {
    m_environmentBackdrop = dp::sg::renderer::rix::gl::FSQRenderer::create( m_tonemapFBO ); // Render the backdrop to the tonemapper texture.
  }
  else
  {
    m_environmentBackdrop = dp::sg::renderer::rix::gl::FSQRenderer::create( m_renderTarget ); // Render to the standard framebuffer.
  }
  m_environmentBackdrop->setEffect( dp::sg::core::EffectData::create( EffectLibrary::instance()->getEffectSpec( std::string("environmentBackdrop") ) ) );

  dp::sg::core::SamplerSharedPtr environmentSampler = dp::sg::core::Sampler::create( GetApp()->getEnvironmentSampler()->getTexture() );
  environmentSampler->setWrapModes( dp::sg::core::TWM_REPEAT, dp::sg::core::TWM_CLAMP_TO_EDGE, dp::sg::core::TWM_REPEAT );
  environmentSampler->setMagFilterMode( dp::sg::core::TFM_MAG_LINEAR );
  environmentSampler->setMinFilterMode( dp::sg::core::TFM_MIN_LINEAR );

  m_environmentBackdrop->setSamplerByName( "environment", environmentSampler );
}

void SceneRendererPipeline::initTonemapper()
{
  // Create an FBO with 2D texture color attachment and depth stencil render buffers.
  // This one remains monoscopic, SceneRendererPipeline::doRender() works per eye.
  m_tonemapFBO = dp::gl::RenderTargetFBO::create( m_highlightFBO->getRenderContext() );

  // Set the defaults for the render pass.
  // This clear color actually doesn't take effect when using a SceneRenderer. The scene background color has precedence.
  m_tonemapFBO->setClearColor(0.0f, 0.0f, 0.0f, 0.0f); 
  m_tonemapFBO->setClearDepth(1.0);
  m_tonemapFBO->setClearStencil(0);

  // Make the OpenGL context on the renderContext current. It's needed for the create() operations.
  dp::gl::RenderContextStack rcglstack;
  rcglstack.push( m_highlightFBO->getRenderContext() );

  // Render to 2D texture.
  m_tonemapFBO->setAttachment(dp::gl::RenderTargetFBO::COLOR_ATTACHMENT0,
                              dp::gl::Texture2D::create(GL_RGBA32F, GL_RGBA, GL_FLOAT)); // HDR rendering!
  // Depth and Stencil are Renderbuffers.
  m_tonemapFBO->setAttachment(dp::gl::RenderTargetFBO::DEPTH_ATTACHMENT,   m_highlightFBO->getAttachment( dp::gl::RenderTargetFBO::DEPTH_ATTACHMENT ) );
  m_tonemapFBO->setAttachment(dp::gl::RenderTargetFBO::STENCIL_ATTACHMENT, m_highlightFBO->getAttachment( dp::gl::RenderTargetFBO::STENCIL_ATTACHMENT ));

  m_tonemapFBO->setDrawBuffers( m_highlightFBO->getDrawBuffers() ); // Still COLOR_ATTACHMENT0.

  rcglstack.pop();

  m_tonemapper = dp::sg::renderer::rix::gl::FSQRenderer::create( m_renderTarget );
  m_tonemapperData = dp::sg::core::EffectData::create( dp::fx::EffectLibrary::instance()->getEffectData("tonemapper") );
  m_tonemapper->setEffect( m_tonemapperData );

  const dp::gl::RenderTargetFBO::SmartAttachment &attachmentTonemap = m_tonemapFBO->getAttachment(dp::gl::RenderTargetFBO::COLOR_ATTACHMENT0);
  const dp::gl::RenderTargetFBO::SmartAttachmentTexture &texAttTonemap = dp::util::smart_cast<dp::gl::RenderTargetFBO::AttachmentTexture>(attachmentTonemap);
  if ( texAttTonemap )
  {
    const dp::sg::gl::TextureGLSharedPtr texGL = dp::sg::gl::TextureGL::create( texAttTonemap->getTexture() );
    dp::sg::core::SamplerSharedPtr sampler = dp::sg::core::Sampler::create( texGL );
    sampler->setWrapModes( dp::sg::core::TWM_CLAMP_TO_EDGE, dp::sg::core::TWM_CLAMP_TO_EDGE, dp::sg::core::TWM_CLAMP_TO_EDGE );
    sampler->setMagFilterMode( dp::sg::core::TFM_MAG_NEAREST );
    sampler->setMinFilterMode( dp::sg::core::TFM_MIN_NEAREST );

    m_tonemapper->setSamplerByName( "tonemapHDR", sampler );
  }

  m_tonemapperEnabled = true;
}
