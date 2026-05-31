#include "Game.h"
#include "Config.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <cstring>

namespace {
    const sf::Color kBackgroundColor(228, 231, 236);
    const sf::Color kPlayfieldColor(49, 57, 72);
    const sf::Color kCellColor(67, 76, 94);
    const sf::Color kSidebarColor(240, 240, 242);
    const sf::Color kPanelBorder(175, 180, 188);
    const sf::Color kTextColor(40, 44, 54);
    const sf::Color kSoftTextColor(84, 89, 99);
    const sf::Color kShieldColor(70, 160, 255);
    const sf::Color kPaddleColor(54, 63, 79);
    const sf::Color kPaddleAccent(120, 214, 255);
    const sf::Color kBallColor(245, 247, 252);
    const sf::Color kUnbreakableColor(95, 104, 123);
    const sf::Color kBonusBlockColor(241, 196, 15);
    const sf::Color kSpeedBlockColor(231, 76, 60);
    const sf::Color kHealthBlockColor(46, 204, 113);

    sf::String utf8(const char* s) {
        return sf::String::fromUtf8(s, s + std::strlen(s));
    }

    sf::Color colorForBonusType(Game::BonusType type) {
        switch (type) {
            case Game::BonusType::PaddleGrow: return sf::Color(46, 204, 113);
            case Game::BonusType::PaddleShrink: return sf::Color(231, 76, 60);
            case Game::BonusType::BallSpeedUp: return sf::Color(155, 89, 182);
            case Game::BonusType::BallSlowDown: return sf::Color(241, 196, 15);
            case Game::BonusType::Sticky: return sf::Color(52, 152, 219);
            case Game::BonusType::BottomShield: return sf::Color(46, 204, 113);
            case Game::BonusType::SecondBall: return sf::Color(231, 76, 60);
            case Game::BonusType::None:
            default: return sf::Color(160, 168, 180);
        }
    }
}

Game::Game()
    : m_window(sf::VideoMode(cfg::WindowWidth, cfg::WindowHeight), "ARKANOID (SFML)")
    , m_rng(std::random_device{}())
{
    m_window.setFramerateLimit(60);

    m_hasFont = m_font.loadFromFile("C:/Windows/Fonts/arial.ttf") ||
                m_font.loadFromFile("C:/Windows/Fonts/segoeui.ttf");

    m_paddle.rect = { (cfg::PlayfieldWidth - cfg::PaddleBaseWidth) * 0.5f, cfg::PaddleY, cfg::PaddleBaseWidth, cfg::PaddleHeight };
    m_paddle.speed = cfg::PaddleSpeed;
    m_paddle.targetWidth = cfg::PaddleBaseWidth;

    createLevel();
    resetRound(false);
}

void Game::run() {
    sf::Clock clock;
    while (m_window.isOpen()) {
        processEvents();
        const float dt = clock.restart().asSeconds();
        if (!m_pause) {
            update(std::min(dt, 0.033f));
        }
        render();
    }
}

void Game::processEvents() {
    sf::Event event{};
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
        } else if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                m_window.close();
            } else if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) {
                m_leftPressed = true;
            } else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) {
                m_rightPressed = true;
            } else if (event.key.code == sf::Keyboard::Space) {
                launchStuckBalls();
            } else if (event.key.code == sf::Keyboard::P) {
                m_pause = !m_pause;
            } else if (event.key.code == sf::Keyboard::R) {
                createLevel();
                resetRound(false);
            }
        } else if (event.type == sf::Event::KeyReleased) {
            if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) {
                m_leftPressed = false;
            } else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) {
                m_rightPressed = false;
            }
        }
    }
}

void Game::update(float dt) {
    updatePaddle(dt);
    updateStickyState(dt);
    updateBalls(dt);
    updateBonuses(dt);
    handleBallLosses();

    if (m_remainingBreakable <= 0) {
        createLevel();
        resetRound(true);
    }
}

