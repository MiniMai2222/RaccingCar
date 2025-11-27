#include <SDL3/SDL.h>
#include <SDL3/SDL_ttf.h>
#include <bits/stdc++.h>
#include <fstream>
#include <stdexcept>

using namespace std;

const int WINDOW_W = 800;
const int WINDOW_H = 600;

const int ROAD_W = 500;
const int ROAD_LEFT = (WINDOW_W - ROAD_W) / 2;
const int ROAD_RIGHT = ROAD_LEFT + ROAD_W;

const float CAR_W = 40.0f;
const float CAR_H = 80.0f;
const int ENEMY_MAX = 4;

const float PLAYER_SPEED = 6.0f;
const float ENEMY_INITIAL_SPEED = 4.0f;
const string HIGHSCORE_FILE = "highscore.txt";

SDL_Window* g_pWindow = nullptr;
SDL_Renderer* g_pRenderer = nullptr;
TTF_Font* g_pFont = nullptr;

enum GameScreen {
    SCREEN_MAIN_MENU = 0,
    SCREEN_INSTRUCTIONS = 1,
    SCREEN_RACING = 2,
    SCREEN_GAMEOVER = 3
};

const SDL_Color COLOR_ROAD = {50, 50, 50, 255};
const SDL_Color COLOR_PLAYER = {0, 100, 255, 255};
const SDL_Color COLOR_ENEMY_R = {255, 0, 0, 255};
const SDL_Color COLOR_ENEMY_O = {255, 128, 0, 255};
const SDL_Color COLOR_ENEMY_P = {128, 0, 255, 255};
const SDL_Color COLOR_ENEMY_G = {0, 255, 0, 255};
const SDL_Color COLOR_WHITE = {255, 255, 255, 255};
const SDL_Color COLOR_BLACK = {0, 0, 0, 255};

class GameObject {
protected:
    float m_posX, m_posY;
    float m_width, m_height;
    SDL_Color m_color;
    bool m_isActive;

public:
    GameObject(float x, float y, float w, float h, SDL_Color color)
        : m_posX(x), m_posY(y), m_width(w), m_height(h), m_color(color), m_isActive(true) {}

    virtual ~GameObject() = default;

    virtual void update() = 0;
    virtual void draw() {
        if (!m_isActive) return;

        SDL_SetRenderDrawColor(g_pRenderer, m_color.r, m_color.g, m_color.b, m_color.a);
        SDL_FRect carBody = {m_posX, m_posY, m_width, m_height};
        SDL_RenderFillRect(g_pRenderer, &carBody);

        SDL_SetRenderDrawColor(g_pRenderer, 0, 0, 0, 255);
        float wheelW = m_width / 4.0f;
        float wheelH = m_height / 8.0f;
        SDL_FRect wheelTL = {m_posX - wheelW, m_posY + m_height/8.0f, wheelW, wheelH};
        SDL_FRect wheelTR = {m_posX + m_width, m_posY + m_height/8.0f, wheelW, wheelH};
        SDL_FRect wheelBL = {m_posX - wheelW, m_posY + m_height*6/8.0f, wheelW, wheelH};
        SDL_FRect wheelBR = {m_posX + m_width, m_posY + m_height*6/8.0f, wheelW, wheelH};
        SDL_RenderFillRect(g_pRenderer, &wheelTL);
        SDL_RenderFillRect(g_pRenderer, &wheelTR);
        SDL_RenderFillRect(g_pRenderer, &wheelBL);
        SDL_RenderFillRect(g_pRenderer, &wheelBR);
    }

    float getLeft() const { return m_posX; }
    float getRight() const { return m_posX + m_width; }
    float getTop() const { return m_posY; }
    float getBottom() const { return m_posY + m_height; }
    void setActive(bool active) { m_isActive = active; }
    bool isActive() const { return m_isActive; }

    void setX(float x) { m_posX = x; }
    void setY(float y) { m_posY = y; }
};

class PlayerCar : public GameObject {
private:
    float m_speed;
public:
    PlayerCar()
        : GameObject(ROAD_LEFT + ROAD_W / 2 - CAR_W / 2, WINDOW_H - CAR_H - 20, CAR_W, CAR_H, COLOR_PLAYER),
          m_speed(PLAYER_SPEED) {}

