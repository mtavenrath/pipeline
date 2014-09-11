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


#include <dp/rix/fx/inc/BufferManagerOffset.h>
#include <boost/scoped_array.hpp>
#include <vector>

namespace dp
{
  namespace rix
  {
    namespace fx
    {

      BufferManagerOffset::BufferManagerOffset( dp::rix::core::Renderer* renderer
                                          , dp::rix::core::ContainerDescriptorSharedHandle const & descriptor, dp::rix::core::ContainerEntry entry // descriptor with buffer attached
                                          , size_t blockSize, size_t blockAlignment, size_t chunkSize )
        : BufferManagerImpl( renderer, blockSize, blockAlignment, chunkSize )
        , m_descriptor( descriptor )
        , m_entry( entry )
      {
      }

      BufferManagerOffset::~BufferManagerOffset()
      {

      }

      dp::rix::core::ContainerSharedHandle BufferManagerOffset::allocationGetBufferContainer( AllocationHandle allocation )
      {
        AllocationImplHandle allocationImpl = dp::rix::core::handleCast<AllocationImpl>(allocation);
        dp::rix::core::ContainerSharedHandle container = getRenderer()->containerCreate( m_descriptor );

        dp::rix::core::ContainerDataBuffer cdb( allocationImpl->m_chunk->m_buffer, allocationImpl->m_blockIndex * getAlignedBlockSize(), getBlockSize() );
        getRenderer()->containerSetData( container, m_entry, cdb );

        return container;
      }

      dp::rix::core::ContainerDescriptorSharedHandle BufferManagerOffset::getBufferDescriptor()
      {
        return m_descriptor;
      }

      dp::rix::core::ContainerEntry BufferManagerOffset::getBufferDescriptorEntry()
      {
        return m_entry;
      }

      void BufferManagerOffset::useContainers( dp::rix::core::GeometryInstanceSharedHandle const & gi, AllocationHandle allocation )
      {
        getRenderer()->geometryInstanceUseContainer( gi, allocationGetBufferContainer( allocation ) );
      }

      void BufferManagerOffset::useContainers( dp::rix::core::RenderGroupSharedHandle const & renderGroup, AllocationHandle allocation )
      {
        getRenderer()->renderGroupUseContainer( renderGroup, allocationGetBufferContainer( allocation ) );
      }

      SmartBufferManager BufferManagerOffset::create( dp::rix::core::Renderer* renderer
                                              , dp::rix::core::ContainerDescriptorSharedHandle const & descriptor, dp::rix::core::ContainerEntry entry // descriptor with buffer attached
                                              , size_t blockSize, size_t blockAlignment, size_t chunkSize )
      {
        return new BufferManagerOffset( renderer, descriptor, entry, blockSize, blockAlignment, chunkSize );
      }

    } // namespace fx
  } // namespace rix
} // namespace dp