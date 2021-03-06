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

#include <dp/util/RCObject.h>
#include <dp/fx/EffectDefs.h>
#include <dp/fx/ParameterGroupSpec.h>

namespace dp
{
  namespace fx
  {
    class ParameterGroupLayout;
    typedef dp::util::SmartPtr<ParameterGroupLayout> SmartParameterGroupLayout;

    class ParameterGroupLayout : public dp::util::RCObject
    {
    public:
      class ParameterInfo : public dp::util::RCObject
      {
      public:
        virtual void convert( void* dstBase, void const* src ) const = 0;
      };

      typedef dp::util::SmartPtr<ParameterInfo> SmartParameterInfo;

      DP_FX_API static SmartParameterGroupLayout create( dp::fx::Manager manager, const std::vector<SmartParameterInfo>& parameterInfos, const std::string& groupName, size_t bufferSize, bool isInstanced, const SmartParameterGroupSpec& spec );

      /** \brief Get ParameterInfo for the given iterator **/
      SmartParameterInfo const & getParameterInfo( const dp::fx::ParameterGroupSpec::iterator& it) const;

      /** size of a single buffer if non-uniforms are being used **/
      DP_FX_API size_t getBufferSize() const; 

      /** \brief uniform_struct: prefix of uniform names for struct.
                 shaderbuffer/ubo: name of uniform containing the struct.
      **/
      DP_FX_API const std::string& getGroupName() const;

      /** \brief Returns true if multiple ParameterGroups should reside in a shared buffer **/
      DP_FX_API bool        isInstanced() const;

      DP_FX_API dp::fx::Manager getManager() const;
    protected:
      DP_FX_API ParameterGroupLayout( dp::fx::Manager manager, const std::vector<SmartParameterInfo>& parameterInfos, const std::string& groupName, size_t bufferSize, bool isInstanced, const SmartParameterGroupSpec& spec );
      SmartParameterGroupSpec         m_spec;
      std::string                     m_groupName;
      std::vector<SmartParameterInfo> m_parameterInfos;
      dp::fx::Manager                 m_manager;
      size_t                          m_bufferSize;
      bool                            m_isInstanced;
    };

    inline ParameterGroupLayout::SmartParameterInfo const& ParameterGroupLayout::getParameterInfo( const ParameterGroupSpec::iterator& it ) const
    {
      size_t index = std::distance( m_spec->beginParameterSpecs(), it );
      return m_parameterInfos[index];
    }
  } // namespace fx
} // namespace dp