void Game::render() {
    m_window.clear(kBackgroundColor);

    sf::RectangleShape field({ cfg::PlayfieldWidth, static_cast<float>(cfg::WindowHeight) });
    field.setPosition(0.0f, 0.0f);
    field.setFillColor(kPlayfieldColor);
    m_window.draw(field);

    sf::RectangleShape sidebar({ cfg::SidebarWidth - 24.0f, static_cast<float>(cfg::WindowHeight) - 40.0f });
    sidebar.setPosition(cfg::PlayfieldWidth + 12.0f, 20.0f);
    sidebar.setFillColor(kSidebarColor);
    sidebar.setOutlineThickness(2.0f);
    sidebar.setOutlineColor(kPanelBorder);
    m_window.draw(sidebar);

    drawBlocks();
    drawBonuses();
    drawShield();
    drawPaddle();
    drawBalls();
    drawSidebar();

    m_window.display();
}

void Game::createLevel() {
    m_blocks.clear();
    m_fallingBonuses.clear();
    m_remainingBreakable = 0;

    const float usableWidth = cfg::PlayfieldWidth - 2.0f * cfg::BlockMarginX - (cfg::BlockCols - 1) * cfg::BlockGap;
    const float blockWidth = usableWidth / static_cast<float>(cfg::BlockCols);

    std::uniform_int_distribution<int> bonusChance(0, 99);
    std::uniform_int_distribution<int> healthDist(2, 4);

    for (int row = 0; row < cfg::BlockRows; ++row) {
        for (int col = 0; col < cfg::BlockCols; ++col) {
            Block block;
            block.rect = {
                cfg::BlockMarginX + col * (blockWidth + cfg::BlockGap),
                cfg::BlockTop + row * (cfg::BlockHeight + cfg::BlockGap),
                blockWidth,
                cfg::BlockHeight
            };

            if (row == 1 && col % 2 == 0) {
                block.type = BlockType::Bonus;
                block.health = 1;
                block.hiddenBonus = randomBonusType();
                ++m_remainingBreakable;
            } else if (row == 2 && col % 3 == 1) {
                block.type = BlockType::Speed;
                block.health = 1;
                ++m_remainingBreakable;
            } else if (row == 3 && (col % 3 == 0 || col == cfg::BlockCols - 1)) {
                block.type = BlockType::Unbreakable;
                block.health = 1000000;
            } else {
                block.type = BlockType::Health;
                block.health = healthDist(m_rng);
                ++m_remainingBreakable;

                if (bonusChance(m_rng) < 18) {
                    block.hiddenBonus = randomBonusType();
                }
            }

            m_blocks.push_back(block);
        }
    }
}

void Game::resetRound(bool keepScoreState) {
    if (!keepScoreState) {
        m_score = 0;
        m_hits = 0;
        m_losses = 0;
    }

    m_bottomShieldAvailable = false;
    m_stickyEnabled = false;
    m_stickyTimer = 0.0f;
    m_paddle.rect.width = cfg::PaddleBaseWidth;
    m_paddle.targetWidth = cfg::PaddleBaseWidth;
    m_paddle.rect.left = (cfg::PlayfieldWidth - m_paddle.rect.width) * 0.5f;
    resetBallsOnPaddle();
}

void Game::resetBallsOnPaddle() {
    m_balls.clear();
    Ball ball;
    ball.radius = cfg::BallRadius;
    ball.position = { m_paddle.rect.left + m_paddle.rect.width * 0.5f, m_paddle.rect.top - ball.radius - 2.0f };
    ball.velocity = { cfg::BallBaseSpeed * 0.8f, -cfg::BallBaseSpeed };
    ball.stuckToPaddle = true;
    ball.stuckOffsetX = 0.0f;
    m_balls.push_back(ball);
}

