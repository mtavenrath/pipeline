// Copyright NVIDIA Corporation 2013
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


#include <dp/rix/gl/inc/ParameterCacheEntryStream.h>
#include <dp/rix/gl/inc/BufferGL.h>
#include <dp/rix/gl/inc/ContainerGL.h>
#include <dp/rix/gl/inc/Sampler.h>
#include <dp/rix/gl/inc/TextureGL.h>
#include <dp/rix/gl/inc/UniformUpdate.h>
#include <algorithm>
#include <cstring>

#if defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

namespace dp
{
  namespace rix
  {
    namespace gl
    {

      /************************************************************************/
      /* ParameterCacheEntryUniform                                           */
      /************************************************************************/
      ParameterCacheEntryStream::ParameterCacheEntryStream( size_t cacheOffset, size_t containerOffset, size_t arraySize, size_t size )
        : m_arraySize( dp::util::Uint32(arraySize) )
        , m_cacheOffset( cacheOffset )
        , m_containerOffset( containerOffset )
        , m_size( size )
      {

      }

      namespace
      {
        /************************************************************************/
        /* Copy with datatype conversion                                        */
        /************************************************************************/
        template <typename SourceType, typename DestinationType, int numComponents>
        struct UniformConversion
        {
          static void convert( void *dst, void const* src, unsigned int numberOfElements )
          {
            const SourceType* source = reinterpret_cast<const SourceType*>(src);
            DestinationType* destination = reinterpret_cast<DestinationType*>(dst);
            for ( size_t element = 0; element < numberOfElements; ++element )
            {
              destination[element] = source[element];
            }
          }
        };

        /************************************************************************/
        /* CacheEntrynt<n, T>                                                   */
        /************************************************************************/
        template<unsigned int n, typename T>
        class CacheEntrynt : public ParameterCacheEntryStream
        {
        public:
          CacheEntrynt( ProgramGLHandle /*program*/, int m_uniformLocation, size_t m_cacheOffset, size_t m_containerOffset, size_t m_arraySize );
          virtual void render( void const* cache ) const;
          virtual void update( void* cache, void const* container ) const;

        private:
          GLint m_uniformLocation;
        };


        template<unsigned int n, typename T>
        CacheEntrynt<n, T>::CacheEntrynt( ProgramGLHandle /*program*/, int uniformLocation, size_t cacheOffset, size_t containerOffset, size_t arraySize )
          : ParameterCacheEntryStream( cacheOffset, containerOffset, arraySize, sizeof( T ) * n * arraySize )
          , m_uniformLocation( uniformLocation )
        {
        }

        template<unsigned int n, typename T> 
        void CacheEntrynt<n, T>::render(void const* data) const
        {
          void const* offsetData = static_cast<char const*>(data) + m_cacheOffset;
          setUniform<n, T>( m_uniformLocation, m_arraySize, offsetData );
        }

        template<unsigned int n, typename T>
        void CacheEntrynt<n, T>::update( void* cache, void const* container ) const
        {
          memcpy( reinterpret_cast<char*>(cache) + m_cacheOffset, reinterpret_cast<char const*>(container) + m_containerOffset, m_size );
        }

        /************************************************************************/
        /* CacheEntryntConversion<n,T,SourceType>                                */
        /************************************************************************/
        template<unsigned int n, typename T, typename SourceType>
        class CacheEntryntConversion : public ParameterCacheEntryStream
        {
        public:
          CacheEntryntConversion( ProgramGLHandle /*program*/, dp::util::Int32 m_uniformLocation, size_t m_cacheOffset, size_t m_containerOffset, size_t m_arraySize );
          virtual void render( const void* cache ) const;
          virtual void update( void* cache, const void* containerData ) const;

        private:
          GLint m_uniformLocation;
        };


        template<unsigned int n, typename T, typename SourceType>
        CacheEntryntConversion<n, T, SourceType>::CacheEntryntConversion( ProgramGLHandle /*program*/, dp::util::Int32 uniformLocation
                                                                        , size_t cacheOffset, size_t containerOffset, size_t arraySize )
          : ParameterCacheEntryStream( cacheOffset, containerOffset, arraySize, sizeof( T ) * n * arraySize )
          , m_uniformLocation( uniformLocation )
        {
        }

