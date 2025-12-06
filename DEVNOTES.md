# Background Image API

https://peapix.com/bing/feed?country=us

would return JSON, that looks like:

```json
[
  {
    "title": "Leshan Giant Buddha, Sichuan, China",
    "copyright": "\\u00A9 www.anotherdayattheoffice.org/Getty Image",
    "fullUrl": "https://img.peapix.com/3da559556ac64726a87fb1c879b59f46_1920.jpg",
    "thumbUrl": "https://img.peapix.com/3da559556ac64726a87fb1c879b59f46_640.jpg",
    "imageUrl": "https://img.peapix.com/3da559556ac64726a87fb1c879b59f46.jpg",
    "pageUrl": "https://peapix.com/bing/54032",
    "date": "2025-11-22"
  },
  ...
  {
    "title": "A traboule in Lyon, France",
    "copyright": "\\u00A9 TPopova/Getty Image",
    "fullUrl": "https://img.peapix.com/f3952eca8c40478585af02b480fc9547_1920.jpg",
    "thumbUrl": "https://img.peapix.com/f3952eca8c40478585af02b480fc9547_640.jpg",
    "imageUrl": "https://img.peapix.com/f3952eca8c40478585af02b480fc9547.jpg",
    "pageUrl": "https://peapix.com/bing/53960",
    "date": "2025-11-16"
  }
]
```

There's a mock server for this API in `./bing-feed-local-mock-server`. Use it for development.

# Weather API

https://api.open-meteo.com/v1/forecast?latitude=52.3738&longitude=4.8910&hourly=apparent_temperature,precipitation&current_weather=true&windspeed_unit=ms&timezone=auto

would return JSON, that looks like:

```json
{
  "latitude": 52.366,
  "longitude": 4.901,
  "generationtime_ms": 2.517104148864746,
  "utc_offset_seconds": 3600,
  "timezone": "Europe/Amsterdam",
  "timezone_abbreviation": "GMT+1",
  "elevation": 17.0,
  "current_weather_units": {
    "time": "iso8601",
    "interval": "seconds",
    "temperature": "°C",
    "windspeed": "m/s",
    "winddirection": "°",
    "is_day": "",
    "weathercode": "wmo code"
  },
  "current_weather": {
    "time": "2025-12-06T03:30",
    "interval": 900,
    "temperature": 4.9,
    "windspeed": 7.80,
    "winddirection": 159,
    "is_day": 0,
    "weathercode": 53
  },
  "hourly_units": {
    "time": "iso8601",
    "apparent_temperature": "°C",
    "precipitation": "mm"
  },
  "hourly": {
    "time": [
      "2025-12-06T00:00",
      ...
      "2025-12-12T23:00"
    ],
    "apparent_temperature": [
      -0.5,
      ...
      6.5
    ],
    "precipitation": [
      0.00,
      ...
      0.00
    ]
  }
}
```

# clangd LSP Integration

To get your editor pick up on dependencies headers, compile your project once in debug mode.
Then run `ln -s build/debug/compile_commands.json` in the root of your project and restart clangd.
