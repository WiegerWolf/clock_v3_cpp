#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <ctime>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "font_data.h"

using namespace std::string_literals;
using json = nlohmann::json;

template <typename T, auto Deleter>
using SdlPtr = std::unique_ptr<T, std::integral_constant<decltype(Deleter), Deleter>>;
using WindowPtr = SdlPtr<SDL_Window, SDL_DestroyWindow>;
using RendererPtr = SdlPtr<SDL_Renderer, SDL_DestroyRenderer>;
using SurfacePtr = SdlPtr<SDL_Surface, SDL_DestroySurface>;
using TexturePtr = SdlPtr<SDL_Texture, SDL_DestroyTexture>;
using FontPtr = SdlPtr<TTF_Font, TTF_CloseFont>;

namespace Config {
constexpr int screen_width = 1024;
constexpr int screen_height = 600;
constexpr int font_big_size = 382;
constexpr int font_small_size = 48;
constexpr int num_snowflakes = 666;
constexpr const char *AppName = "Digital Clock v3";
constexpr const char *AppVersion = "0.1.0";
} // namespace Config

struct BingImage {
  std::string fullUrl;
  std::string date; // format "2025-11-22"
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BingImage, fullUrl, date)

std::string getCurrentTime() {
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  return std::format("{}:{:02}", tm.tm_hour, tm.tm_min);
}

namespace {
constexpr std::array<std::string_view, 7> weekdays = {"воскресенье", "понедельник", "вторник", "среда",
                                                      "четверг",     "пятница",     "суббота"};
constexpr std::array<std::string_view, 12> months = {"января", "февраля", "марта",    "апреля",  "мая",    "июня",
                                                     "июля",   "августа", "сентября", "октября", "ноября", "декабря"};
} // namespace

std::string getCurrentDate() {
  auto now = std::chrono::system_clock::now();
  auto days = std::chrono::floor<std::chrono::days>(now);
  std::chrono::year_month_day ymd{days};
  std::chrono::weekday wd{days};
  return std::format("{}, {} {} {} года", weekdays[wd.c_encoding()], static_cast<unsigned>(ymd.day()),
                     months[static_cast<unsigned>(ymd.month()) - 1], static_cast<int>(ymd.year()));
}

class SnowSystem {
public:
  struct Flake {
    float x, y;
    float size;
    float speedY;
    int wobbleIndex; // Lookup table index (0-1023)
    int wobbleStep;
    float depth;
  };

  SnowSystem() : gen(std::random_device{}()) {
    sineLUT.resize(1024);
    for (int i = 0; i < 1024; ++i) {
      float phase = (float)i / 1024.0f * 2.0f * 3.14159f;
      sineLUT[i] = std::sin(phase);
    }
  }

  void Init(int width, int height, int count = 200) {
    screenWidth = width;
    screenHeight = height;

    flakes.resize(count);
    vertices.resize(count * 4);
    indices.resize(count * 6);

    // Pre-fill indices (Standard Quad Pattern: 0,1,2, 2,3,0)
    for (int i = 0; i < count; ++i) {
      int v = i * 4;
      int idx = i * 6;
      indices[idx + 0] = v + 0;
      indices[idx + 1] = v + 1;
      indices[idx + 2] = v + 2;
      indices[idx + 3] = v + 2;
      indices[idx + 4] = v + 3;
      indices[idx + 5] = v + 0;
    }

    for (auto &f : flakes) {
      ResetFlake(f, true);
    }
  }

  void Update(double dt) {
    windTimer += dt;

    // Lookup table access for wind
    float slowWind = 20.0f * GetFastSin(windTimer * 0.5f);
    float gustWind = 10.0f * GetFastSin(windTimer * 2.5f);
    float currentWind = slowWind + gustWind + 5.0f;

    const int lutMask = 1023;
    int vIndex = 0;

    for (auto &f : flakes) {
      // Physics Update
      f.y += f.speedY * (float)dt;

      // Wobble (Integer math + Look up table)
      f.wobbleIndex = (f.wobbleIndex + f.wobbleStep) & lutMask;
      float individualSway = sineLUT[f.wobbleIndex] * (10.0f * (1.0f - f.depth));

      f.x += (currentWind * f.depth + individualSway) * (float)dt;

      // Wrap/Reset Logic
      if (f.x > screenWidth)
        f.x = -f.size;
      else if (f.x < -f.size)
        f.x = (float)screenWidth;

      if (f.y > screenHeight) {
        ResetFlake(f, false);
      }

      float alphaVal = 0.2f + (f.depth * 0.8f);
      SDL_FColor col = {1.0f, 1.0f, 1.0f, alphaVal};

      // Top-Left
      vertices[vIndex + 0].position = {f.x, f.y};
      vertices[vIndex + 0].color = col;
      // Top-Right
      vertices[vIndex + 1].position = {f.x + f.size, f.y};
      vertices[vIndex + 1].color = col;
      // Bottom-Right
      vertices[vIndex + 2].position = {f.x + f.size, f.y + f.size};
      vertices[vIndex + 2].color = col;
      // Bottom-Left
      vertices[vIndex + 3].position = {f.x, f.y + f.size};
      vertices[vIndex + 3].color = col;

      vIndex += 4;
    }
  }