void Game::updatePaddle(float dt) {
    float move = 0.0f;
    if (m_leftPressed) move -= m_paddle.speed * dt;
    if (m_rightPressed) move += m_paddle.speed * dt;

    m_paddle.rect.left += move;
    m_paddle.rect.left = std::clamp(m_paddle.rect.left, 0.0f, cfg::PlayfieldWidth - m_paddle.rect.width);

    if (std::abs(m_paddle.rect.width - m_paddle.targetWidth) > 0.1f) {
        const float center = m_paddle.rect.left + m_paddle.rect.width * 0.5f;
        m_paddle.rect.width += (m_paddle.targetWidth - m_paddle.rect.width) * std::min(1.0f, dt * 8.0f);
        m_paddle.rect.width = std::clamp(m_paddle.rect.width, cfg::PaddleMinWidth, cfg::PaddleMaxWidth);
        m_paddle.rect.left = center - m_paddle.rect.width * 0.5f;
        m_paddle.rect.left = std::clamp(m_paddle.rect.left, 0.0f, cfg::PlayfieldWidth - m_paddle.rect.width);
    }

    for (auto& ball : m_balls) {
        if (ball.stuckToPaddle) {
            ball.position.x = m_paddle.rect.left + m_paddle.rect.width * 0.5f + ball.stuckOffsetX;
            ball.position.y = m_paddle.rect.top - ball.radius - 2.0f;
        }
    }
}

void Game::updateBalls(float dt) {
    for (auto& ball : m_balls) {
        if (ball.stuckToPaddle) {
            continue;
        }

        ball.position += ball.velocity * dt;
        handleWallCollision(ball);
        handlePaddleCollision(ball);
        handleBlockCollisions(ball);
    }

    handleBallBallCollisions();
}

void Game::updateBonuses(float dt) {
    for (auto& bonus : m_fallingBonuses) {
        if (!bonus.active) continue;

        bonus.position.y += cfg::BonusFallSpeed * dt;

        const sf::FloatRect bonusRect(
            bonus.position.x - bonus.radius,
            bonus.position.y - bonus.radius,
            bonus.radius * 2.0f,
            bonus.radius * 2.0f
        );

        if (bonusRect.intersects(m_paddle.rect)) {
            applyBonus(bonus.type);
            bonus.active = false;
        } else if (bonus.position.y - bonus.radius > static_cast<float>(cfg::WindowHeight)) {
            bonus.active = false;
        }
    }

    m_fallingBonuses.erase(
        std::remove_if(m_fallingBonuses.begin(), m_fallingBonuses.end(), [](const FallingBonus& bonus) { return !bonus.active; }),
        m_fallingBonuses.end());
}

void Game::updateStickyState(float dt) {
    if (m_stickyEnabled) {
        m_stickyTimer -= dt;
        if (m_stickyTimer <= 0.0f) {
            m_stickyEnabled = false;
            m_stickyTimer = 0.0f;
        }
    }
}

void Game::handleWallCollision(Ball& ball) {
    if (ball.position.x - ball.radius < 0.0f) {
        ball.position.x = ball.radius;
        ball.velocity.x = std::abs(ball.velocity.x);
    }
    if (ball.position.x + ball.radius > cfg::PlayfieldWidth) {
        ball.position.x = cfg::PlayfieldWidth - ball.radius;
        ball.velocity.x = -std::abs(ball.velocity.x);
    }
    if (ball.position.y - ball.radius < 0.0f) {
        ball.position.y = ball.radius;
        ball.velocity.y = std::abs(ball.velocity.y);
    }

    if (m_bottomShieldAvailable && ball.position.y + ball.radius >= cfg::BottomShieldY) {
        ball.position.y = cfg::BottomShieldY - ball.radius - 1.0f;
        ball.velocity.y = -std::abs(ball.velocity.y);
        m_bottomShieldAvailable = false;
    }
}

void Game::handlePaddleCollision(Ball& ball) {
    const sf::FloatRect ballRect(ball.position.x - ball.radius, ball.position.y - ball.radius, ball.radius * 2.0f, ball.radius * 2.0f);
    if (!ballRect.intersects(m_paddle.rect)) {
        return;
    }

    if (ball.velocity.y <= 0.0f && !ball.stuckToPaddle) {
        return;
    }

    const float paddleCenter = m_paddle.rect.left + m_paddle.rect.width * 0.5f;
    const float offset = (ball.position.x - paddleCenter) / (m_paddle.rect.width * 0.5f);

    ball.position.y = m_paddle.rect.top - ball.radius - 1.0f;

    if (m_stickyEnabled) {
        ball.stuckToPaddle = true;
        ball.stuckOffsetX = std::clamp(offset * (m_paddle.rect.width * 0.4f), -m_paddle.rect.width * 0.45f, m_paddle.rect.width * 0.45f);
        ball.velocity = { cfg::BallBaseSpeed * 0.8f, -cfg::BallBaseSpeed };
        return;
    }

    const float speed = length(ball.velocity);
    ball.velocity.x = speed * offset;
    ball.velocity.y = -std::sqrt(std::max(120.0f * 120.0f, speed * speed - ball.velocity.x * ball.velocity.x));
    ball.velocity = clampBallVelocity(ball.velocity);
}