        template<unsigned int n, typename T, typename SourceType> 
        void CacheEntryntConversion<n, T, SourceType>::render( void const* data ) const
        {
          const void* offsetData = static_cast<const char *>(data) + m_cacheOffset;
          setUniform<n, T>( m_uniformLocation, m_arraySize, offsetData );
        }

        template<unsigned int n, typename T, typename SourceType> 
        void CacheEntryntConversion<n, T, SourceType>::update( void* cache, void const* container ) const
        {
          UniformConversion<SourceType, T, n>::convert( reinterpret_cast<char*>(cache) + m_cacheOffset, reinterpret_cast<char const*>(container) + m_containerOffset, n * m_arraySize );
        }

        /************************************************************************/
        /* Parameternmt<n,m,T>                                                  */
        /************************************************************************/
        template<unsigned int n, unsigned m, typename T>
        class CacheEntrynmt : public ParameterCacheEntryStream
        {
        public:
          CacheEntrynmt( ProgramGLHandle /*program*/, dp::util::Int32 m_uniformLocation, size_t m_cacheOffset, size_t m_containerOffset, size_t m_arraySize );
          virtual void render( const void* cache ) const;
          virtual void update( void* cache, void const* containerData ) const;

          int          m_uniformLocation;
        };


        template<unsigned int n, unsigned int m, typename T>
        CacheEntrynmt<n, m, T>::CacheEntrynmt( ProgramGLHandle /*program*/, dp::util::Int32 uniformLocation
                                             , size_t cacheOffset, size_t containerOffset, size_t arraySize )
          : ParameterCacheEntryStream( cacheOffset, containerOffset, arraySize, sizeof(T) * arraySize * m * n )
          , m_uniformLocation( uniformLocation )
        {
        }

        template<unsigned int n, unsigned int m, typename T> 
        void CacheEntrynmt<n, m, T>::render(const void *data) const
        {
          void const* offsetData = static_cast<char const*>(data) + m_cacheOffset;
          setUniformMatrix<n, m, T>( m_uniformLocation, m_arraySize, false, offsetData );
        }

        template<unsigned int n, unsigned int m, typename T> 
        void CacheEntrynmt<n, m, T>::update( void* cache, void const* container ) const
        {
          memcpy( reinterpret_cast<char*>(cache) + m_cacheOffset, reinterpret_cast<char const*>(container) + m_containerOffset, m_size );
        }

        /************************************************************************/
        /* ParameterSampler                                                     */
        /************************************************************************/
        class CacheEntrySampler : public ParameterCacheEntryStream
        {
        public:
          CacheEntrySampler( ProgramGLHandle program, dp::util::Int32 uniformLocation, size_t cacheOffset, size_t containerOffset, size_t arraySize );
          virtual void render( void const* cache ) const;
          virtual void update( void* cache, void const* containerData ) const;

          struct CacheInfo
          {
            GLint m_textureId;
            GLint m_samplerId;
          };

          GLenum m_target;
          int    m_unit;
        };

        CacheEntrySampler::CacheEntrySampler( ProgramGLHandle program, dp::util::Int32 uniformLocation, size_t cacheOffset, size_t containerOffset, size_t arraySize )
          : ParameterCacheEntryStream( cacheOffset, containerOffset, arraySize, sizeof(CacheInfo) * arraySize )
        {
          // get texture unit and target for sampler
          m_target = dp::gl::getTargetForSamplerType( program->getProgram()->getUniformType( uniformLocation ) );
          glGetUniformiv( program->getProgram()->getGLId(), uniformLocation, &m_unit );
        }

        void CacheEntrySampler::render( void const* cache) const
        {
          void const* offsetData = static_cast<char const*>(cache) + m_cacheOffset;

          CacheInfo const* cacheInfo = reinterpret_cast<CacheInfo const*>( offsetData );

          for (unsigned int i = 0; i < m_arraySize; ++i)
          {
            // TODO add dsa version?
            glActiveTexture( GL_TEXTURE0 + m_unit + i);
            glBindTexture( m_target, cacheInfo[i].m_textureId );
            glBindSampler( m_unit + i , cacheInfo[i].m_samplerId );
          }
        }

