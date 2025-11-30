#define SDL_MAIN_USE_CALLBACKS 1
// https://wiki.libsdl.org/SDL3/README-main-functions
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using namespace cpr;
using namespace std;
using json = nlohmann::json;

struct Image {
  string fullUrl;
  string date;
};
// https://json.nlohmann.me/features/arbitrary_types/#simplify-your-life-with-macros
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Image, fullUrl, date)

// https://examples.libsdl.org/SDL3/renderer/01-clear/
static SDL_Window *window;
static SDL_Renderer *renderer;

struct AppState {
  Uint64 lastPerformanceCounter;
};

string getBgImageUrl() {
  // https://docs.libcpr.dev/introduction.html#get-requests
  Response response = Get(Url{"https://peapix.com/bing/feed?country=us"});

  // https://json.nlohmann.me/features/parsing/json_lines/
  json response_json = json::parse(response.text);
  auto first_image = response_json.get<vector<Image>>()[0];
  return first_image.fullUrl;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  SDL_SetAppMetadata("Digital Clock v3", "0.1.0", nullptr);
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  if (!SDL_CreateWindowAndRenderer("Digital Clock v3", 1024, 600,
                                   SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  if (!SDL_HideCursor()) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Couldn't hide cursor: %s", SDL_GetError());
  }
  auto *state = new AppState();
  state->lastPerformanceCounter = SDL_GetPerformanceCounter();

  *appstate = state;
  /* This should be in a background thread
  string bg_image_url = getBgImageUrl();
  Response bg_image =
      Get(Url{bg_image_url},
          ReserveSize{1024 * 1024}); // Increase reserve size to 1MB
          */
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

  SDL_SetRenderDrawColor(renderer, 0, 100, 100, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
  SDL_RenderDebugTextFormat(renderer, 10, 10, "FPS: %.2f", fps);

  SDL_RenderPresent(renderer);

  SDL_Delay(target_frame_time * 1000);
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {}
