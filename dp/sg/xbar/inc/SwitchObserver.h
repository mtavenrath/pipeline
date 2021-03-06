// Copyright NVIDIA Corporation 2010-2013
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

#include <dp/sg/xbar/inc/Observer.h>

namespace dp
{
  namespace sg
  {
    namespace xbar
    {

      class SwitchObserver : public Observer<ObjectTreeIndex>
      {
      public:
        static SmartSwitchObserver create()
        {
          return( new SwitchObserver() );
        }
      public:
        class SwitchObserverPayload : public Observer<ObjectTreeIndex>::Payload
        {
        public:
          SwitchObserverPayload( ObjectTreeIndex index, unsigned int hints )
            : Observer<ObjectTreeIndex>::Payload( index )
            , m_hints( hints )
          {
          }

          unsigned int    m_hints;
        };
        typedef dp::util::SmartPtr<SwitchObserverPayload> SwitchObserverPayloadSharedPtr;

      public:
        SwitchObserver() : Observer<ObjectTreeIndex>( nullptr )
          , m_changed(false)
        {}
        ~SwitchObserver()
        {
        }

        void attach( dp::sg::core::SwitchSharedPtr const & s, ObjectTreeIndex index );

        bool isChanged() const { return m_changed; }

        void popDirtySwitches( ObjectTreeIndexSet currentSet )
        {
          currentSet = m_dirtySwitches;
          m_dirtySwitches.clear();
        }

      protected:
        void onNotify( const dp::util::Event &event, dp::util::Payload *payload );
        virtual void onDetach( ObjectTreeIndex index );

      private:
        mutable bool               m_changed;
        mutable ObjectTreeIndexSet m_dirtySwitches;
      };

    } // namespace xbar
  } // namespace sg
} // namespace dp