        void CacheEntrySampler::update( void* cache, void const* container ) const
        {
          void const* containerOffset = reinterpret_cast<char const*>(container) + m_containerOffset;
          void *cacheData = reinterpret_cast<char *>(cache) + m_cacheOffset;

          ContainerGL::ParameterDataSampler const* parameterDataSampler = reinterpret_cast<const ContainerGL::ParameterDataSampler*>( containerOffset );
          CacheInfo* cacheInfo = reinterpret_cast<CacheInfo*>( cacheData );

          for ( size_t index = 0;index < m_arraySize;++index )
          {
            SamplerHandle const& sampler = parameterDataSampler[index].m_samplerHandle;
            TextureGLHandle texture = sampler ? sampler->getTexture().get() : nullptr;
            SamplerStateGLHandle samplerState = sampler ? sampler->getSamplerState().get() : nullptr;

            cacheInfo[index].m_textureId = texture ? texture->getTexture()->getGLId() : 0;
            cacheInfo[index].m_samplerId = samplerState ? samplerState->m_id : 0;
          }
        }

#if 0
        // TODO implement me later
        /************************************************************************/
        /* ParameterSamplerBindless                                             */
        /************************************************************************/
        ParameterSamplerBindless::ParameterSamplerBindless( unsigned int offset, int location, int unit, unsigned int arraySize )
          : ParameterObject( offset, sizeof( ContainerGL::ParameterDataSampler ), static_cast<ConversionFunction>(&ParameterSamplerBindless::doUpdateConverted) )
          , m_uniformLocation( location )
          , m_unit( unit )
          , m_arraySize( arraySize )
        {
          DP_ASSERT( location >= 0 );
        }

        void ParameterSamplerBindless::update(const void *data)
        {
          const void* offsetData = static_cast<const char*>(data) + m_offset;

          const ContainerGL::ParameterDataSampler* parameterDataSampler = reinterpret_cast<const ContainerGL::ParameterDataSampler*>( offsetData );

          // TODO arrays?
          glUniformHandleui64NV( m_uniformLocation, parameterDataSampler->m_bindlessHandle );
        }

        void ParameterSamplerBindless::copy( const void* containerData, void* destination ) const
        {
          memcpy(destination, reinterpret_cast<const char*>(containerData) + m_offset, getConvertedSize() );
        }

        void ParameterSamplerBindless::doUpdateConverted( void const* convertedData ) const
        {
          const void* offsetData = static_cast<const char*>(convertedData);
          const ContainerGL::ParameterDataSampler* parameterDataSampler = reinterpret_cast<const ContainerGL::ParameterDataSampler*>( offsetData );

          // TODO arrays?
          glUniformHandleui64NV( m_uniformLocation, parameterDataSampler->m_bindlessHandle );
        }
#endif

        /************************************************************************/
        /* CacheEntryImage                                                      */
        /************************************************************************/
        class CacheEntryImage : public ParameterCacheEntryStream
        {
        public:
          CacheEntryImage( ProgramGLHandle program, dp::util::Int32 uniformLocation, size_t cacheOffset, size_t containerOffset, size_t arraySize );
          virtual void render( void const* cache ) const;
          virtual void update( void* cache, void const* containerData ) const;

          struct CacheInfo
          {
            GLint     m_textureId;
            GLint     m_level;
            GLboolean m_layered;
            GLint     m_layer;
            GLenum    m_access;
            GLenum    m_internalFormat;
          };

          int    m_unit;
        };

        CacheEntryImage::CacheEntryImage( ProgramGLHandle program, dp::util::Int32 uniformLocation, size_t cacheOffset, size_t containerOffset, size_t arraySize )
          : ParameterCacheEntryStream( cacheOffset, containerOffset, arraySize, arraySize * sizeof(CacheInfo) )
        {
          DP_ASSERT( arraySize == 1 );

          // get unit for image
          glGetUniformiv( program->getProgram()->getGLId(), uniformLocation, &m_unit );
        }

