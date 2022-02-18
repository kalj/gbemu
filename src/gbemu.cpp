#include "gameboy.h"
#include "log.h"

#include <fmt/core.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
// #include <SDL2/SDL.h>


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

    // SDL_Init(SDL_INIT_VIDEO);

    // SDL_Window * window = SDL_CreateWindow("",
    //                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    //                                        LCD_WIDTH*2, LCD_HEIGHT*2,
    //                                        SDL_WINDOW_RESIZABLE);

    // SDL_Renderer * renderer = SDL_CreateRenderer(window,
    //                                              -1, SDL_RENDERER_PRESENTVSYNC);

    // // Since we are going to display a low resolution buffer,
    // // it is best to limit the window size so that it cannot
    // // be smaller than our internal buffer size.
    // SDL_SetWindowMinimumSize(window, width, height);

    // SDL_RenderSetLogicalSize(renderer, width, height);
    // SDL_RenderSetIntegerScale(renderer, 1);

    // SDL_Texture * screen_texture = SDL_CreateTexture(renderer,
    //                                                  SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
    //                                                  LCD_WIDTH, LCD_HEIGHT);

    log_set_enable(false);
    fmt::print("------------------------------------------------------\n");
    fmt::print("Starting execution\n\n");

    try {
        for(int i=0;;i++) {

            log(fmt::format(" === [CPU cycle = {}]\n", i));
            gb.do_tick();

            // if(i%(70224/4) == 0) {
            //     // It's a good idea to clear the screen every frame,
            //     // as artifacts may occur if the window overlaps with
            //     // other windows or transparent overlays.
            //     SDL_RenderClear(renderer);
            //     SDL_UpdateTexture(screen_texture, NULL, gb.get_pixel_buffer(), LCD_WIDTH * sizeof(uint32_t));
            //     SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
            //     SDL_RenderPresent(renderer);
            // }
        }
    } catch(std::exception &e) {
        fmt::print("EXCEPTION CAUGHT: {}\n", e.what());
        auto state_file = "state.txt";
        fmt::print("Saving state to \"{}\"...\n", state_file);
        std::ofstream fs(state_file);
        gb.dump(fs);
        return 1;
    }

    return 0;
}
