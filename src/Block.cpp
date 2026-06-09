#include "Block.h"

#include "Bonus.h"
#include "Game.h"

namespace {
const sf::Color kUnbreakableColor(95, 104, 123);
const sf::Color kBonusBlockColor(241, 196, 15);
const sf::Color kSpeedBlockColor(231, 76, 60);
const sf::Color kHealthBlockColor(46, 204, 113);
const sf::Color kOutlineColor(28, 33, 43);
}

Block::Block(const sf::FloatRect& rect)
    : m_rect(rect) {
}

const sf::FloatRect& Block::bounds() const {
    return m_rect;
}

bool Block::isAlive() const {
    return m_alive;
}

bool Block::isBreakable() const {
    return true;
}

void Block::destroy() {
    m_alive = false;
}

UnbreakableBlock::UnbreakableBlock(const sf::FloatRect& rect)
    : Block(rect) {
}

void UnbreakableBlock::onHit(Game&, Ball&) {
}

void UnbreakableBlock::draw(sf::RenderTarget& target, const sf::Font&, bool) const {
    sf::RectangleShape shape({m_rect.width, m_rect.height});
    shape.setPosition(m_rect.left, m_rect.top);
    shape.setFillColor(kUnbreakableColor);
    shape.setOutlineThickness(2.0f);
    shape.setOutlineColor(kOutlineColor);
    target.draw(shape);

    sf::RectangleShape stripe({m_rect.width - 12.0f, 4.0f});
    stripe.setPosition(m_rect.left + 6.0f, m_rect.top + m_rect.height * 0.5f - 2.0f);
    stripe.setFillColor(sf::Color(210, 215, 223));
    target.draw(stripe);
}

SpeedBlock::SpeedBlock(const sf::FloatRect& rect)
    : Block(rect) {
}

void SpeedBlock::onHit(Game& game, Ball& ball) {
    game.registerBlockHit();
    game.accelerateBall(ball, 1.12f);
    destroy();
    game.notifyBlockDestroyed();
}

void SpeedBlock::draw(sf::RenderTarget& target, const sf::Font&, bool) const {
    sf::RectangleShape shape({m_rect.width, m_rect.height});
    shape.setPosition(m_rect.left, m_rect.top);
    shape.setFillColor(kSpeedBlockColor);
    shape.setOutlineThickness(2.0f);
    shape.setOutlineColor(kOutlineColor);
    target.draw(shape);

    sf::ConvexShape arrow(3);
    arrow.setPoint(0, {m_rect.left + m_rect.width * 0.35f, m_rect.top + m_rect.height * 0.25f});
    arrow.setPoint(1, {m_rect.left + m_rect.width * 0.7f, m_rect.top + m_rect.height * 0.5f});
    arrow.setPoint(2, {m_rect.left + m_rect.width * 0.35f, m_rect.top + m_rect.height * 0.75f});
    arrow.setFillColor(sf::Color::White);
    target.draw(arrow);
}

HealthBlock::HealthBlock(const sf::FloatRect& rect, int health)
    : Block(rect)
    , m_health(health) {
}

void HealthBlock::onHit(Game& game, Ball&) {
    game.registerBlockHit();
    --m_health;
    if (m_health <= 0) {
        destroy();
        game.notifyBlockDestroyed();
    }
}

void HealthBlock::draw(sf::RenderTarget& target, const sf::Font& font, bool hasFont) const {
    sf::RectangleShape shape({m_rect.width, m_rect.height});
    shape.setPosition(m_rect.left, m_rect.top);
    sf::Color color = kHealthBlockColor;
    if (m_health >= 4) color = sf::Color(39, 174, 96);
    else if (m_health == 3) color = sf::Color(46, 204, 113);
    else if (m_health == 2) color = sf::Color(88, 214, 141);
    shape.setFillColor(color);
    shape.setOutlineThickness(2.0f);
    shape.setOutlineColor(kOutlineColor);
    target.draw(shape);

    if (!hasFont) {
        return;
    }

    sf::Text hp(std::to_string(m_health), font, 16);
    hp.setFillColor(sf::Color::White);
    const auto bounds = hp.getLocalBounds();
    hp.setPosition(
        m_rect.left + m_rect.width * 0.5f - (bounds.width * 0.5f + bounds.left),
        m_rect.top + m_rect.height * 0.5f - (bounds.height * 0.5f + bounds.top) - 1.0f
    );
    target.draw(hp);
}

BonusBlock::BonusBlock(const sf::FloatRect& rect, std::unique_ptr<FallingBonus> hiddenBonus)
    : Block(rect)
    , m_hiddenBonus(std::move(hiddenBonus)) {
}

BonusBlock::~BonusBlock() = default;

void BonusBlock::onHit(Game& game, Ball&) {
    game.registerBlockHit();
    destroy();
    game.notifyBlockDestroyed();
    if (m_hiddenBonus) {
        game.spawnBonus(m_hiddenBonus->cloneAt({m_rect.left + m_rect.width * 0.5f, m_rect.top + m_rect.height * 0.5f}));
    }
}

void BonusBlock::draw(sf::RenderTarget& target, const sf::Font&, bool) const {
    sf::RectangleShape shape({m_rect.width, m_rect.height});
    shape.setPosition(m_rect.left, m_rect.top);
    shape.setFillColor(kBonusBlockColor);
    shape.setOutlineThickness(2.0f);
    shape.setOutlineColor(kOutlineColor);
    target.draw(shape);

    if (m_hiddenBonus) {
        m_hiddenBonus->drawIcon(target, {m_rect.left + m_rect.width * 0.5f, m_rect.top + m_rect.height * 0.5f}, 0.75f);
    }
}