        void CacheEntryImage::render( void const* cache ) const
        {
          void const* cacheData = static_cast<char const*>(cache) + m_cacheOffset;

          CacheInfo const* cacheInfo = reinterpret_cast<CacheInfo const*>( cacheData );
          glBindImageTexture( m_unit, cacheInfo->m_textureId, cacheInfo->m_level, cacheInfo->m_layered
                            , cacheInfo->m_layer, cacheInfo->m_access, cacheInfo->m_internalFormat );
        }

        void CacheEntryImage::update( void *cache, void const* container ) const
        {
          void const* containerData = static_cast<char const*>( container ) + m_containerOffset;
          void *cacheData = reinterpret_cast<char*>( cache ) + m_cacheOffset;

          ContainerGL::ParameterDataImage const* pdi = reinterpret_cast<ContainerGL::ParameterDataImage const*>(containerData); 
          CacheInfo* cacheInfo = reinterpret_cast<CacheInfo*>( cacheData );
          if ( pdi->m_textureHandle )
          {
            cacheInfo->m_textureId = pdi->m_textureHandle->getTexture()->getGLId();
            cacheInfo->m_level = pdi->m_level;
            cacheInfo->m_layered = pdi->m_layered;
            cacheInfo->m_layer = pdi->m_layer;
            cacheInfo->m_access = pdi->m_access;
            cacheInfo->m_internalFormat = pdi->m_textureHandle->getTexture()->getInternalFormat();
          }
          else
          {
            DP_ASSERT( !"invalid image passed" );
            memset( cacheInfo, 0, sizeof(CacheInfo ) );
          }
        }

        /************************************************************************/
        /* CacheEntryBufferBinding<GLenum BufferBinding>                        */
        /************************************************************************/
        template <GLenum BufferBinding>
        class CacheEntryBufferBinding : public ParameterCacheEntryStream
        {
        public:
          CacheEntryBufferBinding( dp::util::Int32 bindingIndex, size_t cacheOffset, size_t containerOffset, size_t arraySize );
          virtual void render( void const* cache ) const;
          virtual void update( void* cache, void const* container ) const;
          void doUpdateConverted( void const* converted ) const;

        private:
          GLuint     m_bindingIndex;
          struct CacheInfo
          {
            GLuint     m_bufferId;
            GLintptr   m_offset;
            GLsizeiptr m_size;
          };
        };

        template <GLenum BufferBinding>
        CacheEntryBufferBinding<BufferBinding>::CacheEntryBufferBinding( dp::util::Int32 bindingIndex, size_t cacheOffset, size_t containerOffset, size_t arraySize )
          : ParameterCacheEntryStream( cacheOffset, containerOffset, arraySize, sizeof(CacheInfo) * arraySize )
          , m_bindingIndex( bindingIndex )
        {
        }

        template <GLenum BufferBinding>
        void CacheEntryBufferBinding<BufferBinding>::render( void const* cache ) const
        {
          void const * cacheData = reinterpret_cast<char const *>(cache) + m_cacheOffset;
          CacheInfo const * parametersCache = reinterpret_cast<CacheInfo const *>(cacheData);

          glBindBufferRange( BufferBinding, m_bindingIndex, parametersCache->m_bufferId, parametersCache->m_offset, parametersCache->m_size );
        }

