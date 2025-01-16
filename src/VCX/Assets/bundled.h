#pragma once

#include <array>
#include <string_view>

namespace VCX::Assets {
    inline constexpr auto DefaultIcons {
        std::to_array<std::string_view>({
            "assets/images/vcl-logo-32x32.png",
            "assets/images/vcl-logo-48x48.png",
        })
    };

    inline constexpr auto DefaultFonts {
        std::to_array<std::string_view>({
            "assets/fonts/Ubuntu.ttf",
            "assets/fonts/UbuntuMono.ttf",
        })
    };

    inline constexpr auto ExampleSVGs {
        std::to_array<std::string_view>({
            "assets/svgs/pikachu.svg",
            "assets/svgs/DarthVader.svg",
            "assets/svgs/github.svg",
            "assets/svgs/kfc.svg",
            "assets/svgs/luffy.svg",
            "assets/svgs/animeGirl.svg",
            "assets/svgs/mask.svg",
            "assets/svgs/manga.svg",
            "assets/svgs/fox.svg",
            "assets/svgs/chinaFlag.svg",
        })
    };
}
