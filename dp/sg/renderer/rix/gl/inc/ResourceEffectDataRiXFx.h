// Copyright NVIDIA Corporation 2012
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


#pragma once

#include <dp/sg/renderer/rix/gl/Config.h>
#include <dp/sg/renderer/rix/gl/inc/ResourceManager.h>
#include <dp/sg/renderer/rix/gl/inc/ResourceEffectSpecRiXFx.h>
#include <dp/sg/renderer/rix/gl/inc/ResourceParameterGroupDataRiXFx.h>
#include <dp/sg/core/EffectData.h>
#include <dp/rix/fx/Manager.h>

namespace dp
{
  namespace sg
  {
    namespace renderer
    {
      namespace rix
      {
        namespace gl
        {
          class ResourceEffectDataRiXFx;
          typedef dp::util::SmartPtr<ResourceEffectDataRiXFx> SmartResourceEffectDataRiXFx;
          typedef ResourceEffectDataRiXFx* WeakResourceEffectDataRiXFx;


          class ResourceEffectDataRiXFx : public ResourceManager::Resource
          {
          public:
            typedef std::vector<dp::rix::fx::GroupDataSharedHandle> GroupDatas;

            /** \brief Fetch resource for the given object/resourceManager. If no resource exists it'll be created **/
            static SmartResourceEffectDataRiXFx get( const dp::sg::core::EffectDataSharedPtr &effectData, const dp::rix::fx::SmartManager& rixFx, const SmartResourceManager& resourceManager );
            virtual ~ResourceEffectDataRiXFx();

            virtual const dp::sg::core::HandledObjectSharedPtr& getHandledObject() const;
            virtual void update();

            ResourceEffectDataRiXFx::GroupDatas getGroupDatas() const;
          protected:
            ResourceEffectDataRiXFx( const dp::sg::core::EffectDataSharedPtr &effectData, const dp::rix::fx::SmartManager& rixFx, const SmartResourceManager& resourceManager );

            std::vector<SmartResourceParameterGroupDataRiXFx> m_resourceParameterGroupDataRiXFxs;
            SmartResourceEffectSpecRiXFx         m_resourceEffectSpec;
            dp::rix::fx::SmartManager            m_rixFx;
            dp::sg::core::EffectDataSharedPtr            m_effectData;
          };

        } // namespace gl
      } // namespace rix
    } // namespace renderer
  } // namespace sg
} // namespace dp

