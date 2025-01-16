#include <algorithm>
#include <array>
#include <iostream>
#include <tinyxml2.h>
#include <chrono>

#include "Labs/svg/CaseSVG.h"
#include "Labs/Common/ImGuiHelper.h"
#include "Labs/svg/render.h"

namespace VCX::Labs::Project {

    CaseSVG::CaseSVG():
    _texture({ .MinFilter = Engine::GL::FilterMode::Linear, .MagFilter = Engine::GL::FilterMode::Nearest }){}

    void CaseSVG::OnSetupPropsUI() {
        
        _recompute |= ImGui::SliderInt("SSAA Scale", &_scale, 1, 3);
        ImGui::Spacing();

        if (ImGui::BeginCombo("Demos", GetPath(_SVGIdx).c_str())) {
            for (std::size_t i = 0; i < Assets::ExampleSVGs.size(); ++i) {
                bool selected = i == _SVGIdx;
                if (ImGui::Selectable(GetPath(i).c_str(), selected)) {
                    if (! selected) {
                        _SVGIdx  = i;
                        _recompute = true;
                    }
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();

        std::string path = GetPath(_SVGIdx);
        path = path.substr(0, path.length() - 4) + ".png";
        Common::ImGuiHelper::SaveImage(path.c_str(), _texture, std::make_pair(_width, _height));
        ImGui::Spacing();

        ImGui::Checkbox("Zoom Tooltip", &_enableZoom);
        ImGui::Spacing();
    }

    Common::CaseRenderResult CaseSVG::OnRender(std::pair<std::uint32_t, std::uint32_t> const desiredSize) {

        int width = ((int)desiredSize.first - 100) * _scale;
        int height = ((int)desiredSize.second - 100) * _scale;

        if (width != _windowWidth || height != _windowHeight) {
            _windowWidth = width;
            _windowHeight = height;
            _recompute = true;
        }

        if (_recompute) {
            _recompute = false;
            _width = width;
            _height = height;

            auto superTex { Common::CreatePureImageRGB(100, 100, { 1., 1., 1. }) };

            tinyxml2::XMLDocument doc;
            if (doc.LoadFile(std::filesystem::path(Assets::ExampleSVGs[_SVGIdx]).string().c_str())) {
                std::cerr << "File loading error!" << GetPath(_SVGIdx) << std::endl;
            }
            else {
                auto const * root = doc.FirstChildElement("svg");
                renderImage(superTex, root, _width, _height, false);
            }

            _width /= _scale, _height /= _scale;
            auto downTex { Common::CreatePureImageRGB(_width, _height, { 1., 1., 1. }) };
            SSAA(_scale, downTex, superTex);
            _texture.Update(downTex);
        }

        return Common::CaseRenderResult {
            .Fixed     = true,
            .Image     = _texture,
            .ImageSize = { _width, _height }
        };
    }

    void CaseSVG::OnProcessInput(ImVec2 const & pos) {
        auto         window  = ImGui::GetCurrentWindow();
        bool         hovered = false;
        bool         held    = false;
        ImVec2 const delta   = ImGui::GetIO().MouseDelta;
        ImGui::ButtonBehavior(window->Rect(), window->GetID("##io"), &hovered, &held, ImGuiButtonFlags_MouseButtonLeft);
        if (held && delta.x != 0.f)
            ImGui::SetScrollX(window, window->Scroll.x - delta.x);
        if (held && delta.y != 0.f)
            ImGui::SetScrollY(window, window->Scroll.y - delta.y);
        if (_enableZoom && ! held && ImGui::IsItemHovered())
            Common::ImGuiHelper::ZoomTooltip(_texture, { _width, _height}, pos);
    }

    void CaseSVG::SSAA(int scale, ImageRGB& down, ImageRGB& super){
        int _width = down.GetSizeX(), _height = down.GetSizeY();
            for (int i = 0; i < _width; ++i)
                for(int j = 0; j < _height; ++j){
                    glm::vec3 color(0);
                    for (int x = 0; x < scale; ++x)
                        for (int y = 0; y < scale; ++y){
                            auto && proxy = super.At(j * scale + y, i * scale + x); 
                            color += static_cast<glm::vec3>(proxy);
                        }
                    color /= scale * scale;
                    down.At(i, j) = color;
                }
    }
}
