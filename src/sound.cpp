#include "sound.h"

#include <fmt/core.h>
#include <cstdint>

#define REG_NR10 0x10
#define REG_NR11 0x11
#define REG_NR12 0x12
#define REG_NR13 0x13
#define REG_NR14 0x14
#define REG_NR21 0x16
#define REG_NR22 0x17
#define REG_NR23 0x18
#define REG_NR24 0x19
#define REG_NR30 0x1a
#define REG_NR31 0x1b
#define REG_NR32 0x1c
#define REG_NR33 0x1d
#define REG_NR34 0x1e
#define REG_NR41 0x20
#define REG_NR42 0x21
#define REG_NR43 0x22
#define REG_NR44 0x23
#define REG_NR50 0x24
#define REG_NR51 0x25
#define REG_NR52 0x26

#define COUNTER_WIDTH_7BITS  true
#define COUNTER_WIDTH_15BITS false

namespace gb_sound {

    void Sound::reset() {
        this->write_reg(REG_NR10, 0x80);
        this->write_reg(REG_NR11, 0xBF);
        this->write_reg(REG_NR12, 0xF3);
        this->write_reg(REG_NR14, 0xBF);
        this->write_reg(REG_NR21, 0x3F);
        this->write_reg(REG_NR22, 0x00);
        this->write_reg(REG_NR24, 0xBF);
        this->write_reg(REG_NR30, 0x7F);
        this->write_reg(REG_NR31, 0xFF);
        this->write_reg(REG_NR32, 0x9F);
        this->write_reg(REG_NR33, 0xBF);
        this->write_reg(REG_NR41, 0xFF);
        this->write_reg(REG_NR42, 0x00);
        this->write_reg(REG_NR43, 0x00);
        this->write_reg(REG_NR44, 0xBF);
        this->write_reg(REG_NR50, 0x77);
        this->write_reg(REG_NR51, 0xF3);
        this->write_reg(REG_NR52, 0xF1); //($F0-SGB)
    }

    void Sound::render(int16_t *buffer, int n_frames) {
        memset(buffer, 0, n_frames * N_CHANNELS * sizeof(int16_t));

        if (!this->master_on) {
            return;
        }

        const int16_t AMPLITUDE = 0x1000;

        // channel 1
        const float    ch1_frequency = float(131072) / (2048 - this->ch1_freq);
        const uint32_t ch1_dphase    = ch1_frequency / SAMPLE_RATE * UINT32_MAX;

        if (this->ch1_initial) {
            this->ch1_initial = false;
            this->ch1_active  = true;
        }

        if ((this->channel_matrix & 0x11) && this->ch1_active) {
            for (int i = 0; i < n_frames; i++) {
                buffer[N_CHANNELS * i + 0] = (this->ch1_phase < 0x80000000U) ? AMPLITUDE : -AMPLITUDE;
                this->ch1_phase += ch1_dphase;
            }
        }

        if (this->ch2_initial) {
            this->ch2_initial = false;
            this->ch2_active  = true;
        }

        // channel 2
        const float    ch2_frequency = float(131072) / (2048 - this->ch2_freq);
        const uint32_t ch2_dphase    = ch2_frequency / SAMPLE_RATE * UINT32_MAX;

        if ((this->channel_matrix & 0x22) && this->ch2_active) {
            for (int i = 0; i < n_frames; i++) {
                buffer[N_CHANNELS * i + 1] = (this->ch2_phase < 0x80000000U) ? AMPLITUDE : -AMPLITUDE;
                this->ch2_phase += ch2_dphase;
            }
        }

        // this->so1_vol+1
        // for(int i=0; i<n_frames; i++) {
        //     for(int ch=0; ch<N_CHANNELS; ch++) {
        //         buffer[N_CHANNELS*i + ch] = i<(n_frames/2) ? 0x1000: -0x1000;
        //     }
        // }
    }

    uint8_t Sound::read_reg(uint8_t regid) const {
        switch (regid) {
            case REG_NR10:
                return (this->ch1_sweep_time & 0x7) << 4 | (static_cast<uint8_t>(this->ch1_sweep_dir_decrease) << 3) |
                       (this->ch1_sweep_n_steps & 0x7);
            case REG_NR11:
                return ((this->ch1_duty & 0x3) << 6) | 0x3f;
            case REG_NR12:
                return (this->ch1_env_initial_vol & 0xf) << 4 |
                       (static_cast<uint8_t>(this->ch1_env_dir_increase) << 3) | (this->ch1_env_n_steps & 0x7);
            case REG_NR13:
                return 0xff; // Write Only
            case REG_NR14:
                return (static_cast<uint8_t>(this->ch1_counter_consecutive) << 6) | 0xbf;

            case REG_NR21:
                return ((this->ch2_duty & 0x3) << 6) | 0x3f;
            case REG_NR22:
                return (this->ch2_env_initial_vol & 0xf) << 4 |
                       (static_cast<uint8_t>(this->ch2_env_dir_increase) << 3) | (this->ch2_env_n_steps & 0x7);
            case REG_NR23:
                return 0xff; // Write Only
            case REG_NR24:
                return (static_cast<uint8_t>(this->ch2_counter_consecutive) << 6) | 0xbf;

            case REG_NR30:
                return (static_cast<uint8_t>(this->ch3_on) << 7) | 0x7f;
            case REG_NR31:
                return 0xff; // Write Only
            case REG_NR32:
                return ((this->ch3_level & 0x3) << 5) | 0x9f;
            case REG_NR33:
                return 0xff; // Write Only
            case REG_NR34:
                return (static_cast<uint8_t>(this->ch3_counter_consecutive) << 6) | 0xbf;

            case REG_NR41:
                return 0xff; // Write Only
            case REG_NR42:
                return (this->ch4_env_initial_vol & 0xf) << 4 |
                       (static_cast<uint8_t>(this->ch4_env_dir_increase) << 3) | (this->ch4_env_n_steps & 0x7);
            case REG_NR43:
                return (this->ch4_shift_clock & 0xf) << 4 | (static_cast<uint8_t>(this->ch4_counter_width) << 3) |
                       (this->ch4_div_ratio & 0x7);
            case REG_NR44:
                return (static_cast<uint8_t>(this->ch4_counter_consecutive) << 6) | 0xbf;

            case REG_NR50:
                return (this->so1_vol & 0x7) | ((this->so2_vol & 0x7) << 4);
            case REG_NR51:
                return this->channel_matrix;
            case REG_NR52:
                fmt::print("Read from Sound Controller Register ${:02X}\n", regid);
                return 0xff; //(static_cast<uint8_t>(this->master_on)<<7)|(this->)|()|()|()|0x70;

            // wave pattern ram
            case 0x30:
            case 0x31:
            case 0x32:
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x36:
            case 0x37:
            case 0x38:
            case 0x39:
            case 0x3a:
            case 0x3b:
            case 0x3c:
            case 0x3d:
            case 0x3e:
            case 0x3f:
                fmt::print("Read from Sound Controller Wave Pattern RAM: ${:02X}\n", regid);
                return 0xff;
            default:
                throw std::runtime_error(fmt::format("Invalid regid passed to Sound: {}", regid));
        }
    }

