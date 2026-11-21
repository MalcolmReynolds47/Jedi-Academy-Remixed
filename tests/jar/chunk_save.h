// Sequential native save chunks for production serializer fixtures. GPL-2.0-only.
#pragma once
#include "memory_save.h"
class ChunkSave : public ojk::ISavedGame {
 std::vector<byte> data;size_t offset=0,index=0;
public:
 std::vector<std::pair<uint32_t,std::vector<byte>>> chunks;
 bool read_chunk(uint32_t id)override{CHECK(index<chunks.size()&&chunks[index].first==id);data=chunks[index++].second;offset=0;return true;}
 bool write_chunk(uint32_t id)override{chunks.push_back({id,data});return true;}
 bool is_all_data_read()const override{return offset==data.size();}
 void ensure_all_data_read()override{CHECK(is_all_data_read());}
 bool read(void*p,int n)override{CHECK(n>=0&&offset+n<=data.size());if(n)memcpy(p,data.data()+offset,n);offset+=n;return true;}
 bool write(const void*p,int n)override{CHECK(n>=0);if(n){auto*b=static_cast<const byte*>(p);data.insert(data.end(),b,b+n);}return true;}
 bool skip(int n)override{CHECK(n>=0&&offset+n<=data.size());offset+=n;return true;}
 void save_buffer()override{throw std::runtime_error("unexpected buffer operation");}
 void load_buffer()override{throw std::runtime_error("unexpected buffer operation");}
 const void*get_buffer_data()const override{return data.data();}
 int get_buffer_size()const override{return int(data.size());}
 void reset_buffer()override{data.clear();offset=0;}
 void reset_buffer_offset()override{offset=0;}
 bool is_failed()const override{return false;}void clear_error()override{}
 void throw_error()override{throw std::runtime_error("save failure");}
};
