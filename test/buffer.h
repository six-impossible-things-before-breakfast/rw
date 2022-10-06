/* -------------------------------------------------------------------------------
* Copyright (C) 2022 Six Impossible Things Before Breakfast Limited.
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*	 * Redistributions of source code must retain the above copyright
*	   notice, this list of conditions and the following disclaimer.
*	 * Redistributions in binary form must reproduce the above copyright
*	   notice, this list of conditions and the following disclaimer in the
*	   documentation and/or other materials provided with the distribution.
*	 * Neither the name of the Six Impossible Things Before Breakfast Limited nor the
*	   names of its contributors may be used to endorse or promote products
*	   derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL SIX IMPOSSIBLE THINGS BEFORE BREAKFAST LIMITED BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

#ifndef BUFFER_H
#define BUFFER_H

static constexpr size_t MIN_BUFFER = 1024;

class Buffer {
public:
    class ReadIter
    {
        const uint8_t* begin = nullptr;
        const uint8_t* end = nullptr;
        size_t currentOffset = 0;

    public:
        using BufferT = Buffer;

    public:
        ReadIter() {}
        ReadIter( const Buffer& buff ) {
            begin = buff.begin();
            end = buff.begin() + buff.size();
        }
        bool isData() { return end > begin; }
        size_t directlyAvailableSize() { return begin != nullptr ? end - begin : 0; }
        const uint8_t* directRead( size_t sz ) { 
            if ( begin != nullptr ) { 
                assert( end >= begin && sz <= static_cast<size_t>(end - begin) ); 
                auto ret = begin; 
                begin += sz; 
                currentOffset += sz;
                return ret; 
            }
            else
                return nullptr;
        }
        uint8_t operator * ()
        {
            assert( begin != end ); 
            return *begin;
        }
        void operator ++ () 
        {
            begin++;
            ++currentOffset;
        }
        size_t read( void* buff, size_t size )
        {
            if ( begin )
            {
                assert( end >= begin ); 
                size = size <= static_cast<size_t>(end - begin) ? size : static_cast<size_t>(end - begin);
                memcpy( buff, begin, size );
                begin += size;
                currentOffset += size;
            }
            else
                size = 0;
            return size; 
        }
        size_t skip( size_t size )
        {
            if ( begin )
            {
                assert( end >= begin ); 
                size = size <= static_cast<size_t>(end - begin) ? size : static_cast<size_t>(end - begin);
                begin += size;
                currentOffset += size;
            }
            else
                size = 0;
            return size;
        }
        size_t offset() const { return currentOffset; }
    };

    using ReadIteratorT = ReadIter;

    ReadIter getReadIter() { return ReadIter( *this ); }

private:
    size_t _size = 0;
    size_t _capacity = 0;
    std::unique_ptr<uint8_t[]> _data;

private:
    void ensureCapacity(size_t sz) { // NOTE: may invalidate pointers
        if (_data == nullptr) {
            reserve(sz);
        }
        else if (sz > _capacity) {
            size_t cp = std::max(sz, MIN_BUFFER);
            std::unique_ptr<uint8_t[]> tmp(new uint8_t[cp]);
            memcpy(tmp.get(), _data.get(), _size);
            _capacity = cp;
            _data = std::move(tmp);
        }
    }

protected:
    void reserve(size_t sz) {
        assert( _size == 0 ); 
        assert( _capacity == 0 );
        assert( _data == nullptr );

        size_t cp = std::max(sz, MIN_BUFFER);
        std::unique_ptr<uint8_t[]> tmp(new uint8_t[cp]);

        _capacity = cp;
        _data = std::move(tmp);
    }

public:
    Buffer() {}
//        Buffer(size_t res) { reserve(res); }
    Buffer(Buffer&& p) noexcept {
        std::swap(_size, p._size);
        std::swap(_capacity, p._capacity);
        std::swap(_data, p._data);
    }
    Buffer& operator = (Buffer&& p) noexcept {
        std::swap(_size, p._size);
        std::swap(_capacity, p._capacity);
        std::swap(_data, p._data);
        return *this;
    }
    Buffer(const Buffer&) = delete;
    Buffer& operator = (const Buffer& p) = delete;

    size_t size() const { return _size; }
    bool empty() const { return _size == 0; }
    uint8_t* begin() { return _data.get(); }
    const uint8_t* begin() const { return _data.get(); }
    uint8_t* end() { return _data.get() + _size; }
    const uint8_t* end() const { return _data.get() + _size; }

    void append(const void* dt, size_t sz) { // NOTE: may invalidate pointers
        ensureCapacity(_size + sz);
        memcpy(end(), dt, sz);
        _size += sz;
    }

    void append(ReadIteratorT it, size_t count)
    {
        ensureCapacity(_size + count);
        while(count != 0 && it.isData())
        {
            size_t avail = it.directlyAvailableSize();
            size_t toRead = std::min(count, avail);
            size_t actualyRead = it.read(end(), toRead);
            _size += actualyRead;
            count -= actualyRead;
        }

    }

    size_t appendUint8( int8_t val ) {
        ensureCapacity(size() + 1);
        *reinterpret_cast<uint8_t*>(begin() + size() ) = val;
        return ++_size;
    }

    void set_size(size_t sz) { // NOTE: keeps pointers
        assert( sz <= _capacity );
        assert( _data != nullptr );
        _size = sz;
    }
};


#endif // BUFFER_H