    void Sound::write_reg(uint8_t regid, uint8_t data) {
        switch (regid) {
            case REG_NR10:
                this->ch1_sweep_n_steps      = data & 0x7;
                this->ch1_sweep_dir_decrease = data & (1 << 3);
                this->ch1_sweep_time         = (data >> 4) & 0x7;
                break;
            case REG_NR11:
                this->ch1_duty   = (data >> 6) & 0x3;
                this->ch1_length = 64 - (data & 0x3f);
                break;
            case REG_NR12:
                this->ch1_env_n_steps      = data & 0x7;
                this->ch1_env_dir_increase = data & (1 << 3);
                this->ch1_env_initial_vol  = (data >> 4) & 0xf;
                break;
            case REG_NR13:
                this->ch1_freq = (this->ch1_freq & 0x700) | data;
                break;
            case REG_NR14:
                this->ch1_counter_consecutive = data & (1 << 6);
                if (data & (1 << 7)) {
                    this->ch1_initial = true;
                }
                break;

            case REG_NR21:
                this->ch2_duty   = (data >> 6) & 0x3;
                this->ch2_length = 64 - (data & 0x3f);
                break;
            case REG_NR22:
                this->ch2_env_n_steps      = data & 0x7;
                this->ch2_env_dir_increase = data & (1 << 3);
                this->ch2_env_initial_vol  = (data >> 4) & 0xf;
                break;
            case REG_NR23:
                this->ch2_freq = (this->ch2_freq & 0x700) | data;
                break;
            case REG_NR24:
                this->ch2_freq                = (static_cast<uint16_t>(data & 0x7) << 8) | (this->ch2_freq & 0xff);
                this->ch2_counter_consecutive = data & (1 << 6);
                if (data & (1 << 7)) {
                    this->ch2_initial = true;
                }
                break;

            case REG_NR30:
                this->ch3_on = data & 0x80;
                break;
            case REG_NR31:
                this->ch3_length = 256 - data;
                break;
            case REG_NR32:
                this->ch3_level = (data >> 5) & 0x3;
                break;
            case REG_NR33:
                this->ch3_freq = (this->ch3_freq & 0x700) | data;
                break;
            case REG_NR34:
                this->ch3_freq                = (static_cast<uint16_t>(data & 0x7) << 8) | (this->ch3_freq & 0xff);
                this->ch3_counter_consecutive = data & (1 << 6);
                if (data & (1 << 7)) {
                    this->ch3_initial = true;
                }
                break;

            case REG_NR41:
                this->ch4_length = 64 - (data & 0x3f);
                break;
            case REG_NR42:
                this->ch4_env_n_steps      = data & 0x7;
                this->ch4_env_dir_increase = data & (1 << 3);
                this->ch4_env_initial_vol  = (data >> 4) & 0xf;
                break;
            case REG_NR43:
                this->ch4_div_ratio     = data & 0x7;
                this->ch4_counter_width = (data & (1 << 3)) ? COUNTER_WIDTH_7BITS : COUNTER_WIDTH_15BITS;
                this->ch4_shift_clock   = (data >> 4) & 0xf;
                break;
            case REG_NR44:
                this->ch4_counter_consecutive = data & (1 << 6);
                if (data & (1 << 7)) {
                    this->ch4_initial = true;
                }
                break;

            case REG_NR50:
                this->so1_vol = data & 0x7;
                this->so2_vol = (data >> 4) & 0x7;
                break;
            case REG_NR51:
                this->channel_matrix = data;
                break;
            case REG_NR52:
                this->master_on = data & 0x80;
                break;

            // wave pattern ram
            case 0x30:
            case 0x31:
            case 0x32:
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x36:
            case 0x37:
            case 0x38:
            case 0x39:
            case 0x3a:
            case 0x3b:
            case 0x3c:
            case 0x3d:
            case 0x3e:
            case 0x3f:
                break;

                // break;
            default:
                throw std::runtime_error(fmt::format("Invalid regid passed to Sound: {}", regid));
        }
    }

    void Sound::dump(std::ostream &os) const {
        os << fmt::format("Sound not implemented...\n");
    }

} // namespace gb_sound