        template <GLenum BufferBinding>
        void CacheEntryBufferBinding<BufferBinding>::update( void* cache, void const* container ) const
        {
          void const* offsetData = static_cast<char const*>( container ) + m_containerOffset;
          ContainerGL::ParameterDataBuffer const* parametersContainer = static_cast<ContainerGL::ParameterDataBuffer const*>( offsetData );

          void * cacheData = reinterpret_cast<char *>(cache) + m_cacheOffset;
          CacheInfo * parametersCache = reinterpret_cast<CacheInfo *>(cacheData);

          if ( parametersContainer->m_bufferHandle )
          {
            DP_ASSERT( parametersContainer->m_bufferHandle->getBuffer() );
            dp::gl::SmartBuffer const& b = parametersContainer->m_bufferHandle->getBuffer();
            parametersCache->m_bufferId = b->getGLId();
            parametersCache->m_offset = parametersContainer->m_offset;
            parametersCache->m_size = GLuint( parametersContainer->m_length == size_t(~0) ? b->getSize() - parametersContainer->m_offset : parametersContainer->m_length );

            DP_ASSERT( parametersCache->m_bufferId );
            DP_ASSERT( parametersCache->m_size <= GLsizeiptr(b->getSize() ) );
          }
          else
          {
            parametersCache->m_bufferId = 0;
            parametersCache->m_offset = 0;
            parametersCache->m_size = 0;
          }
        }

        /************************************************************************/
        /* CacheEntryShaderBuffer                                               */
        /************************************************************************/
        class CacheEntryShaderBuffer : public ParameterCacheEntryStream
        {
        public:
          CacheEntryShaderBuffer( ProgramGLHandle program, dp::util::Int32 uniformLocation, size_t cacheOffset, size_t containerOffset, size_t arraySize );
          virtual void render( void const* cache ) const;
          virtual void update( void* cache, void const* containerData ) const;
        
        private:
          GLint m_uniformLocation;
        };

        CacheEntryShaderBuffer::CacheEntryShaderBuffer( UNUSED ProgramGLHandle program, dp::util::Int32 uniformLocation, size_t cacheOffset, size_t containerOffset, UNUSED size_t arraySize )
          : ParameterCacheEntryStream( cacheOffset, containerOffset, arraySize, sizeof(GLuint64EXT) * arraySize )
          , m_uniformLocation( uniformLocation )
        {
          DP_ASSERT( arraySize == 1 && "arrays not yet supported");
        }

        void CacheEntryShaderBuffer::render( void const* cache ) const
        {
          void const* cacheOffset  = static_cast<const char*>(cache) + m_cacheOffset;
          glUniformui64vNV( m_uniformLocation, 1, reinterpret_cast<GLuint64EXT const*>(cacheOffset) );
        }

        void CacheEntryShaderBuffer::update( void* cache, const void* container) const
        {
          const void* offsetData = static_cast<const char*>(container) + m_containerOffset;
          const ContainerGL::ParameterDataBuffer* param = static_cast<const ContainerGL::ParameterDataBuffer*>( offsetData );
          DP_ASSERT( param->m_bufferHandle && param->m_bufferHandle->getBuffer() );

          GLuint64EXT* cacheOffset = reinterpret_cast<GLuint64EXT *>(reinterpret_cast<char*>(cache) + m_cacheOffset);
          *cacheOffset = param->m_bufferHandle->getBuffer()->getAddress() + param->m_offset;
        }

      } // namespace anonymous

