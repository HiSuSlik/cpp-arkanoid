#include "Bonus.h"

#include "Game.h"

#include <SFML/Graphics.hpp>
#include <array>
#include <cmath>
#include <cstring>

namespace {
constexpr float kPi = 3.1415926535f;

sf::String utf8(const char* s) {
    return sf::String::fromUtf8(s, s + std::strlen(s));
}
}

FallingBonus::FallingBonus(const sf::Vector2f& position, float radius)
    : m_position(position)
    , m_radius(radius) {
}

void FallingBonus::update(float dt, float fallSpeed) {
    m_position.y += fallSpeed * dt;
}

bool FallingBonus::isActive() const {
    return m_active;
}

void FallingBonus::deactivate() {
    m_active = false;
}

float FallingBonus::radius() const {
    return m_radius;
}

const sf::Vector2f& FallingBonus::position() const {
    return m_position;
}

sf::FloatRect FallingBonus::bounds() const {
    return {m_position.x - m_radius, m_position.y - m_radius, m_radius * 2.0f, m_radius * 2.0f};
}

void FallingBonus::draw(sf::RenderTarget& target) const {
    sf::CircleShape bubble(m_radius);
    bubble.setOrigin(m_radius, m_radius);
    bubble.setPosition(m_position);
    bubble.setFillColor(color());
    bubble.setOutlineThickness(2.0f);
    bubble.setOutlineColor(sf::Color(36, 40, 48));
    target.draw(bubble);
    drawIcon(target, m_position, 1.0f);
}

PaddleGrowBonus::PaddleGrowBonus(const sf::Vector2f& position) : FallingBonus(position) {}
void PaddleGrowBonus::apply(Game& game) const { game.growPaddle(32.0f); }
sf::String PaddleGrowBonus::label() const { return utf8("каретка +"); }
sf::Color PaddleGrowBonus::color() const { return sf::Color(46, 204, 113); }
std::unique_ptr<FallingBonus> PaddleGrowBonus::cloneAt(const sf::Vector2f& position) const { return std::make_unique<PaddleGrowBonus>(position); }

PaddleShrinkBonus::PaddleShrinkBonus(const sf::Vector2f& position) : FallingBonus(position) {}
void PaddleShrinkBonus::apply(Game& game) const { game.shrinkPaddle(24.0f); }
sf::String PaddleShrinkBonus::label() const { return utf8("каретка -"); }
sf::Color PaddleShrinkBonus::color() const { return sf::Color(231, 76, 60); }
std::unique_ptr<FallingBonus> PaddleShrinkBonus::cloneAt(const sf::Vector2f& position) const { return std::make_unique<PaddleShrinkBonus>(position); }

BallSpeedUpBonus::BallSpeedUpBonus(const sf::Vector2f& position) : FallingBonus(position) {}
void BallSpeedUpBonus::apply(Game& game) const { game.multiplyBallSpeeds(1.18f); }
sf::String BallSpeedUpBonus::label() const { return utf8("скорость +"); }
sf::Color BallSpeedUpBonus::color() const { return sf::Color(155, 89, 182); }
std::unique_ptr<FallingBonus> BallSpeedUpBonus::cloneAt(const sf::Vector2f& position) const { return std::make_unique<BallSpeedUpBonus>(position); }

BallSlowDownBonus::BallSlowDownBonus(const sf::Vector2f& position) : FallingBonus(position) {}
void BallSlowDownBonus::apply(Game& game) const { game.multiplyBallSpeeds(0.84f); }
sf::String BallSlowDownBonus::label() const { return utf8("скорость -"); }
sf::Color BallSlowDownBonus::color() const { return sf::Color(241, 196, 15); }
std::unique_ptr<FallingBonus> BallSlowDownBonus::cloneAt(const sf::Vector2f& position) const { return std::make_unique<BallSlowDownBonus>(position); }

StickyBonus::StickyBonus(const sf::Vector2f& position) : FallingBonus(position) {}
void StickyBonus::apply(Game& game) const { game.enableSticky(14.0f); }
sf::String StickyBonus::label() const { return utf8("липкая каретка"); }
sf::Color StickyBonus::color() const { return sf::Color(52, 152, 219); }
std::unique_ptr<FallingBonus> StickyBonus::cloneAt(const sf::Vector2f& position) const { return std::make_unique<StickyBonus>(position); }

BottomShieldBonus::BottomShieldBonus(const sf::Vector2f& position) : FallingBonus(position) {}
void BottomShieldBonus::apply(Game& game) const { game.enableBottomShield(); }
sf::String BottomShieldBonus::label() const { return utf8("одноразовое дно"); }
sf::Color BottomShieldBonus::color() const { return sf::Color(39, 174, 96); }
std::unique_ptr<FallingBonus> BottomShieldBonus::cloneAt(const sf::Vector2f& position) const { return std::make_unique<BottomShieldBonus>(position); }

SecondBallBonus::SecondBallBonus(const sf::Vector2f& position) : FallingBonus(position) {}
void SecondBallBonus::apply(Game& game) const { game.spawnSecondBallFromActive(); }
sf::String SecondBallBonus::label() const { return utf8("второй мяч"); }
sf::Color SecondBallBonus::color() const { return sf::Color(231, 76, 60); }
std::unique_ptr<FallingBonus> SecondBallBonus::cloneAt(const sf::Vector2f& position) const { return std::make_unique<SecondBallBonus>(position); }