void Game::handleBlockCollisions(Ball& ball) {
    const sf::FloatRect ballRect(ball.position.x - ball.radius, ball.position.y - ball.radius, ball.radius * 2.0f, ball.radius * 2.0f);

    for (auto& block : m_blocks) {
        if (!block.alive) continue;
        if (!ballRect.intersects(block.rect)) continue;

        const float overlapLeft = (ball.position.x + ball.radius) - block.rect.left;
        const float overlapRight = (block.rect.left + block.rect.width) - (ball.position.x - ball.radius);
        const float overlapTop = (ball.position.y + ball.radius) - block.rect.top;
        const float overlapBottom = (block.rect.top + block.rect.height) - (ball.position.y - ball.radius);

        const float minX = std::min(overlapLeft, overlapRight);
        const float minY = std::min(overlapTop, overlapBottom);

        if (minX < minY) {
            if (overlapLeft < overlapRight) {
                ball.position.x = block.rect.left - ball.radius - 1.0f;
                ball.velocity.x = -std::abs(ball.velocity.x);
            } else {
                ball.position.x = block.rect.left + block.rect.width + ball.radius + 1.0f;
                ball.velocity.x = std::abs(ball.velocity.x);
            }
        } else {
            if (overlapTop < overlapBottom) {
                ball.position.y = block.rect.top - ball.radius - 1.0f;
                ball.velocity.y = -std::abs(ball.velocity.y);
            } else {
                ball.position.y = block.rect.top + block.rect.height + ball.radius + 1.0f;
                ball.velocity.y = std::abs(ball.velocity.y);
            }
        }

        if (block.type != BlockType::Unbreakable) {
            ++m_score;
            ++m_hits;

            if (block.type == BlockType::Speed) {
                ball.velocity *= 1.12f;
                ball.velocity = clampBallVelocity(ball.velocity);
            }

            block.health -= 1;
            if (block.health <= 0) {
                block.alive = false;
                --m_remainingBreakable;
                if (block.hiddenBonus != BonusType::None) {
                    spawnBonus({ block.rect.left + block.rect.width * 0.5f, block.rect.top + block.rect.height * 0.5f }, block.hiddenBonus);
                }
            }
        }
        break;
    }
}

void Game::handleBallBallCollisions() {
    for (std::size_t i = 0; i < m_balls.size(); ++i) {
        for (std::size_t j = i + 1; j < m_balls.size(); ++j) {
            if (m_balls[i].stuckToPaddle || m_balls[j].stuckToPaddle) continue;

            const sf::Vector2f delta = m_balls[j].position - m_balls[i].position;
            const float dist = length(delta);
            const float minDist = m_balls[i].radius + m_balls[j].radius;
            if (dist <= 0.001f || dist >= minDist) continue;

            const sf::Vector2f normal = delta / dist;
            const float penetration = minDist - dist;
            m_balls[i].position -= normal * (penetration * 0.5f);
            m_balls[j].position += normal * (penetration * 0.5f);

            const sf::Vector2f tangent(-normal.y, normal.x);
            const float v1n = dot(normal, m_balls[i].velocity);
            const float v1t = dot(tangent, m_balls[i].velocity);
            const float v2n = dot(normal, m_balls[j].velocity);
            const float v2t = dot(tangent, m_balls[j].velocity);

            m_balls[i].velocity = tangent * v1t + normal * v2n;
            m_balls[j].velocity = tangent * v2t + normal * v1n;

            m_balls[i].velocity = clampBallVelocity(m_balls[i].velocity);
            m_balls[j].velocity = clampBallVelocity(m_balls[j].velocity);
        }
    }
}

