#include "gameboy.h"
#include "log.h"

#include <SDL2/SDL.h>
#include <fmt/core.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include <csignal>

int main(int argc, char **argv) {

    if (argc < 2) {
        fmt::print("Cartridge ROM path expected\n");
        return 1;
    }

    auto cartridge_rom_file = std::filesystem::path(argv[1]);

    if (!std::filesystem::exists(cartridge_rom_file)) {
        fmt::print("No Cartridge ROM found at \"{}\"\n", cartridge_rom_file.string());
        return 1;
    }

    std::ifstream infile(cartridge_rom_file, std::ios_base::binary);

    std::vector<uint8_t> cartridge_rom_contents(std::istreambuf_iterator<char>(infile), {});

    Gameboy gb{cartridge_rom_contents};
    gb.reset();

    gb.print_header();

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow("GbEmu",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          LCD_WIDTH,
                                          LCD_HEIGHT,
                                          SDL_WINDOW_RESIZABLE);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

    // Since we are going to display a low resolution buffer,
    // it is best to limit the window size so that it cannot
    // be smaller than our internal buffer size.
    SDL_SetWindowMinimumSize(window, LCD_WIDTH, LCD_HEIGHT);

    SDL_RenderSetLogicalSize(renderer, LCD_WIDTH, LCD_HEIGHT);
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

    SDL_Texture *screen_texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, LCD_WIDTH, LCD_HEIGHT);

    log_set_enable(false);
    fmt::print("------------------------------------------------------\n");
    fmt::print("Starting execution\n\n");

    int res=0;

    try {
        bool running = true;
        int i=0;
        while(running) {

            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT ||
                    (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_q || event.key.keysym.sym == SDLK_ESCAPE))) {
                    running = false;
                }
            }

            const auto cycles_to_execute = 1000;
            for(int j=0; j<cycles_to_execute; j++,i++) {
                // log(fmt::format(" === [CPU cycle = {}]\n", i));
                gb.do_tick();
            }

            // It's a good idea to clear the screen every frame,
            // as artifacts may occur if the window overlaps with
            // other windows or transparent overlays.
            SDL_RenderClear(renderer);
            SDL_UpdateTexture(screen_texture, NULL, gb.get_pixel_buffer_data(), LCD_WIDTH * sizeof(uint32_t));
            SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    } catch (std::exception &e) {
        fmt::print("EXCEPTION CAUGHT: {}\n", e.what());
        res = 1;
    }

    SDL_Quit();

    auto state_file = "state.txt";
    fmt::print("Saving state to \"{}\"...\n", state_file);
    std::ofstream fs(state_file);
    gb.dump(fs);

    return res;
}
