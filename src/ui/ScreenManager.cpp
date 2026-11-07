#include <algorithm>
#include <stdexcept>
#include "ui/ScreenManager.h"

//-------------------------------------------------
// Registration and switching
//-------------------------------------------------

void sg::chess::ui::ScreenManager::Add(const ScreenId t_id, std::unique_ptr<Screen> t_screen)
{
    if (!t_screen)
    {
        throw std::invalid_argument("Cannot add a null screen");
    }

    const bool exists{ std::ranges::any_of(m_entries, [t_id](const Entry& t_entry) { return t_entry.id == t_id; }) };
    if (exists)
    {
        throw std::invalid_argument("A screen with this id is already registered");
    }

    m_entries.push_back({ t_id, std::move(t_screen) });
}

void sg::chess::ui::ScreenManager::SwitchTo(const ScreenId t_id)
{
    const std::size_t index{ IndexOf(t_id) };
    if (m_current == index)
    {
        return;
    }

    if (m_current)
    {
        m_entries[*m_current].screen->OnExit();
    }

    m_current = index;
    m_entries[index].screen->OnEnter();
}

//-------------------------------------------------
// Query
//-------------------------------------------------

std::optional<sg::chess::ui::ScreenId> sg::chess::ui::ScreenManager::CurrentId() const noexcept
{
    if (!m_current)
    {
        return std::nullopt;
    }

    return m_entries[*m_current].id;
}

std::span<const sg::chess::ui::ScreenManager::Entry> sg::chess::ui::ScreenManager::Entries() const noexcept
{
    return m_entries;
}

//-------------------------------------------------
// Frame steps (forwarded to the active screen)
//-------------------------------------------------

void sg::chess::ui::ScreenManager::HandleEvent(const sf::Event& t_event)
{
    if (Screen* screen{ Active() })
    {
        screen->HandleEvent(t_event);
    }
}

void sg::chess::ui::ScreenManager::Update(const sf::Time t_dt)
{
    if (Screen* screen{ Active() })
    {
        screen->Update(t_dt);
    }
}

void sg::chess::ui::ScreenManager::Draw(sf::RenderTarget& t_target)
{
    if (Screen* screen{ Active() })
    {
        screen->Draw(t_target);
    }
}

void sg::chess::ui::ScreenManager::DrawGui()
{
    if (Screen* screen{ Active() })
    {
        screen->DrawGui();
    }
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

std::size_t sg::chess::ui::ScreenManager::IndexOf(const ScreenId t_id) const
{
    const auto it{ std::ranges::find_if(m_entries, [t_id](const Entry& t_entry) { return t_entry.id == t_id; }) };
    if (it == m_entries.end())
    {
        throw std::invalid_argument("No screen with this id is registered");
    }

    return static_cast<std::size_t>(it - m_entries.begin());
}

sg::chess::ui::Screen* sg::chess::ui::ScreenManager::Active() const noexcept
{
    return m_current ? m_entries[*m_current].screen.get() : nullptr;
}