void Game::handleBallLosses() {
    std::vector<Ball> kept;
    kept.reserve(m_balls.size());

    for (auto& ball : m_balls) {
        if (ball.position.y - ball.radius <= static_cast<float>(cfg::WindowHeight)) {
            kept.push_back(ball);
            continue;
        }

        ++m_losses;
        m_paddle.targetWidth = std::max(cfg::PaddleMinWidth, m_paddle.targetWidth - 12.0f);
    }

    m_balls = std::move(kept);

    if (m_balls.empty()) {
        resetBallsOnPaddle();
    }
}

void Game::spawnBonus(const sf::Vector2f& center, BonusType type) {
    FallingBonus bonus;
    bonus.position = center;
    bonus.type = type;
    bonus.active = true;
    m_fallingBonuses.push_back(bonus);
}

void Game::applyBonus(BonusType type) {
    switch (type) {
        case BonusType::PaddleGrow:
            m_paddle.targetWidth = std::min(cfg::PaddleMaxWidth, m_paddle.targetWidth + 32.0f);
            break;
        case BonusType::PaddleShrink:
            m_paddle.targetWidth = std::max(cfg::PaddleMinWidth, m_paddle.targetWidth - 24.0f);
            break;
        case BonusType::BallSpeedUp:
            for (auto& ball : m_balls) {
                ball.velocity *= 1.18f;
                ball.velocity = clampBallVelocity(ball.velocity);
            }
            break;
        case BonusType::BallSlowDown:
            for (auto& ball : m_balls) {
                ball.velocity *= 0.84f;
                ball.velocity = clampBallVelocity(ball.velocity);
            }
            break;
        case BonusType::Sticky:
            m_stickyEnabled = true;
            m_stickyTimer = 14.0f;
            break;
        case BonusType::BottomShield:
            startBottomShield();
            break;
        case BonusType::SecondBall:
            if (!m_balls.empty()) {
                triggerSecondBall(m_balls.front().position);
            }
            break;
        case BonusType::None:
        default:
            break;
    }
}

void Game::launchStuckBalls() {
    for (auto& ball : m_balls) {
        if (!ball.stuckToPaddle) continue;

        ball.stuckToPaddle = false;
        const float ratio = (ball.position.x - (m_paddle.rect.left + m_paddle.rect.width * 0.5f)) / (m_paddle.rect.width * 0.5f);
        ball.velocity = { 220.0f * ratio, -cfg::BallBaseSpeed };
        ball.velocity = clampBallVelocity(ball.velocity);
    }
}

void Game::triggerSecondBall(const sf::Vector2f& position) {
    Ball ball;
    ball.radius = cfg::BallRadius;
    ball.position = position + sf::Vector2f(18.0f, -8.0f);
    ball.velocity = clampBallVelocity({ -220.0f, -cfg::BallBaseSpeed * 0.94f });
    m_balls.push_back(ball);
}

void Game::startBottomShield() {
    m_bottomShieldAvailable = true;
}