void PaddleGrowBonus::drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const {
    const float s = 10.0f * scale;
    sf::RectangleShape h({s * 1.5f, 3.0f * scale});
    h.setOrigin(h.getSize() * 0.5f);
    h.setPosition(center);
    h.setFillColor(sf::Color::White);
    target.draw(h);

    sf::RectangleShape v({3.0f * scale, s * 1.5f});
    v.setOrigin(v.getSize() * 0.5f);
    v.setPosition(center);
    v.setFillColor(sf::Color::White);
    target.draw(v);
}

void PaddleShrinkBonus::drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const {
    const float s = 10.0f * scale;
    sf::RectangleShape h({s * 1.5f, 3.0f * scale});
    h.setOrigin(h.getSize() * 0.5f);
    h.setPosition(center);
    h.setFillColor(sf::Color::White);
    target.draw(h);
}

static void drawArrowIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale, bool withBar) {
    const float s = 10.0f * scale;
    sf::ConvexShape arrow(3);
    arrow.setPoint(0, {center.x - s * 0.5f, center.y - s * 0.55f});
    arrow.setPoint(1, {center.x + s * 0.7f, center.y});
    arrow.setPoint(2, {center.x - s * 0.5f, center.y + s * 0.55f});
    arrow.setFillColor(sf::Color::White);
    target.draw(arrow);

    if (withBar) {
        sf::RectangleShape bar({3.0f * scale, s * 1.4f});
        bar.setOrigin(bar.getSize() * 0.5f);
        bar.setPosition(center.x - s * 0.8f, center.y);
        bar.setFillColor(sf::Color::White);
        target.draw(bar);
    }
}

void BallSpeedUpBonus::drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const {
    drawArrowIcon(target, center, scale, false);
}

void BallSlowDownBonus::drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const {
    drawArrowIcon(target, center, scale, true);
}

void StickyBonus::drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const {
    const float s = 10.0f * scale;
    sf::CircleShape drop(s * 0.45f, 20);
    drop.setOrigin(drop.getRadius(), drop.getRadius());
    drop.setScale(1.0f, 1.25f);
    drop.setPosition(center.x, center.y + 1.5f * scale);
    drop.setFillColor(sf::Color::White);
    target.draw(drop);
}

void BottomShieldBonus::drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const {
    const float s = 10.0f * scale;
    sf::RectangleShape line({s * 1.6f, 3.0f * scale});
    line.setOrigin(line.getSize() * 0.5f);
    line.setPosition(center.x, center.y + s * 0.35f);
    line.setFillColor(sf::Color::White);
    target.draw(line);

    sf::RectangleShape top({s * 1.2f, 3.0f * scale});
    top.setOrigin(top.getSize() * 0.5f);
    top.setPosition(center.x, center.y - s * 0.15f);
    top.setFillColor(sf::Color::White);
    target.draw(top);
}

void SecondBallBonus::drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const {
    const float s = 10.0f * scale;
    sf::CircleShape c1(s * 0.32f, 20);
    c1.setOrigin(c1.getRadius(), c1.getRadius());
    c1.setPosition(center.x - s * 0.35f, center.y);
    c1.setFillColor(sf::Color::White);
    target.draw(c1);

    sf::CircleShape c2(s * 0.32f, 20);
    c2.setOrigin(c2.getRadius(), c2.getRadius());
    c2.setPosition(center.x + s * 0.35f, center.y);
    c2.setFillColor(sf::Color::White);
    target.draw(c2);
}

std::unique_ptr<FallingBonus> BonusFactory::createRandom(std::mt19937& rng, const sf::Vector2f& position) {
    std::uniform_int_distribution<int> dist(0, 6);
    switch (dist(rng)) {
        case 0: return std::make_unique<PaddleGrowBonus>(position);
        case 1: return std::make_unique<PaddleShrinkBonus>(position);
        case 2: return std::make_unique<BallSpeedUpBonus>(position);
        case 3: return std::make_unique<BallSlowDownBonus>(position);
        case 4: return std::make_unique<StickyBonus>(position);
        case 5: return std::make_unique<BottomShieldBonus>(position);
        case 6:
        default: return std::make_unique<SecondBallBonus>(position);
    }
}

std::vector<std::unique_ptr<FallingBonus>> BonusFactory::createLegendSamples() {
    std::vector<std::unique_ptr<FallingBonus>> items;
    items.push_back(std::make_unique<PaddleGrowBonus>(sf::Vector2f{}));
    items.push_back(std::make_unique<PaddleShrinkBonus>(sf::Vector2f{}));
    items.push_back(std::make_unique<BallSpeedUpBonus>(sf::Vector2f{}));
    items.push_back(std::make_unique<BallSlowDownBonus>(sf::Vector2f{}));
    items.push_back(std::make_unique<StickyBonus>(sf::Vector2f{}));
    items.push_back(std::make_unique<BottomShieldBonus>(sf::Vector2f{}));
    items.push_back(std::make_unique<SecondBallBonus>(sf::Vector2f{}));
    return items;
}