    void handleInput() {
        int numKeys;
        const Uint8* currentKeyStates = reinterpret_cast<const Uint8*>(SDL_GetKeyboardState(&numKeys));

        if (currentKeyStates[SDL_SCANCODE_A]) {
            m_posX -= m_speed;
        }
        if (currentKeyStates[SDL_SCANCODE_D]) {
            m_posX += m_speed;
        }

        if (m_posX < ROAD_LEFT + 5) {
            m_posX = ROAD_LEFT + 5;
        }
        if (m_posX + m_width > ROAD_RIGHT - 5) {
            m_posX = ROAD_RIGHT - m_width - 5;
        }
    }

    void update() override { }
};

class OpponentCar : public GameObject {
private:
    float m_currentSpeed;
public:
    OpponentCar()
        : GameObject(0.0f, 0.0f, CAR_W, CAR_H, COLOR_ENEMY_R),
          m_currentSpeed(ENEMY_INITIAL_SPEED)
    {
        m_isActive = false;
    }

    void setSpeed(float speed) { m_currentSpeed = speed; }
    void setColor(const SDL_Color& color) { m_color = color; }

    void update() override {
        if (m_isActive) {
            m_posY += m_currentSpeed;
        }
    }
    
    void spawn(int score) {
        int minX = ROAD_LEFT + (int)(CAR_W / 2);
        int maxX = ROAD_RIGHT - (int)(CAR_W * 3 / 2);

        m_posX = (float)(minX + (rand() % (maxX - minX)));
        m_posY = -m_height;
        m_currentSpeed = ENEMY_INITIAL_SPEED + score * 0.1f;
        m_isActive = true;
    }
};

void drawTextAt(const char* text, float x, float y, const SDL_Color& color, TTF_Font* font) {
    if (font == nullptr || text == nullptr) return;

    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, SDL_strlen(text), color); 
    
    if (textSurface == nullptr) {
        cerr << "Text surface render failed: " << SDL_GetError() << endl;
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(g_pRenderer, textSurface);
    if (textTexture == nullptr) {
        cerr << "Texture from text creation failed: " << SDL_GetError() << endl;
    }

    SDL_FRect renderQuad = {x, y, (float)textSurface->w, (float)textSurface->h};

    if (textTexture != nullptr) {
        SDL_RenderTexture(g_pRenderer, textTexture, nullptr, &renderQuad);
    }

    SDL_DestroySurface(textSurface);
    SDL_DestroyTexture(textTexture);
}


class GameManager {
private:
    PlayerCar m_player;
    vector<OpponentCar> m_opponentList; 
    int m_currentScore;
    int m_highScore;
    GameScreen m_currentScreen;
    bool m_isRunning;
    float m_markerOffset;

    void loadHighScore() {
        try {
            ifstream file(HIGHSCORE_FILE);
            if (!file.is_open()) {
                throw runtime_error("Không tìm thấy file highscore.txt. Tạo mới highscore = 0.");
            }
            if (!(file >> m_highScore)) {
                throw runtime_error("Lỗi định dạng dữ liệu trong file highscore.txt.");
            }
            file.close();
        } catch (const runtime_error& e) {
            cerr << "Lỗi đọc High Score: " << e.what() << endl;
            m_highScore = 0;
        }
    }

    void saveHighScore() {
        if (m_currentScore > m_highScore) {
            m_highScore = m_currentScore;
        }
        ofstream file(HIGHSCORE_FILE);
        if (file.is_open()) {
            file << m_highScore;
            file.close();
        } else {
            cerr << "Lỗi ghi file highscore.txt!" << endl;
        }
    }