void Game::drawBlocks() {
    for (const auto& block : m_blocks) {
        if (!block.alive) continue;

        sf::RectangleShape shape({ block.rect.width, block.rect.height });
        shape.setPosition(block.rect.left, block.rect.top);
        shape.setFillColor(colorForBlock(block));
        shape.setOutlineThickness(2.0f);
        shape.setOutlineColor(sf::Color(28, 33, 43));
        m_window.draw(shape);

        if (block.type == BlockType::Unbreakable) {
            sf::RectangleShape stripe({ block.rect.width - 12.0f, 4.0f });
            stripe.setPosition(block.rect.left + 6.0f, block.rect.top + block.rect.height * 0.5f - 2.0f);
            stripe.setFillColor(sf::Color(210, 215, 223));
            m_window.draw(stripe);
        } else if (block.type == BlockType::Speed) {
            sf::ConvexShape arrow(3);
            arrow.setPoint(0, { block.rect.left + block.rect.width * 0.35f, block.rect.top + block.rect.height * 0.25f });
            arrow.setPoint(1, { block.rect.left + block.rect.width * 0.7f, block.rect.top + block.rect.height * 0.5f });
            arrow.setPoint(2, { block.rect.left + block.rect.width * 0.35f, block.rect.top + block.rect.height * 0.75f });
            arrow.setFillColor(sf::Color::White);
            m_window.draw(arrow);
        } else if (block.type == BlockType::Bonus) {
            drawBonusIcon(m_window, { block.rect.left + block.rect.width * 0.5f, block.rect.top + block.rect.height * 0.5f }, block.hiddenBonus, 0.75f);
        }

        if (block.type == BlockType::Health && m_hasFont) {
            sf::Text hp(std::to_string(std::max(1, block.health)), m_font, 16);
            hp.setFillColor(sf::Color::White);
            const auto bounds = hp.getLocalBounds();
            hp.setPosition(
                block.rect.left + block.rect.width * 0.5f - (bounds.width * 0.5f + bounds.left),
                block.rect.top + block.rect.height * 0.5f - (bounds.height * 0.5f + bounds.top) - 1.0f
            );
            m_window.draw(hp);
        }
    }
}

void Game::drawBalls() {
    for (const auto& ball : m_balls) {
        sf::CircleShape shape(ball.radius);
        shape.setOrigin(ball.radius, ball.radius);
        shape.setPosition(ball.position);
        shape.setFillColor(kBallColor);
        shape.setOutlineThickness(2.0f);
        shape.setOutlineColor(sf::Color(96, 104, 124));
        m_window.draw(shape);

        sf::CircleShape shine(ball.radius * 0.33f);
        shine.setOrigin(shine.getRadius(), shine.getRadius());
        shine.setPosition(ball.position.x - ball.radius * 0.35f, ball.position.y - ball.radius * 0.35f);
        shine.setFillColor(sf::Color(255, 255, 255, 180));
        m_window.draw(shine);
    }
}

void Game::drawPaddle() {
    sf::RectangleShape paddle({ m_paddle.rect.width, m_paddle.rect.height });
    paddle.setPosition(m_paddle.rect.left, m_paddle.rect.top);
    paddle.setFillColor(kPaddleColor);
    paddle.setOutlineThickness(2.0f);
    paddle.setOutlineColor(kPaddleAccent);
    m_window.draw(paddle);

    sf::RectangleShape accent({ m_paddle.rect.width * 0.7f, 4.0f });
    accent.setPosition(m_paddle.rect.left + m_paddle.rect.width * 0.15f, m_paddle.rect.top + 4.0f);
    accent.setFillColor(kPaddleAccent);
    m_window.draw(accent);
}

void Game::drawBonuses() {
    for (const auto& bonus : m_fallingBonuses) {
        if (!bonus.active) continue;

        sf::CircleShape bubble(bonus.radius);
        bubble.setOrigin(bonus.radius, bonus.radius);
        bubble.setPosition(bonus.position);
        bubble.setFillColor(colorForBonusType(bonus.type));
        bubble.setOutlineThickness(2.0f);
        bubble.setOutlineColor(sf::Color(36, 40, 48));
        m_window.draw(bubble);
        drawBonusIcon(m_window, bonus.position, bonus.type, 1.0f);
    }
}

void Game::drawShield() {
    if (!m_bottomShieldAvailable) return;

    sf::RectangleShape shield({ cfg::PlayfieldWidth, 8.0f });
    shield.setPosition(0.0f, cfg::BottomShieldY);
    shield.setFillColor(kShieldColor);
    m_window.draw(shield);
}