      ParameterCacheEntryStreamSharedPtr createParameterCacheEntryStream( dp::rix::gl::ProgramGLHandle program, dp::rix::core::ContainerParameterType containerParameterType
                                                                          , int uniformLocation, size_t cacheOffset
                                                                          , size_t containerOffset, size_t arraySize )
      {
        ParameterCacheEntryStreamSharedPtr parameterCacheEntry;
        GLenum uniformType = program->getProgram()->getUniformType( uniformLocation );

        // not a gl variable?
        if ( uniformType == GL_NONE )
        {
          return ParameterCacheEntryStreamSharedPtr::null;
        }

        size_t newArraySize = std::max( size_t(1), arraySize );

        switch( containerParameterType )
        {
        case dp::rix::core::CPT_FLOAT:
          DP_ASSERT( uniformType == GL_FLOAT );
          parameterCacheEntry = new CacheEntrynt<1, float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_FLOAT2:
          DP_ASSERT( uniformType == GL_FLOAT_VEC2 );
          parameterCacheEntry = new CacheEntrynt<2, float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_FLOAT3:
          DP_ASSERT( uniformType == GL_FLOAT_VEC3 );
          parameterCacheEntry = new CacheEntrynt<3, float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_FLOAT4:
          DP_ASSERT( uniformType == GL_FLOAT_VEC4 );
          parameterCacheEntry = new CacheEntrynt<4, float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;

        case dp::rix::core::CPT_INT_8:
          DP_ASSERT( uniformType == GL_INT );
          parameterCacheEntry = new CacheEntryntConversion<1, dp::util::Int32, dp::util::Int8>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_INT2_8:
          DP_ASSERT( uniformType == GL_INT_VEC2 );
          parameterCacheEntry = new CacheEntryntConversion<2, dp::util::Int32, dp::util::Int8>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_INT3_8:
          DP_ASSERT( uniformType == GL_INT_VEC3 );
          parameterCacheEntry = new CacheEntryntConversion<3, dp::util::Int32, dp::util::Int8>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_INT4_8:
          DP_ASSERT( uniformType == GL_INT_VEC4 );
          parameterCacheEntry = new CacheEntryntConversion<4, dp::util::Int32, dp::util::Int8>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;

        case dp::rix::core::CPT_INT_16:
          DP_ASSERT( uniformType == GL_INT );
          parameterCacheEntry = new CacheEntryntConversion<1, dp::util::Int32, dp::util::Int16>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_INT2_16:
          DP_ASSERT( uniformType == GL_INT_VEC2 );
          parameterCacheEntry = new CacheEntryntConversion<2, dp::util::Int32, dp::util::Int16>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_INT3_16:
          DP_ASSERT( uniformType == GL_INT_VEC3 );
          parameterCacheEntry = new CacheEntryntConversion<3, dp::util::Int32, dp::util::Int16>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_INT4_16:
          DP_ASSERT( uniformType == GL_INT_VEC4 );
          parameterCacheEntry = new CacheEntryntConversion<4, dp::util::Int32, dp::util::Int16>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;

        case dp::rix::core::CPT_INT_32:
          DP_ASSERT( uniformType == GL_INT );
          parameterCacheEntry = new CacheEntrynt<1, dp::util::Int32>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_INT2_32:
          DP_ASSERT( uniformType == GL_INT_VEC2 );
          parameterCacheEntry = new CacheEntrynt<2, dp::util::Int32>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_INT3_32:
          DP_ASSERT( uniformType == GL_INT_VEC3 );
          parameterCacheEntry = new CacheEntrynt<3, dp::util::Int32>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_INT4_32:
          DP_ASSERT( uniformType == GL_INT_VEC4 );
          parameterCacheEntry = new CacheEntrynt<4, dp::util::Int32>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;

        case dp::rix::core::CPT_INT_64:
          DP_ASSERT( uniformType == GL_INT64_NV );
          parameterCacheEntry = new CacheEntrynt<1, dp::util::Int64>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_INT2_64:
          DP_ASSERT( uniformType == GL_INT64_VEC2_NV );
          parameterCacheEntry = new CacheEntrynt<2, dp::util::Int64>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_INT3_64:
          DP_ASSERT( uniformType == GL_INT64_VEC3_NV );
          parameterCacheEntry = new CacheEntrynt<3, dp::util::Int64>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_INT4_64:
          DP_ASSERT( uniformType == GL_INT64_VEC4_NV );
          parameterCacheEntry = new CacheEntrynt<4, dp::util::Int64>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;

        case dp::rix::core::CPT_UINT_8:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT );
          parameterCacheEntry = new CacheEntryntConversion<1, dp::util::Int32, dp::util::Uint8>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_UINT2_8:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT_VEC2 );
          parameterCacheEntry = new CacheEntryntConversion<2, dp::util::Int32, dp::util::Uint8>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_UINT3_8:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT_VEC3 );
          parameterCacheEntry = new CacheEntryntConversion<3, dp::util::Int32, dp::util::Uint8>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_UINT4_8:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT_VEC4 );
          parameterCacheEntry = new CacheEntryntConversion<4, dp::util::Int32, dp::util::Uint8>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;

