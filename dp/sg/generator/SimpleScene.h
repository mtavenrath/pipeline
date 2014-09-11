// Copyright NVIDIA Corporation 2009
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

#include <dp/sg/generator/Config.h>

#include <dp/sg/core/nvsgapi.h> // NVSG storage-class defines
#include <dp/sg/core/CoreTypes.h>
#include <dp/util/SmartPtr.h>

/* This class defines a simple scene with a set of drawables.
*/

class SimpleScene
{
public:
  DP_SG_GENERATOR_API SimpleScene();
  DP_SG_GENERATOR_API virtual ~SimpleScene();

  DP_SG_GENERATOR_API void setEffectData( size_t index, const std::string& effectData );

  dp::sg::core::SceneSharedPtr       m_sceneHandle;
  dp::sg::core::EffectDataSharedPtr  m_effectHandle[4];    // one new material for each cube
  dp::sg::core::TransformSharedPtr   m_transformHandle[4]; // one transform for each cube
  dp::sg::core::GeoNodeSharedPtr     m_geoNodeHandle[4];   // one geonode for each cube
  dp::sg::core::PrimitiveSharedPtr   m_primitive;          // the drawable attached to the transforms
};