    void drawRaceTrack() {
        SDL_SetRenderDrawColor(g_pRenderer, COLOR_ROAD.r, COLOR_ROAD.g, COLOR_ROAD.b, COLOR_ROAD.a);
        SDL_FRect roadRect = {(float)ROAD_LEFT, 0.0f, (float)ROAD_W, (float)WINDOW_H};
        SDL_RenderFillRect(g_pRenderer, &roadRect);

        SDL_SetRenderDrawColor(g_pRenderer, 255, 255, 255, 255);
        float markerW = 10.0f;
        float markerH = 50.0f;

        float laneMarker1X = ROAD_LEFT + ROAD_W / 4 - markerW / 2;
        float laneMarker3X = ROAD_LEFT + ROAD_W * 3 / 4 - markerW / 2;
        float laneMarker2X = ROAD_LEFT + ROAD_W * 2 / 4 - markerW / 2;

        m_markerOffset += ENEMY_INITIAL_SPEED + m_currentScore * 0.1f;
        if (m_markerOffset > markerH * 2) {
            m_markerOffset = 0.0f;
        }

        for (int i = 0; i < WINDOW_H / (markerH * 2) + 2; ++i) {
            SDL_FRect marker1 = {laneMarker1X, i * (markerH * 2) - m_markerOffset, markerW, markerH};
            SDL_RenderFillRect(g_pRenderer, &marker1);
            SDL_FRect marker3 = {laneMarker3X, i * (markerH * 2) - m_markerOffset, markerW, markerH};
            SDL_RenderFillRect(g_pRenderer, &marker3);
            SDL_FRect marker2 = {laneMarker2X, i * (markerH * 2) - m_markerOffset, markerW, markerH};
            SDL_RenderFillRect(g_pRenderer, &marker2);
        }
    }

    bool checkAABBCollision() {
        for (auto& enemy : m_opponentList) {
            if (enemy.isActive()) {
                float pLeft = m_player.getLeft();
                float pRight = m_player.getRight();
                float pTop = m_player.getTop();
                float pBottom = m_player.getBottom();

                float eLeft = enemy.getLeft();
                float eRight = enemy.getRight();
                float eTop = enemy.getTop();
                float eBottom = enemy.getBottom();

                if (pRight > eLeft && pLeft < eRight && pBottom > eTop && pTop < eBottom) {
                    return true;
                }
            }
        }
        return false;
    }

    void spawnNewOpponent(int index) {
        int minX = ROAD_LEFT + (int)(CAR_W / 2);
        int maxX = ROAD_RIGHT - (int)(CAR_W * 3 / 2);

        m_opponentList[index].setX((float)(minX + (rand() % (maxX - minX))));
        m_opponentList[index].setY(-CAR_H);
        m_opponentList[index].setSpeed(ENEMY_INITIAL_SPEED + m_currentScore * 0.1f);
        m_opponentList[index].setActive(true);

        if (index == 0) m_opponentList[index].setColor(COLOR_ENEMY_R);
        else if (index == 1) m_opponentList[index].setColor(COLOR_ENEMY_O);
        else if (index == 2) m_opponentList[index].setColor(COLOR_ENEMY_P);
        else m_opponentList[index].setColor(COLOR_ENEMY_G);
    }

    void initRacing() {
        m_currentScore = 0;
        m_player = PlayerCar();
        m_markerOffset = 0.0f;

        m_opponentList.clear();
        for (int i = 0; i < ENEMY_MAX; ++i) {
            m_opponentList.emplace_back();
            m_opponentList.back().setActive(i < 2);
            if (m_opponentList.back().isActive()) {
                 spawnNewOpponent(i);
                 m_opponentList[i].setY(-CAR_H * (i * 3 + 1));
            }
        }
    }

public:
    GameManager()
        : m_currentScore(0), m_highScore(0), m_currentScreen(SCREEN_MAIN_MENU), m_isRunning(true), m_markerOffset(0.0f)
    {
        m_opponentList.reserve(ENEMY_MAX);
        loadHighScore();
        initRacing();
    }