void Game::drawSidebar() {
    if (!m_hasFont) return;

    const float left = cfg::PlayfieldWidth + 34.0f;
    float top = 48.0f;

    auto drawLine = [&](const sf::String& text, unsigned size, const sf::Color& color, float extraGap = 0.0f) {
        sf::Text line(text, m_font, size);
        line.setFillColor(color);
        line.setPosition(left, top);
        m_window.draw(line);
        top += line.getLocalBounds().height + 16.0f + extraGap;
    };

    drawLine("ARKANOID", 31, kTextColor, 10.0f);
    drawLine(utf8("Попадания"), 18, kSoftTextColor, -10.0f);
    drawLine(sf::String(std::to_string(m_hits)), 26, kTextColor, 8.0f);
    drawLine(utf8("Очки"), 18, kSoftTextColor, -10.0f);
    drawLine(sf::String(std::to_string(m_score)), 26, kTextColor, 8.0f);
    drawLine(utf8("Потери"), 18, kSoftTextColor, -10.0f);
    drawLine(sf::String(std::to_string(m_losses)), 26, kTextColor, 8.0f);
    drawLine(utf8("Блоков осталось"), 18, kSoftTextColor, -10.0f);
    drawLine(sf::String(std::to_string(std::max(0, m_remainingBreakable))), 26, kTextColor, 18.0f);

    drawLine(utf8("Бонусы"), 22, kTextColor, 6.0f);
    const std::array<BonusType, 7> shown = {
        BonusType::PaddleGrow,
        BonusType::PaddleShrink,
        BonusType::BallSpeedUp,
        BonusType::BallSlowDown,
        BonusType::Sticky,
        BonusType::BottomShield,
        BonusType::SecondBall
    };

    for (auto type : shown) {
        const sf::Vector2f center(left + 18.0f, top + 12.0f);
        sf::CircleShape icon(12.0f);
        icon.setOrigin(12.0f, 12.0f);
        icon.setPosition(center);
        icon.setFillColor(colorForBonusType(type));
        icon.setOutlineThickness(2.0f);
        icon.setOutlineColor(sf::Color(36, 40, 48));
        m_window.draw(icon);
        drawBonusIcon(m_window, center, type, 0.82f);

        sf::Text label(textForBonus(type), m_font, 16);
        label.setFillColor(kSoftTextColor);
        label.setPosition(left + 38.0f, top - 1.0f);
        m_window.draw(label);
        top += 30.0f;
    }

    top += 12.0f;
    drawLine(utf8("Управление"), 22, kTextColor, 4.0f);

    const float controlLeft = left;
    auto drawControl = [&](const sf::String& text) {
        sf::Text line(text, m_font, 14);
        line.setFillColor(kSoftTextColor);
        line.setPosition(controlLeft, top);
        m_window.draw(line);
        top += 22.0f;
    };

    drawControl(utf8("A / D или ← / → — движение"));
    drawControl(utf8("Пробел — запуск прилипшего мяча"));
    drawControl(utf8("P — пауза"));
    drawControl(utf8("R — перезапуск уровня"));
}

void Game::drawBonusIcon(sf::RenderTarget& target, const sf::Vector2f& center, BonusType type, float scale) const {
    const float s = 10.0f * scale;
    switch (type) {
        case BonusType::PaddleGrow: {
            sf::RectangleShape h({ s * 1.5f, 3.0f * scale });
            h.setOrigin(h.getSize() * 0.5f);
            h.setPosition(center);
            h.setFillColor(sf::Color::White);
            target.draw(h);

            sf::RectangleShape v({ 3.0f * scale, s * 1.5f });
            v.setOrigin(v.getSize() * 0.5f);
            v.setPosition(center);
            v.setFillColor(sf::Color::White);
            target.draw(v);
        } break;
        case BonusType::PaddleShrink: {
            sf::RectangleShape h({ s * 1.5f, 3.0f * scale });
            h.setOrigin(h.getSize() * 0.5f);
            h.setPosition(center);
            h.setFillColor(sf::Color::White);
            target.draw(h);
        } break;
        case BonusType::BallSpeedUp:
        case BonusType::BallSlowDown: {
            sf::ConvexShape arrow(3);
            arrow.setPoint(0, { center.x - s * 0.5f, center.y - s * 0.55f });
            arrow.setPoint(1, { center.x + s * 0.7f, center.y });
            arrow.setPoint(2, { center.x - s * 0.5f, center.y + s * 0.55f });
            arrow.setFillColor(sf::Color::White);
            target.draw(arrow);

            if (type == BonusType::BallSlowDown) {
                sf::RectangleShape bar({ 3.0f * scale, s * 1.4f });
                bar.setOrigin(bar.getSize() * 0.5f);
                bar.setPosition(center.x - s * 0.8f, center.y);
                bar.setFillColor(sf::Color::White);
                target.draw(bar);
            }
        } break;
        case BonusType::Sticky: {
            sf::CircleShape drop(s * 0.45f, 20);
            drop.setOrigin(drop.getRadius(), drop.getRadius());
            drop.setScale(1.0f, 1.25f);
            drop.setPosition(center.x, center.y + 1.5f * scale);
            drop.setFillColor(sf::Color::White);
            target.draw(drop);
        } break;
        case BonusType::BottomShield: {
            sf::RectangleShape line({ s * 1.6f, 3.0f * scale });
            line.setOrigin(line.getSize() * 0.5f);
            line.setPosition(center.x, center.y + s * 0.35f);
            line.setFillColor(sf::Color::White);
            target.draw(line);
            sf::RectangleShape top({ s * 1.2f, 3.0f * scale });
            top.setOrigin(top.getSize() * 0.5f);
            top.setPosition(center.x, center.y - s * 0.15f);
            top.setFillColor(sf::Color::White);
            target.draw(top);
        } break;
        case BonusType::SecondBall: {
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
        } break;
        case BonusType::None:
        default:
            break;
    }
}

