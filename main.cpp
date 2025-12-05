#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <array>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
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

    bgLoaderThread = std::jthread(&Clock::FetchBackgroundImage, this);
    lastPerformanceCounter = SDL_GetPerformanceCounter();

    return true;
  }

  SDL_AppResult Iterate() {
    UpdateTiming();
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

  std::jthread bgLoaderThread;
  std::mutex bgImageLoaderMutex;
  std::string lastLoadedUrl;
  SurfacePtr pendingBgImage;
  TexturePtr bgTexture;

  Uint64 lastPerformanceCounter = 0;
  double fps = 0.0;

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
    fps = 1.0 / ((double)diff / (double)SDL_GetPerformanceFrequency());
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
      SDL_SetTextureColorMod(bgTexture.get(), 100, 100, 100);
      RenderTextureCover(bgTexture.get());
    }
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