    bool isRunning() const { return m_isRunning; }
    GameScreen getCurrentScreen() const { return m_currentScreen; }

    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_EVENT_QUIT) {
                m_isRunning = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (m_currentScreen == SCREEN_RACING && event.key.key == SDLK_ESCAPE) {
                    m_currentScreen = SCREEN_GAMEOVER;
                    saveHighScore();
                }
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                float mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                if (m_currentScreen == SCREEN_MAIN_MENU) {
                    if (mouseX > WINDOW_W/2 - 100 && mouseX < WINDOW_W/2 + 100) {
                        if (mouseY > 200 && mouseY < 250) {
                            initRacing();
                            m_currentScreen = SCREEN_RACING;
                        } else if (mouseY > 270 && mouseY < 320) {
                            m_currentScreen = SCREEN_INSTRUCTIONS;
                        } else if (mouseY > 340 && mouseY < 390) {
                            m_isRunning = false;
                        }
                    }
                } else if (m_currentScreen == SCREEN_INSTRUCTIONS) {
                    m_currentScreen = SCREEN_MAIN_MENU;
                } else if (m_currentScreen == SCREEN_GAMEOVER) {
                    m_currentScreen = SCREEN_MAIN_MENU;
                }
            }
        }

        if (m_currentScreen == SCREEN_RACING) {
            m_player.handleInput();
        }
    }

    void updateLogic() {
        if (m_currentScreen != SCREEN_RACING) return;

        for (int i = 0; i < ENEMY_MAX; ++i) {
            if (m_opponentList[i].isActive()) {
                m_opponentList[i].update();

                if (m_opponentList[i].getTop() > WINDOW_H) {
                    spawnNewOpponent(i);
                    m_currentScore++;

                    if (m_currentScore >= 5 && i == 0 && !m_opponentList[2].isActive()) {
                        m_opponentList[2].setActive(true);
                        spawnNewOpponent(2);
                    }
                    if (m_currentScore >= 15 && i == 1 && !m_opponentList[3].isActive()) {
                        m_opponentList[3].setActive(true);
                        spawnNewOpponent(3);
                    }
                }
            }
        }

        if (checkAABBCollision()) {
            m_currentScreen = SCREEN_GAMEOVER;
            saveHighScore();
        }
    }

    void renderGame() {
        SDL_SetRenderDrawColor(g_pRenderer, 20, 20, 50, 255);
        SDL_RenderClear(g_pRenderer);

        if (m_currentScreen == SCREEN_RACING || m_currentScreen == SCREEN_GAMEOVER) {
            drawRaceTrack();

            SDL_SetRenderDrawColor(g_pRenderer, 200, 200, 200, 255);
            SDL_FRect uiRectLeft = {0.0f, 0.0f, (float)ROAD_LEFT, (float)WINDOW_H};
            SDL_FRect uiRectRight = {(float)ROAD_RIGHT, 0.0f, (float)WINDOW_W - ROAD_RIGHT, (float)WINDOW_H};
            SDL_RenderFillRect(g_pRenderer, &uiRectLeft);
            SDL_RenderFillRect(g_pRenderer, &uiRectRight);

            char scoreText[50];
            snprintf(scoreText, 50, "ĐIỂM: %d", m_currentScore);
            drawTextAt(scoreText, (float)ROAD_RIGHT + 10, 20.0f, COLOR_BLACK, g_pFont);

            char highScoreText[50];
            snprintf(highScoreText, 50, "CAO NHẤT: %d", m_highScore);
            drawTextAt(highScoreText, (float)ROAD_RIGHT + 10, 60.0f, COLOR_BLACK, g_pFont);


            for (auto& enemy : m_opponentList) {
                enemy.draw();
            }

            m_player.draw();

            if (m_currentScreen == SCREEN_GAMEOVER) {
                SDL_SetRenderDrawColor(g_pRenderer, 255, 0, 0, 150);
                SDL_FRect goOverlay = {0.0f, 0.0f, (float)WINDOW_W, (float)WINDOW_H};
                SDL_RenderFillRect(g_pRenderer, &goOverlay);

                drawTextAt("GAME OVER!", (float)WINDOW_W/2 - 120, (float)WINDOW_H/2 - 50, {255, 255, 0, 255}, g_pFont);
                drawTextAt("Click để về Menu Chính", (float)WINDOW_W/2 - 100, (float)WINDOW_H/2, COLOR_WHITE, g_pFont);
            }

        } else if (m_currentScreen == SCREEN_MAIN_MENU) {
            drawTextAt("GROUP 3: Cuộc Đua", (float)WINDOW_W/2 - 130, 80.0f, {255, 255, 0, 255}, g_pFont);

            SDL_SetRenderDrawColor(g_pRenderer, 0, 200, 0, 255);
            SDL_FRect startRect = {(float)WINDOW_W/2 - 100, 200.0f, 200.0f, 50.0f};
            SDL_RenderFillRect(g_pRenderer, &startRect);
            drawTextAt("1. Bắt Đầu Game", (float)WINDOW_W/2 - 85, 215.0f, COLOR_WHITE, g_pFont);

            SDL_SetRenderDrawColor(g_pRenderer, 0, 150, 200, 255);
            SDL_FRect instrRect = {(float)WINDOW_W/2 - 100, 270.0f, 200.0f, 50.0f};
            SDL_RenderFillRect(g_pRenderer, &instrRect);
            drawTextAt("2. Hướng Dẫn", (float)WINDOW_W/2 - 70, 285.0f, COLOR_WHITE, g_pFont);

            SDL_SetRenderDrawColor(g_pRenderer, 200, 50, 50, 255);
            SDL_FRect quitRect = {(float)WINDOW_W/2 - 100, 340.0f, 200.0f, 50.0f};
            SDL_RenderFillRect(g_pRenderer, &quitRect);
            drawTextAt("3. Thoát", (float)WINDOW_W/2 - 45, 355.0f, COLOR_WHITE, g_pFont);

            char highScoreText[70];
            snprintf(highScoreText, 70, "ĐIỂM CAO NHẤT: %d", m_highScore);
            drawTextAt(highScoreText, (float)WINDOW_W/2 - 100, 450.0f, {255, 255, 255, 255}, g_pFont);

        } else if (m_currentScreen == SCREEN_INSTRUCTIONS) {
            SDL_SetRenderDrawColor(g_pRenderer, 255, 255, 255, 255);
            SDL_FRect instrBox = {50.0f, 50.0f, (float)WINDOW_W - 100, (float)WINDOW_H - 100};
            SDL_RenderFillRect(g_pRenderer, &instrBox);

            drawTextAt("HƯỚNG DẪN CHƠI", (float)WINDOW_W/2 - 100, 70.0f, COLOR_BLACK, g_pFont);
            drawTextAt("Tránh né xe địch.", 100.0f, 150.0f, COLOR_BLACK, g_pFont);
            drawTextAt("Dùng 'A' (Trái) và 'D' (Phải) để di chuyển.", 100.0f, 180.0f, COLOR_BLACK, g_pFont);
            drawTextAt("Nhấn ESC để về Menu.", 100.0f, 210.0f, COLOR_BLACK, g_pFont);
            drawTextAt("Điểm được tính khi xe địch đi qua màn hình.", 100.0f, 240.0f, COLOR_BLACK, g_pFont);
            drawTextAt("Click bất kỳ để trở về Menu Chính.", (float)WINDOW_W/2 - 130, 500.0f, {100, 100, 100, 255}, g_pFont);
        }

        SDL_RenderPresent(g_pRenderer);
    }
};

