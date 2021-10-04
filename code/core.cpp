#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "libretro.h"
#include "chip8.hpp"

namespace {

class emu {
public:
    using cpu = chip8::cpu;
    using video = chip8::video<>;

    static constexpr double fps = 60.f;
    static constexpr double sample_rate = 0.f;

    auto framebuffer() { return _video.framebuffer(); }

    void boot() { _cpu.boot(); }
    void reset() { _cpu.reset(); }
    void cycle() { _cpu.cycle(); }

    using data_t = cpu::ram_t::data_t;

    void load_rom(const data_t *data, size_t size) { _cpu.load_rom(data, size); }
    constexpr auto vram_data() const { return _cpu.vram_data(); }

private:
    cpu _cpu;
    video _video;
};

emu s_emu;

retro_video_refresh_t video_cb;
retro_audio_sample_t audio_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_environment_t environ_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;
retro_log_printf_t log_cb;

void fallback_log(enum retro_log_level level, const char* fmt, ...)
{
    (void)level;
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

} // namespace

void retro_init(void)
{
    s_emu.boot();
}

void retro_deinit(void)
{
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info* info)
{
    memset(info, 0, sizeof(*info));
    info->library_name = "chip8";
    info->library_version = "v1";
    info->need_fullpath = false;
    info->valid_extensions = nullptr; // Anything is fine, we don't care.
}


void retro_get_system_av_info(struct retro_system_av_info* info)
{
    info->timing.fps = emu::fps;
    info->timing.sample_rate = emu::sample_rate;

    info->geometry.base_width = emu::video::width;
    info->geometry.base_height = emu::video::height;
    info->geometry.max_width = emu::video::width;
    info->geometry.max_height = emu::video::height;
    info->geometry.aspect_ratio = emu::video::aspect_ratio;
}

void retro_set_environment(retro_environment_t cb)
{
    environ_cb = cb;

    bool no_content = true;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

    retro_log_callback logging{};
    if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
        log_cb = logging.log;
    else
        log_cb = ::fallback_log;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
    input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}

static unsigned x_coord;
static unsigned y_coord;
static int mouse_rel_x;
static int mouse_rel_y;

void retro_reset(void)
{
    x_coord = 0;
    y_coord = 0;
}

static void update_input(void)
{
    input_poll_cb();
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
    {
        /* stub */
    }
}

static void render_checkered(void)
{
    uint32_t* buf = s_emu.framebuffer();
    unsigned stride = emu::video::width;
    uint32_t color_r = 0xff << 16;
    uint32_t color_g = 0xff << 8;
    uint32_t* line = buf;

    for (unsigned y = 0; y < emu::video::height; y++, line += stride)
    {
        unsigned index_y = ((y - y_coord) >> 4) & 1;
        for (unsigned x = 0; x < emu::video::width; x++)
        {
            unsigned index_x = ((x - x_coord) >> 4) & 1;
            line[x] = (index_y ^ index_x) ? color_r : color_g;
        }
    }

    for (unsigned y = mouse_rel_y - 5; y <= mouse_rel_y + 5; y++)
        for (unsigned x = mouse_rel_x - 5; x <= mouse_rel_x + 5; x++)
            buf[y * stride + x] = 0xff;

    video_cb(buf, emu::video::width, emu::video::height, stride << 2);
}

static void render_vram(void) {
    uint32_t* buf = s_emu.framebuffer();
    uint32_t color_w = 0xffffff;
    uint32_t color_b = 0x000000;
    
    constexpr auto vram = s_emu.vram_data();
    constexpr auto vram_size = emu::cpu::vram_t::size;
    constexpr auto bit = 8;

    for (unsigned y = 0; y < emu::video::height; y++) {
        for (unsigned x = 0; x < emu::video::width; x++) {
            auto index = x + y * emu::video::width;
            auto vindex = x / bit + y * (emu::video::width / bit);
            auto sub = 7 - x % bit;
            auto data = vram[vindex];
            buf[index] = (data & (1 << sub)) ? color_w : color_b;
        }
    }

    video_cb(buf, emu::video::width, emu::video::height, emu::video::width * sizeof(uint32_t));
}

static void check_variables(void)
{
}

static void audio_callback(void)
{
    audio_cb(0, 0);
}

void retro_run(void)
{
    update_input();
    //render_checkered();
    s_emu.cycle();
    render_vram();
    audio_callback();

    bool updated = false;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
        check_variables();
}

bool retro_load_game(const struct retro_game_info* info)
{
    auto fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    {
        log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
        return false;
    }

    check_variables();

    s_emu.load_rom(reinterpret_cast<const emu::data_t*>(info->data), info->size);

    return true;
}

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info* info, size_t num)
{
    if (type != 0x200)
        return false;
    if (num != 2)
        return false;
    return retro_load_game(NULL);
}

size_t retro_serialize_size(void)
{
    return 2;
}

bool retro_serialize(void* data_, size_t size)
{
    if (size < 2)
        return false;

    uint8_t* data = reinterpret_cast<uint8_t*>(data_);
    data[0] = x_coord;
    data[1] = y_coord;
    return true;
}

bool retro_unserialize(const void* data_, size_t size)
{
    if (size < 2)
        return false;

    const uint8_t* data = reinterpret_cast<const uint8_t*>(data_);
    x_coord = data[0] & 31;
    y_coord = data[1] & 31;
    return true;
}

void* retro_get_memory_data(unsigned id)
{
    (void)id;
    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    (void)id;
    return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char* code)
{
    (void)index;
    (void)enabled;
    (void)code;
}
