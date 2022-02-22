#include "gameboy.h"
#include "log.h"

#include <fmt/core.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include <csignal>

#include <CLI/CLI.hpp>

#include <SDL2/SDL.h>

int main(int argc, char **argv) {

    CLI::App app{"Gameboy Emulator"};

    std::filesystem::path rom_path;
    bool verbose = false;
    bool nowin = false;
    app.add_option("cartridge_rom", rom_path, "Path to cartridge rom file")->required()->check(CLI::ExistingFile);
    app.add_flag("-v,--verbose", verbose, "Enable verbose log output");
    app.add_flag("-n,--nowin", nowin, "Disable launching SDL2 window");

    CLI11_PARSE(app, argc, argv);

    if (!std::filesystem::exists(rom_path)) {
        fmt::print("No Cartridge ROM found at \"{}\"\n", rom_path.string());
        return 1;
    }

    std::ifstream infile(rom_path, std::ios_base::binary);

    std::vector<uint8_t> cartridge_rom_contents(std::istreambuf_iterator<char>(infile), {});

    if(verbose) {
        log_set_enable(true);
    }

    Gameboy gb{cartridge_rom_contents};
    gb.reset();

    gb.print_header();

    SDL_Window *window          = NULL;
    SDL_Renderer *renderer      = NULL;
    SDL_Texture *screen_texture = NULL;
    if (!nowin) {
        SDL_Init(SDL_INIT_VIDEO);

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
    }

    fmt::print("------------------------------------------------------\n");
    fmt::print("Starting execution\n\n");

    int res=0;

    try {
        bool running = true;
        int i=0;
        while(running) {

            if(!nowin) {
                SDL_Event event;
                while (SDL_PollEvent(&event))
                {
                    if (event.type == SDL_QUIT ||
                        (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_q || event.key.keysym.sym == SDLK_ESCAPE))) {
                        running = false;
                    }
                }
            }

            const auto cycles_to_execute = 154*456;
            for(int j=0; j<cycles_to_execute; j++,i++) {
                // log(fmt::format(" === [CPU cycle = {}]\n", i));
                gb.do_tick();
            }

            if(!nowin) {
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

    if(!nowin) {
        SDL_Quit();
    }

    auto state_file = "state.txt";
    fmt::print("Saving state to \"{}\"...\n", state_file);
    std::ofstream fs(state_file);
    gb.dump(fs);

    return res;
}