bool initializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cerr << "SDL failed to initialize: " << SDL_GetError() << endl;
        return false;
    }

    g_pWindow = SDL_CreateWindow("GROUP 3: The Race", WINDOW_W, WINDOW_H, 0);
    if (g_pWindow == nullptr) {
        cerr << "Window creation failed: " << SDL_GetError() << endl;
        return false;
    }

    g_pRenderer = SDL_CreateRenderer(g_pWindow, nullptr);
    if (g_pRenderer == nullptr) {
        cerr << "Renderer creation failed: " << SDL_GetError() << endl;
        return false;
    }

    if (TTF_Init() == -1) {
        cerr << "SDL_ttf failed to initialize: " << SDL_GetError() << endl;
        return false;
    }

    try {
        g_pFont = TTF_OpenFont("arial.ttf", 20);
        if (g_pFont == nullptr) {
            throw runtime_error("Không thể load font: arial.ttf. Font phải nằm trong thư mục chạy.");
        }
    } catch (const runtime_error& e) {
        cerr << "LỖI FONT: " << e.what() << " " << SDL_GetError() << endl;
        return false;
    }

    srand(time(0));
    return true;
}

void cleanupSDL() {
    if (g_pFont != nullptr) {
        TTF_CloseFont(g_pFont);
        g_pFont = nullptr;
    }
    TTF_Quit();

    SDL_DestroyRenderer(g_pRenderer);
    SDL_DestroyWindow(g_pWindow);
    g_pWindow = nullptr;
    g_pRenderer = nullptr;
    SDL_Quit();
}


int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    if (!initializeSDL()) {
        cerr << "Application failed to initialize!" << endl;
        return 1;
    }

    GameManager gameManager;

    while (gameManager.isRunning()) {
        gameManager.handleEvents();
        gameManager.updateLogic();
        gameManager.renderGame();

        SDL_Delay(1000 / 60);
    }

    cleanupSDL();
    return 0;
}