  void Draw(SDL_Renderer *renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(renderer,
                       nullptr, // No texture (white rectangles)
                       vertices.data(), (int)vertices.size(), indices.data(), (int)indices.size());
  }

private:
  int screenWidth = 0;
  int screenHeight = 0;
  double windTimer = 0.0;

  std::vector<Flake> flakes;
  std::vector<float> sineLUT;

  std::vector<SDL_Vertex> vertices;
  std::vector<int> indices;

  std::mt19937 gen;
  std::uniform_real_distribution<float> distDepth{0.2f, 1.0f};
  std::uniform_int_distribution<int> distPhase{0, 1023};

  float GetFastSin(double val) {
    int idx = static_cast<int>(val * (1024.0 / (2.0 * 3.14159))) & 1023;
    return sineLUT[idx];
  }

  void ResetFlake(Flake &f, bool randomizeY) {
    std::uniform_real_distribution<float> distX(0.0f, (float)screenWidth);
    std::uniform_real_distribution<float> distY(-50.0f, (float)screenHeight);

    f.depth = distDepth(gen);
    f.size = 2.0f + (f.depth * 3.0f);
    f.speedY = 30.0f + (f.depth * 60.0f);
    f.wobbleIndex = distPhase(gen);
    f.wobbleStep = 1 + static_cast<int>(f.depth * 3);
    f.x = distX(gen);
    f.y = randomizeY ? distY(gen) : -f.size;
  }
};

class Clock {
public:
  Clock() = default;
  Clock(const Clock &) = delete;
  Clock &operator=(const Clock &) = delete;

  bool Init() {
    SDL_SetAppMetadata(Config::AppName, Config::AppVersion, nullptr);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
      return false;
    }

    SDL_Window *w;
    SDL_Renderer *r;
    if (!SDL_CreateWindowAndRenderer(Config::AppName, Config::screen_width, Config::screen_height, SDL_WINDOW_RESIZABLE,
                                     &w, &r)) {
      SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window/renderer: %s", SDL_GetError());
      return false;
    }
    // Transfer ownership of the window and renderer to the unique_ptr
    window.reset(w);
    renderer.reset(r);

    if (!TTF_Init()) {
      SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL_ttf: %s", SDL_GetError());
      return false;
    }
    SDL_IOStream *stream1 = SDL_IOFromConstMem(BellotaText_Bold_ttf, BellotaText_Bold_ttf_len);
    // Note: SDL_ttf takes ownership of IOStream if closeio is true.
    fontSmall.reset(TTF_OpenFontIO(stream1, true, Config::font_small_size));
    // We create a second stream for the second font to avoid ownership ambiguity.
    SDL_IOStream *stream2 = SDL_IOFromConstMem(BellotaText_Bold_ttf, BellotaText_Bold_ttf_len);
    fontBig.reset(TTF_OpenFontIO(stream2, true, Config::font_big_size));
    if (!fontSmall || !fontBig) {
      SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load embedded font: %s", SDL_GetError());
      return false;
    }

    if (!SDL_SetRenderLogicalPresentation(renderer.get(), Config::screen_width, Config::screen_height,
                                          SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Couldn't set logical presentation: %s", SDL_GetError());
    }
    if (!SDL_HideCursor()) {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Couldn't hide cursor: %s", SDL_GetError());
    }

    snow.Init(Config::screen_width, Config::screen_height, Config::num_snowflakes);
    bgLoaderThread = std::jthread(&Clock::FetchBackgroundImage, this);
    lastPerformanceCounter = SDL_GetPerformanceCounter();

    return true;
  }

  SDL_AppResult Iterate() {
    UpdateTiming();
    snow.Update(deltaTime);
    UpdateTextures();
    {
      std::lock_guard lock(bgImageLoaderMutex);
      if (pendingBgImage) {
        bgTexture.reset(SDL_CreateTextureFromSurface(renderer.get(), pendingBgImage.get()));
        pendingBgImage.reset();
      }
    }
    Render();
    return SDL_APP_CONTINUE;
  }

private:
  WindowPtr window;
  RendererPtr renderer;
  FontPtr fontBig;
  FontPtr fontSmall;

  SnowSystem snow;

  std::jthread bgLoaderThread;
  std::mutex bgImageLoaderMutex;
  std::string lastLoadedUrl;
  SurfacePtr pendingBgImage;
  TexturePtr bgTexture;

  Uint64 lastPerformanceCounter = 0;
  double fps = 0.0;
  double deltaTime = 0.0;

  struct TextLabel {
    std::string text;
    TexturePtr texture;
    SDL_FRect rect;

    // Layout function to position the text label within the window
    using LayoutFunc = std::function<SDL_FRect(float w, float h)>;

