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


#include <IndicesGL.h>
#include <BufferGL.h>

namespace dp
{
  namespace rix
  {
    namespace gl
    {
      using namespace dp::rix::core;

      IndicesGL::Event::Event( IndicesGLHandle indices )
        : m_indices( indices )
      {
      }

      IndicesGL::IndicesGL()
        : m_markedForUpload( false )
        , m_bufferGLSize( 0 )
        , m_dataType( dp::util::DT_UNSIGNED_INT_8 )
        , m_bufferHandle( nullptr )
        , m_offset( 0 )
        , m_count( 0 )
      {
      }

      IndicesGL::~IndicesGL()
      {
        if ( m_bufferHandle )
        {
          m_bufferHandle->detach( this );
        }

        handleReset( m_bufferHandle );
      }

      void IndicesGL::setData( dp::util::DataType dataType, dp::rix::core::BufferHandle bufferHandle, size_t offset, size_t count )
      {
        if ( m_bufferHandle )
        {
          m_bufferHandle->detach( this );
        }

        m_dataType = dataType;
        handleAssign( m_bufferHandle, handleCast<BufferGL>(bufferHandle) );
        m_offset = offset;
        m_count = count;

        if ( m_bufferHandle )
        {
          m_bufferHandle->attach( this );
        }
        notify( Event(this) );

        // TODO observe buffer
      }

      void IndicesGL::onNotify( const dp::util::Event& /*event*/, dp::util::Payload* /*payload*/ )
      {
        notify( Event( this ) );
      }

      void IndicesGL::onDestroyed( const dp::util::Subject& /*subject*/, dp::util::Payload* /*payload*/ )
      {
        DP_ASSERT( !"need to detach from something?" );
      }

    } // namespace gl
  } // namespace rix
} // namespace dp
