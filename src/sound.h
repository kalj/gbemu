#ifndef SOUND_H
#define SOUND_H

#include <cstdint>
#include <iosfwd>

#define N_CHANNELS  2
#define SAMPLE_RATE 44100

class Sound {
public:
    void reset();

    void render(int16_t *buffer, int n_frames);

    uint8_t read_reg(uint8_t regid) const;
    void    write_reg(uint8_t regid, uint8_t data);
    void    dump(std::ostream &os) const;

private:
    //---------------------------------------------------------------
    // channel 1 (tone & sweep)
    //---------------------------------------------------------------
    // properties
    uint8_t  ch1_sweep_n_steps{0};
    bool     ch1_sweep_dir_decrease{false};
    uint8_t  ch1_sweep_time{0};
    uint8_t  ch1_duty{2};
    uint8_t  ch1_length{0};
    uint8_t  ch1_env_n_steps{0};
    bool     ch1_env_dir_increase{false};
    uint8_t  ch1_env_initial_vol{0};
    uint16_t ch1_freq{0};
    bool     ch1_counter_consecutive{false};
    bool     ch1_initial{false};
    // state
    uint32_t ch1_phase{0};
    bool     ch1_active{false};

    //---------------------------------------------------------------
    // channel 2 (tone)
    //---------------------------------------------------------------
    // properties
    uint8_t  ch2_duty{2};
    uint8_t  ch2_length{0};
    bool     ch2_counter_consecutive{false};
    bool     ch2_initial{false};
    bool     ch2_env_dir_increase{false};
    uint8_t  ch2_env_initial_vol{0};
    uint8_t  ch2_env_n_steps{0};
    uint16_t ch2_freq{0};
    // state
    uint32_t ch2_phase{0};
    bool     ch2_active{false};

    //---------------------------------------------------------------
    // channel 3 (wave output)
    //---------------------------------------------------------------
    // properties
    bool     ch3_on{false};
    uint8_t  ch3_length{0};
    bool     ch3_counter_consecutive{false};
    bool     ch3_initial{false};
    uint16_t ch3_freq{0};
    uint8_t  ch3_level{0};

    //---------------------------------------------------------------
    // channel 4 (noise)
    //---------------------------------------------------------------
    // properties
    uint8_t ch4_length{0};
    uint8_t ch4_env_n_steps{0};
    bool    ch4_env_dir_increase{false};
    uint8_t ch4_env_initial_vol{0};
    uint8_t ch4_div_ratio{0};
    bool    ch4_counter_width{false};
    uint8_t ch4_shift_clock{0};
    bool    ch4_counter_consecutive{0};
    bool    ch4_initial{false};

    //---------------------------------------------------------------
    // master properties
    //---------------------------------------------------------------
    bool    master_on{true};
    uint8_t so1_vol{0};
    uint8_t so2_vol{0};
    uint8_t channel_matrix{0};
};

#endif /* SOUND_H */