    void update(SDL_Renderer *renderer, TTF_Font *font, std::string_view newText, SDL_Color color, LayoutFunc layout) {
      if (text == newText && texture) return;
      text = newText;
      SurfacePtr surf(TTF_RenderText_Blended(font, text.c_str(), 0, color));
      if (surf) {
        texture.reset(SDL_CreateTextureFromSurface(renderer, surf.get()));
        rect = layout((float)surf->w, (float)surf->h);
      }
    }

    void draw(SDL_Renderer *renderer) const {
      if (!texture) return;
      // Shadow
      SDL_SetTextureColorMod(texture.get(), 0, 0, 0);
      SDL_SetTextureAlphaMod(texture.get(), 128);
      SDL_FRect shadow = rect;
      shadow.x += 1.0f;
      shadow.y += 1.0f;
      SDL_RenderTexture(renderer, texture.get(), nullptr, &shadow);
      // Text
      SDL_SetTextureColorMod(texture.get(), 255, 255, 255);
      SDL_SetTextureAlphaMod(texture.get(), 255);
      SDL_RenderTexture(renderer, texture.get(), nullptr, &rect);
    }
  };

  TextLabel timeLabel;
  TextLabel dateLabel;

  void FetchBackgroundImage(std::stop_token stopToken) {
    while (!stopToken.stop_requested()) {
      try {
        cpr::Response response = cpr::Get(cpr::Url{"https://peapix.com/bing/feed?country=us"});
        if (response.status_code == 200) {
          auto response_json = json::parse(response.text);
          const auto &images = response_json.get<std::vector<BingImage>>();
          if (!images.empty()) {
            // TODO: instead of grabbing the first image, grab the image with today's date
            std::string imgUrl = images[0].fullUrl;
            if (imgUrl != lastLoadedUrl) {
              cpr::Response imgResp = cpr::Get(cpr::Url{imgUrl}, cpr::ReserveSize{2 * 1024 * 1024});
              if (imgResp.status_code == 200) {
                SDL_IOStream *io = SDL_IOFromConstMem(imgResp.text.data(), imgResp.text.size());
                SurfacePtr loadedSurf(IMG_Load_IO(io, true));
                if (loadedSurf) {
                  std::lock_guard lock(bgImageLoaderMutex);
                  pendingBgImage = std::move(loadedSurf);
                  lastLoadedUrl = imgUrl;
                }
              }
            }
          }
        }
      } catch (const std::exception &e) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Background image fetch failed: %s", e.what());
      }
      std::mutex sleepMutex;             // We use a dummy mutex because the condition_variable's wait_for requires
      std::unique_lock lock(sleepMutex); // a lock to wait on
      std::condition_variable_any().wait_for(lock, stopToken, std::chrono::hours(4),
                                             [&stopToken] { return stopToken.stop_requested(); });
    }
  }

  void UpdateTiming() {
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 diff = now - lastPerformanceCounter;
    lastPerformanceCounter = now;
    deltaTime = (double)diff / (double)SDL_GetPerformanceFrequency();
    fps = 1.0 / deltaTime;

    SDL_Delay(16); // Wait for 16ms to maintain 60 FPS
  }

  void UpdateTextures() {
    SDL_Color white = {255, 255, 255, SDL_ALPHA_OPAQUE};

    timeLabel.update(renderer.get(), fontBig.get(), getCurrentTime(), white, [](float w, float h) {
      return SDL_FRect{(Config::screen_width - w) / 2.0f, (Config::screen_height - h) / 2.0f - 40.0f, w, h};
    });
    dateLabel.update(renderer.get(), fontSmall.get(), getCurrentDate(), white,
                     [](float w, float h) { return SDL_FRect{(Config::screen_width - w) / 2.0f, 30.0f, w, h}; });
  }

  void Render() {
    SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer.get());

    if (bgTexture) {
      SDL_SetTextureColorMod(bgTexture.get(), 150, 150, 150);
      RenderTextureCover(bgTexture.get());
    }
    snow.Draw(renderer.get());
    dateLabel.draw(renderer.get());
    timeLabel.draw(renderer.get());

    SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDebugTextFormat(renderer.get(), 10, 10, "FPS: %.2f", fps);

    SDL_RenderPresent(renderer.get());
  }

  // Helper to simulate "CSS object-fit: cover"
  void RenderTextureCover(SDL_Texture *texture) {
    float w, h;
    SDL_GetTextureSize(texture, &w, &h);
    float scale = std::max((float)Config::screen_width / w, (float)Config::screen_height / h);
    float newW = w * scale;
    float newH = h * scale;
    SDL_FRect dst = {((float)Config::screen_width - newW) / 2.0f, ((float)Config::screen_height - newH) / 2.0f, newW,
                     newH};
    SDL_RenderTexture(renderer.get(), texture, nullptr, &dst);
  }
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  auto *app = new Clock();
  if (!app->Init()) {
    delete app;
    return SDL_APP_FAILURE;
  }
  *appstate = app;
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  auto *app = static_cast<Clock *>(appstate);
  return app->Iterate();
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  auto *app = static_cast<Clock *>(appstate);
  delete app;
  TTF_Quit();
}
