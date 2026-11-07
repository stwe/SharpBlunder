#include <imgui-SFML.h>
#include <imgui.h>
#include <stdexcept>
#include <string>
#include "ui/GuiLayer.h"

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::GuiLayer::GuiLayer(Window& t_window, const std::filesystem::path& t_fontFile, const float t_fontSize)
    : m_window(t_window)
{
    // The destructor does not run if the constructor throws, so ImGui must be shut down by hand on every error.
    const auto fail = [](const std::string& t_message)
    {
        ImGui::SFML::Shutdown();
        throw std::runtime_error(t_message);
    };

    // 'false': do not load Dear ImGui's default font, we load our own below.
    if (!ImGui::SFML::Init(m_window.GetRenderWindow(), false))
    {
        throw std::runtime_error("Cannot initialize ImGui-SFML");
    }

    ImGuiIO& io{ ImGui::GetIO() };

    // Without this, ImGui writes an "imgui.ini" with window positions into the working directory.
    io.IniFilename = nullptr;

    if (!std::filesystem::exists(t_fontFile))
    {
        fail("Font file not found: " + t_fontFile.string());
    }

    io.Fonts->Clear();

    // The default glyph range covers Basic Latin and Latin-1 Supplement, which includes umlauts.
    if (io.Fonts->AddFontFromFileTTF(t_fontFile.string().c_str(), t_fontSize, nullptr, io.Fonts->GetGlyphRangesDefault()) == nullptr)
    {
        fail("Cannot load font: " + t_fontFile.string());
    }

    if (!ImGui::SFML::UpdateFontTexture())
    {
        fail("Cannot create the font texture");
    }
}

sg::chess::ui::GuiLayer::~GuiLayer() noexcept
{
    ImGui::SFML::Shutdown();
}

//-------------------------------------------------
// Frame steps
//-------------------------------------------------

void sg::chess::ui::GuiLayer::ProcessEvent(const sf::Event& t_event)
{
    ImGui::SFML::ProcessEvent(m_window.GetRenderWindow(), t_event);
}

bool sg::chess::ui::GuiLayer::WantsEvent(const sf::Event& t_event) const
{
    const ImGuiIO& io{ ImGui::GetIO() };

    const bool isMouseEvent{ t_event.is<sf::Event::MouseMoved>() ||
                             t_event.is<sf::Event::MouseButtonPressed>() ||
                             t_event.is<sf::Event::MouseButtonReleased>() ||
                             t_event.is<sf::Event::MouseWheelScrolled>() };

    const bool isKeyboardEvent{ t_event.is<sf::Event::KeyPressed>() ||
                                t_event.is<sf::Event::KeyReleased>() ||
                                t_event.is<sf::Event::TextEntered>() };

    return (isMouseEvent && io.WantCaptureMouse) || (isKeyboardEvent && io.WantCaptureKeyboard);
}

void sg::chess::ui::GuiLayer::BeginFrame(const sf::Time t_dt)
{
    ImGui::SFML::Update(m_window.GetRenderWindow(), t_dt);
}

void sg::chess::ui::GuiLayer::Render()
{
    ImGui::SFML::Render(m_window.GetRenderWindow());
}