sf::Color Game::colorForBlock(const Block& block) const {
    switch (block.type) {
        case BlockType::Unbreakable: return kUnbreakableColor;
        case BlockType::Bonus: return kBonusBlockColor;
        case BlockType::Speed: return kSpeedBlockColor;
        case BlockType::Health: {
            if (block.health >= 4) return sf::Color(39, 174, 96);
            if (block.health == 3) return sf::Color(46, 204, 113);
            if (block.health == 2) return sf::Color(88, 214, 141);
            return kHealthBlockColor;
        }
        default: return sf::Color::White;
    }
}

sf::String Game::textForBonus(BonusType type) const {
    switch (type) {
        case BonusType::PaddleGrow: return utf8("каретка +");
        case BonusType::PaddleShrink: return utf8("каретка -");
        case BonusType::BallSpeedUp: return utf8("скорость +");
        case BonusType::BallSlowDown: return utf8("скорость -");
        case BonusType::Sticky: return utf8("липкая каретка");
        case BonusType::BottomShield: return utf8("одноразовое дно");
        case BonusType::SecondBall: return utf8("второй мяч");
        case BonusType::None:
        default: return utf8("нет");
    }
}

char Game::glyphForBonus(BonusType type) const {
    switch (type) {
        case BonusType::PaddleGrow: return '+';
        case BonusType::PaddleShrink: return '-';
        case BonusType::BallSpeedUp: return '>';
        case BonusType::BallSlowDown: return '<';
        case BonusType::Sticky: return 'S';
        case BonusType::BottomShield: return 'U';
        case BonusType::SecondBall: return '2';
        case BonusType::None:
        default: return '?';
    }
}

Game::BonusType Game::randomBonusType() {
    std::uniform_int_distribution<int> dist(1, 7);
    return static_cast<BonusType>(dist(m_rng));
}

float Game::length(const sf::Vector2f& value) {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

sf::Vector2f Game::normalize(const sf::Vector2f& value) {
    const float len = length(value);
    if (len <= 0.0001f) return { 0.0f, 0.0f };
    return value / len;
}

float Game::dot(const sf::Vector2f& a, const sf::Vector2f& b) {
    return a.x * b.x + a.y * b.y;
}

sf::Vector2f Game::clampBallVelocity(const sf::Vector2f& velocity) {
    const float speed = std::clamp(length(velocity), cfg::BallBaseSpeed * 0.72f, cfg::BallMaxSpeed);
    sf::Vector2f dir = normalize(velocity);
    if (std::abs(dir.x) < 0.16f) {
        dir.x = (dir.x < 0.0f) ? -0.16f : 0.16f;
        dir = normalize(dir);
    }
    if (std::abs(dir.y) < 0.35f) {
        dir.y = (dir.y < 0.0f) ? -0.35f : 0.35f;
        dir = normalize(dir);
    }
    return dir * speed;
}
