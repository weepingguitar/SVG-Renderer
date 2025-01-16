#include "Assets/bundled.h"
#include "Labs/svg/App.h"

int main() {
    using namespace VCX;
    return Engine::RunApp<Labs::Project::App>(Engine::AppContextOptions {
        .Title      = "SVG Renderer",
        .WindowSize = { 1024, 768 },
        .FontSize   = 16,

        .IconFileNames = Assets::DefaultIcons,
        .FontFileNames = Assets::DefaultFonts,
    });
}
