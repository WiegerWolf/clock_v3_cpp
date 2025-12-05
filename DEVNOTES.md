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

# clangd LSP Integration

To get your editor pick up on dependencies headers, compile your project once in debug mode.
Then run `ln -s build/debug/compile_commands.json` in the root of your project and restart clangd.
