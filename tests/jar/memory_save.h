// In-memory native save fixture. GPL-2.0-only.
#pragma once
#include "qcommon/ojk_i_saved_game.h"
#include "qcommon/ojk_saved_game_helper.h"
#include "test_support.h"
#include <vector>
// Use the real save serializers against an in-memory chunk buffer.
class MemorySave final : public ojk::ISavedGame {
    std::vector<byte> data;
    size_t offset = 0;
    bool reading = false;
public:
    bool read_chunk(uint32_t) override { throw std::runtime_error("unexpected chunk operation"); }
    bool write_chunk(uint32_t) override { throw std::runtime_error("unexpected chunk operation"); }
    bool is_all_data_read() const override { return offset == data.size(); }
    void ensure_all_data_read() override { CHECK(is_all_data_read()); }
    bool read(void* dst, int size) override {
        CHECK(reading && size >= 0 && offset + size_t(size) <= data.size());
        memcpy(dst, data.data() + offset, size); offset += size; return true;
    }
    bool write(const void* src, int size) override {
        CHECK(!reading && size >= 0); const byte* first = static_cast<const byte*>(src);
        data.insert(data.end(), first, first + size); offset += size; return true;
    }
    bool skip(int size) override {
        CHECK(size >= 0);
        if(reading)CHECK(offset + size_t(size) <= data.size());
        else data.resize(offset + size,0);
        offset += size; return true;
    }
    void save_buffer() override { throw std::runtime_error("unexpected buffer operation"); }
    void load_buffer() override { throw std::runtime_error("unexpected buffer operation"); }
    const void* get_buffer_data() const override { return data.data(); }
    int get_buffer_size() const override { return int(data.size()); }
    void reset_buffer() override { data.clear(); offset = 0; reading = false; }
    void reset_buffer_offset() override { offset = 0; reading = true; }
    bool is_failed() const override { return false; }
    void clear_error() override {}
    void throw_error() override { throw std::runtime_error("save serialization error"); }
};
