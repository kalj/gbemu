#include "gameboy.h"
#include "logging.h"

#include <fmt/core.h>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include <CLI/CLI.hpp>

#include <SDL2/SDL.h>

void audio_callback(void *user_data, Uint8 *raw_buffer, int bytes) {
    Sint16   *buffer   = (Sint16 *)raw_buffer;
    const int n_frames = bytes / gb_sound::N_CHANNELS / sizeof(int16_t);

    Gameboy *gb = (Gameboy *)user_data;
    gb->render_audio(buffer, n_frames);
}

int main(int argc, char **argv) {

    CLI::App app{"Gameboy Emulator"};

    std::filesystem::path rom_path;
    bool                  verbose = false;
    bool                  no_sdl  = false;
    app.add_option("cartridge_rom", rom_path, "Path to cartridge rom file")->required()->check(CLI::ExistingFile);
    app.add_flag("-v,--verbose", verbose, "Enable verbose log output");
    app.add_flag("-n,--nosdl", no_sdl, "Disable SDL2 video and sound rendering");

    const bool with_sdl = !no_sdl;

    CLI11_PARSE(app, argc, argv);

    if (!std::filesystem::exists(rom_path)) {
        fmt::print("No Cartridge ROM found at \"{}\"\n", rom_path.string());
        return 1;
    }

    std::ifstream infile(rom_path, std::ios_base::binary);

    std::vector<uint8_t> cartridge_rom_contents(std::istreambuf_iterator<char>(infile), {});

    logging::set_level(verbose ? logging::LogLevel::DEBUG : logging::LogLevel::WARNING);

    Gameboy gb{cartridge_rom_contents};
    gb.reset();

    gb.print_cartridge_info();

    SDL_Window   *window         = NULL;
    SDL_Renderer *renderer       = NULL;
    SDL_Texture  *screen_texture = NULL;
    if (with_sdl) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
            throw std::runtime_error(fmt::format("Failed to initialize SDL: {}", SDL_GetError()));
        }

        window = SDL_CreateWindow("GbEmu",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  LCD_WIDTH,
                                  LCD_HEIGHT,
                                  SDL_WINDOW_RESIZABLE);

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

        // Since we are going to display a low resolution buffer,
        // it is best to limit the window size so that it cannot
        // be smaller than our internal buffer size.
        SDL_SetWindowMinimumSize(window, LCD_WIDTH, LCD_HEIGHT);

        SDL_RenderSetLogicalSize(renderer, LCD_WIDTH, LCD_HEIGHT);
        SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

        screen_texture =
            SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, LCD_WIDTH, LCD_HEIGHT);

        SDL_AudioSpec desired;
        desired.freq     = gb_sound::SAMPLE_RATE; // number of samples per second
        desired.format   = AUDIO_S16SYS;          // sample type (here: signed short i.e. 16 bit)
        desired.channels = gb_sound::N_CHANNELS;
        desired.samples  = gb_sound::BLOCK_SIZE; // buffer-size
        desired.callback = audio_callback;
        desired.userdata = &gb;

        SDL_AudioSpec obtained;
        if (SDL_OpenAudio(&desired, &obtained) != 0) {
            SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s", SDL_GetError());
        }
        if (desired.format != obtained.format) {
            SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");
        }
        SDL_PauseAudio(0);
    }

    fmt::print("------------------------------------------------------\n");
    fmt::print("Starting execution\n\n");

    int  res = 0;
    auto tic = std::chrono::high_resolution_clock::now();
    try {
        bool running = true;
        int  i       = 0;
        while (running) {

            if (with_sdl) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        running = false;
                    } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                        const gb_controller::State new_state =
                            event.type == SDL_KEYDOWN ? gb_controller::State::DOWN : gb_controller::State::UP;

                        if (event.key.keysym.sym == SDLK_UP) {
                            gb.set_button_state(gb_controller::Button::UP, new_state);
                        } else if (event.key.keysym.sym == SDLK_DOWN) {
                            gb.set_button_state(gb_controller::Button::DOWN, new_state);
                        } else if (event.key.keysym.sym == SDLK_LEFT) {
                            gb.set_button_state(gb_controller::Button::LEFT, new_state);
                        } else if (event.key.keysym.sym == SDLK_RIGHT) {
                            gb.set_button_state(gb_controller::Button::RIGHT, new_state);
                        } else if (event.key.keysym.sym == SDLK_x) {
                            gb.set_button_state(gb_controller::Button::A, new_state);
                        } else if (event.key.keysym.sym == SDLK_z) {
                            gb.set_button_state(gb_controller::Button::B, new_state);
                        } else if (event.key.keysym.sym == SDLK_RETURN) {
                            gb.set_button_state(gb_controller::Button::START, new_state);
                        } else if (event.key.keysym.sym == SDLK_BACKSPACE) {
                            gb.set_button_state(gb_controller::Button::SELECT, new_state);
                        }
                    }
                }
            }

            // const auto cycles_to_execute = 154 * 456 * 4;
            const auto cycles_to_execute = 154 * 110 * 4;
            for (int j = 0; j < cycles_to_execute; j++) {
                gb.do_tick();
            }

            const auto nprint = 10;
            if ((i++) % nprint == 0) {
                auto toc      = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(toc - tic);
                fmt::print("Emulation frequency (M-cycles): {} MHz\n",
                           float(cycles_to_execute * nprint / 4) / duration.count());
                tic = toc;
            }

            if (with_sdl) {
                // It's a good idea to clear the screen every frame,
                // as artifacts may occur if the window overlaps with
                // other windows or transparent overlays.
                SDL_RenderClear(renderer);
                SDL_UpdateTexture(screen_texture, NULL, gb.get_pixel_buffer_data(), LCD_WIDTH * sizeof(uint32_t));
                SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
                SDL_RenderPresent(renderer);
            }
        }
    } catch (std::exception &e) {
        fmt::print("EXCEPTION CAUGHT: {}\n", e.what());
        res = 1;
    }

    if (with_sdl) {
        // SDL_CloseAudio();
        SDL_Quit();
    }

    auto state_file = "state.txt";
    fmt::print("Saving state to \"{}\"...\n", state_file);
    std::ofstream fs(state_file);
    gb.dump(fs);

    return res;
}