        case dp::rix::core::CPT_UINT_16:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT );
          parameterCacheEntry = new CacheEntryntConversion<1, dp::util::Int32, dp::util::Uint16>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_UINT2_16:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT_VEC2 );
          parameterCacheEntry = new CacheEntryntConversion<2, dp::util::Int32, dp::util::Uint16>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_UINT3_16:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT_VEC3 );
          parameterCacheEntry = new CacheEntryntConversion<3, dp::util::Int32, dp::util::Uint16>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_UINT4_16:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT_VEC4 );
          parameterCacheEntry = new CacheEntryntConversion<4, dp::util::Int32, dp::util::Uint16>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;

        case dp::rix::core::CPT_UINT_32:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT );
          parameterCacheEntry = new CacheEntrynt<1, dp::util::Uint32>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_UINT2_32:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT_VEC2 );
          parameterCacheEntry = new CacheEntrynt<2, dp::util::Uint32>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_UINT3_32:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT_VEC3 );
          parameterCacheEntry = new CacheEntrynt<3, dp::util::Uint32>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_UINT4_32:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT_VEC4 );
          parameterCacheEntry = new CacheEntrynt<4, dp::util::Uint32>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;

        case dp::rix::core::CPT_UINT_64:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT64_NV );
          parameterCacheEntry = new CacheEntrynt<1, dp::util::Uint64>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_UINT2_64:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT64_VEC2_NV );
          parameterCacheEntry = new CacheEntrynt<2, dp::util::Uint64>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_UINT3_64:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT64_VEC3_NV );
          parameterCacheEntry = new CacheEntrynt<3, dp::util::Uint64>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_UINT4_64:
          DP_ASSERT( uniformType == GL_UNSIGNED_INT64_VEC4_NV );
          parameterCacheEntry = new CacheEntrynt<4, dp::util::Uint64>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;

        case dp::rix::core::CPT_BOOL:
          DP_ASSERT( uniformType == GL_BOOL );
          parameterCacheEntry = new CacheEntryntConversion<1, dp::util::Int32, bool>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_BOOL2:
          DP_ASSERT( uniformType == GL_BOOL_VEC2 );
          parameterCacheEntry = new CacheEntryntConversion<2, dp::util::Int32, bool>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_BOOL3:
          DP_ASSERT( uniformType == GL_BOOL_VEC3 );
          parameterCacheEntry = new CacheEntryntConversion<3, dp::util::Int32, bool>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_BOOL4:
          DP_ASSERT( uniformType == GL_BOOL_VEC4 );
          parameterCacheEntry = new CacheEntryntConversion<4, dp::util::Int32, bool>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;

        case dp::rix::core::CPT_MAT2X2:
          DP_ASSERT( uniformType == GL_FLOAT_MAT2 );
          parameterCacheEntry = new CacheEntrynmt<2,2,float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_MAT2X3:
          DP_ASSERT( uniformType == GL_FLOAT_MAT2x3 );
          parameterCacheEntry = new CacheEntrynmt<2,3,float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_MAT2X4:
          DP_ASSERT( uniformType == GL_FLOAT_MAT2x4 );
          parameterCacheEntry = new CacheEntrynmt<2,4,float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;

        case dp::rix::core::CPT_MAT3X2:
          DP_ASSERT( uniformType == GL_FLOAT_MAT3x2 );
          parameterCacheEntry = new CacheEntrynmt<3,2,float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_MAT3X3:
          DP_ASSERT( uniformType == GL_FLOAT_MAT3 );
          parameterCacheEntry = new CacheEntrynmt<3,3,float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_MAT3X4:
          DP_ASSERT( uniformType == GL_FLOAT_MAT3x4 );
          parameterCacheEntry = new CacheEntrynmt<3,4,float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;

        case dp::rix::core::CPT_MAT4X2:
          DP_ASSERT( uniformType == GL_FLOAT_MAT4x2 );
          parameterCacheEntry = new CacheEntrynmt<4,2,float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_MAT4X3:
          DP_ASSERT( uniformType == GL_FLOAT_MAT4x3 );
          parameterCacheEntry = new CacheEntrynmt<4,3,float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_MAT4X4:
          DP_ASSERT( uniformType == GL_FLOAT_MAT4 );
          parameterCacheEntry = new CacheEntrynmt<4,4,float>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_BUFFER_ADDRESS:
          DP_ASSERT( uniformType == GL_GPU_ADDRESS_NV );
          parameterCacheEntry = new CacheEntrynt<1, dp::util::Uint64>( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_SAMPLER:
          DP_ASSERT( dp::gl::isSamplerType( uniformType ) );
          parameterCacheEntry = new CacheEntrySampler( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_IMAGE:
          DP_ASSERT( dp::gl::isImageType( uniformType ) );
          parameterCacheEntry= new CacheEntryImage( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        case dp::rix::core::CPT_BUFFER:
          DP_ASSERT( uniformType == GL_GPU_ADDRESS_NV );
          parameterCacheEntry = new CacheEntryShaderBuffer( program, uniformLocation, cacheOffset, containerOffset, newArraySize );
          break;
        default:
          DP_ASSERT( !"unsupported type" );
          break;
        }
        return parameterCacheEntry;
      }

      ParameterCacheEntryStreamSharedPtr createParameterCacheEntryUniformBuffer( ProgramGL::BufferBinding binding, size_t cacheOffset
                                                                                , size_t containerOffset, size_t arraySize )
      {
        // arrays not yet supported
        DP_ASSERT( arraySize == 0 && "buffer arrays not supported" );
        ParameterCacheEntryStreamSharedPtr parameterCacheEntry;

        arraySize = 1;
        switch ( binding.bufferBindingType )
        {
#if defined(GL_VERSION_4_3)
        case ProgramGL::BBT_ATOMIC_COUNTER:
          return new CacheEntryBufferBinding<GL_ATOMIC_COUNTER_BUFFER>( binding.bufferIndex, cacheOffset, containerOffset, arraySize );
        case ProgramGL::BBT_SHADER_STORAGE_BUFFER:
          return new CacheEntryBufferBinding<GL_SHADER_STORAGE_BUFFER>( binding.bufferIndex, cacheOffset, containerOffset, arraySize );
#endif
        case ProgramGL::BBT_UBO:
          return new CacheEntryBufferBinding<GL_UNIFORM_BUFFER>( binding.bufferIndex, cacheOffset, containerOffset, arraySize );
        default:
          DP_ASSERT( !"unknown buffer binding type " );
          return nullptr;
        }
      }

      ParameterCacheEntryStreams createParameterCacheEntriesStream( dp::rix::gl::ProgramGLHandle program, dp::rix::gl::ContainerDescriptorGLHandle descriptor )
      {
        std::vector<ParameterCacheEntryStreamSharedPtr> parameterCacheEntries;
        size_t cacheOffset = 0;
        for ( std::vector<ContainerDescriptorGL::ParameterInfo>::iterator it = descriptor->m_parameterInfos.begin(); it != descriptor->m_parameterInfos.end(); ++it )
        {
          GLint uniformLocation = program->getProgram()->getUniformLocation( it->m_name.c_str() );
          
          // is it a uniform?
          if ( uniformLocation != -1 )
          {
            parameterCacheEntries.push_back( createParameterCacheEntryStream( program, it->m_type, uniformLocation, cacheOffset, it->m_offset, it->m_arraySize ) );
            cacheOffset += parameterCacheEntries.back()->getSize();
          }
          // it's no uniform, is it a buffer?
          else if ( it->m_type == dp::rix::core::CPT_BUFFER )
          {
              ProgramGL::BufferBinding bufferBinding = program->getBufferBinding( it->m_name );
              if ( bufferBinding.bufferBindingType != ProgramGL::BBT_NONE )
              {
                parameterCacheEntries.push_back( createParameterCacheEntryUniformBuffer( bufferBinding, cacheOffset, it->m_offset, it->m_arraySize) );
                cacheOffset += parameterCacheEntries.back()->getSize();
              }
          }
        }
        return parameterCacheEntries;
      }

    } // namespace gl
  } // namespace rix
} // namespace dp
