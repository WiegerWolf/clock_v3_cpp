#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <ctime>
#include <iomanip>
#include <sstream>

#include "font_data.h"

using namespace cpr;
using namespace std;
using json = nlohmann::json;

struct Image {
  string fullUrl;
  string date;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Image, fullUrl, date)

namespace {
SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *fontBig;
TTF_Font *fontSmall;
const int screen_width = 1024;
const int screen_height = 600;
} // anonymous namespace

struct AppState {
  Uint64 lastPerformanceCounter;
  SDL_Mutex *mutex;

  SDL_Surface *bg_image;
  SDL_Texture *bg_texture;

  string last_time_str;
  SDL_Texture *time_texture;
  SDL_FRect time_rect;
};

string getCurrentTime() {
  auto t = time(nullptr);
  auto tm = *localtime(&t);
  ostringstream oss;
  oss << put_time(&tm, "%H:%M");
  return oss.str();
}

string getBgImageUrl() {
  Response response = Get(Url{"https://peapix.com/bing/feed?country=us"});
  json response_json = json::parse(response.text);
  // TODO: instead of grabbing the first image, grab the image with today's date
  auto first_image = response_json.get<vector<Image>>()[0];
  return first_image.fullUrl;
}

SDL_Surface *get_bg_image() {
  string bg_image_url = getBgImageUrl();
  Response bg_image =
      Get(Url{bg_image_url},
          ReserveSize{1024 * 1024}); // Increase reserve size to 1MB
  SDL_IOStream *bg_image_data_stream =
      SDL_IOFromConstMem(bg_image.text.data(), bg_image.text.size());
  SDL_Surface *bg_image_surface = IMG_Load_IO(bg_image_data_stream, true);
  if (!bg_image_surface) {
    SDL_Log("Couldn't load background image: %s", SDL_GetError());
    return nullptr;
  }
  return bg_image_surface;
}

int bgImageLoaderThread(void *data) {
  SDL_Surface *bg_image = get_bg_image();
  if (!bg_image) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to load background image");
    return -1;
  }

  auto *state = static_cast<AppState *>(data);
  SDL_LockMutex(state->mutex);
  state->bg_image = bg_image;
  SDL_UnlockMutex(state->mutex);

  return 0;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  SDL_SetAppMetadata("Digital Clock v3", "0.1.0", nullptr);
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s",
                    SDL_GetError());
    return SDL_APP_FAILURE;
  }
  if (!SDL_CreateWindowAndRenderer("Digital Clock v3", screen_width,
                                   screen_height, SDL_WINDOW_RESIZABLE, &window,
                                   &renderer)) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  if (!TTF_Init()) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Couldn't initialize SDL_ttf: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_IOStream *font_stream =
      SDL_IOFromConstMem(BellotaText_Bold_ttf, BellotaText_Bold_ttf_len);
  fontSmall = TTF_OpenFontIO(font_stream, true, 48);
  if (!fontSmall) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Couldn't open embedded font: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  // passing closeio false here, cause we're reusing font_stream from fontSmall
  // and we don't want to double free the stream when we TTF_CloseFont(fontBig);
  fontBig = TTF_OpenFontIO(font_stream, false, 420);
  if (!fontBig) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Couldn't open embedded font: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  if (!SDL_SetRenderLogicalPresentation(renderer, screen_width, screen_height,
                                        SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Couldn't set logical presentation: %s", SDL_GetError());
  }
  if (!SDL_HideCursor()) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Couldn't hide cursor: %s",
                SDL_GetError());
  }

  auto *state = new AppState();
  state->lastPerformanceCounter = SDL_GetPerformanceCounter();
  state->mutex = SDL_CreateMutex();
  state->bg_image = nullptr;
  state->bg_texture = nullptr;
  state->time_texture = nullptr;
  state->last_time_str = "";

  *appstate = state;

  // TODO: kick this off every 24 hours
  SDL_Thread *bg_image_loader_thread =
      SDL_CreateThread(bgImageLoaderThread, "bgImageLoaderThread", state);
  if (!bg_image_loader_thread) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Couldn't create background image loader thread: %s",
                 SDL_GetError());
  } else {
    SDL_DetachThread(bg_image_loader_thread);
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  auto *state = static_cast<AppState *>(appstate);

  double target_fps = 60.0;
  double target_frame_time = 1.0 / target_fps;

  Uint64 now = SDL_GetPerformanceCounter();
  Uint64 diff = now - state->lastPerformanceCounter;
  state->lastPerformanceCounter = now;

  double frameTimeS = (double)diff / (double)SDL_GetPerformanceFrequency();
  double fps = 1.0 / frameTimeS;

  string current_time_str = getCurrentTime();
  if (current_time_str != state->last_time_str) {
    SDL_Color white = {255, 255, 255, SDL_ALPHA_OPAQUE};
    SDL_Surface *timeSurface =
        TTF_RenderText_Blended(fontBig, current_time_str.c_str(), 0, white);
    if (timeSurface) {
      if (state->time_texture) {
        SDL_DestroyTexture(state->time_texture);
      }
      state->time_texture = SDL_CreateTextureFromSurface(renderer, timeSurface);
      if (!state->time_texture) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Failed to create texture from surface: %s",
                    SDL_GetError());
      } else {
        state->time_rect.w = (float)timeSurface->w;
        state->time_rect.h = (float)timeSurface->h;
        state->time_rect.x = ((float)screen_width - state->time_rect.w) / 2.0f;
        state->time_rect.y = ((float)screen_height - state->time_rect.h) / 2.0f;
      }
      SDL_DestroySurface(timeSurface);
      state->last_time_str = current_time_str;
    }
  }

  SDL_LockMutex(state->mutex);
  if (state->bg_image) {
    SDL_Texture *bg_texture =
        SDL_CreateTextureFromSurface(renderer, state->bg_image);
    if (!bg_texture) {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "Failed to create texture from surface: %s", SDL_GetError());
    } else {
      if (state->bg_texture) {
        SDL_DestroyTexture(state->bg_texture); // so we don't leak VRAM
      }
      state->bg_texture = bg_texture;
    }
    SDL_DestroySurface(state->bg_image);
    state->bg_image = nullptr;
  }
  SDL_UnlockMutex(state->mutex);

  SDL_SetRenderDrawColor(renderer, 0, 100, 100, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  if (state->bg_texture) {
    // TODO: instead of drawing the texture stretched, set up cover scaling
    SDL_RenderTexture(renderer, state->bg_texture, nullptr, nullptr);
  }

  if (state->time_texture) {
    SDL_RenderTexture(renderer, state->time_texture, nullptr,
                      &state->time_rect);
  }

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
  SDL_RenderDebugTextFormat(renderer, 10, 10, "FPS: %.2f", fps);

  SDL_RenderPresent(renderer);

  SDL_Delay(target_frame_time * 1000);
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  auto *state = static_cast<AppState *>(appstate);
  if (state->bg_texture) {
    SDL_DestroyTexture(state->bg_texture);
  }
  if (state->time_texture) {
    SDL_DestroyTexture(state->time_texture);
  }

  // Note: We deliberately leak the mutex and surface wrapper to avoid
  // crashing the worker thread. We let the OS clean up the memory when the
  // process exits.

  if (fontSmall) {
    TTF_CloseFont(fontSmall);
    fontSmall = nullptr;
  }
  if (fontBig) {
    TTF_CloseFont(fontBig);
    fontBig = nullptr;
  }
  TTF_Quit();
}
