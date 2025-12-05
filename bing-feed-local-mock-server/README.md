# Bing Feed Local Mock Server

A high-performance mock server for the Bing Wallpaper feed. This project utilizes **@napi-rs/canvas** for fast, Rust-based server-side image generation.

It generates a JSON feed of "fake" Bing wallpapers and dynamically renders colored placeholder images based on the file hash.

## Setup

### 1. Install Bun
If you haven't already, install Bun:
```bash
curl -fsSL https://bun.sh/install | bash
```

### 2. Install Dependencies
Installs `@napi-rs/canvas` and TypeScript definitions:
```bash
bun install
```

## Running the Server

Start the server in development mode (hot-reloading not enabled by default in this script, but fast to restart):

```bash
bun run index.ts
```

Output:
```
Listening on http://localhost:3000
```

## API Endpoints

### 1. Get Wallpaper Feed
Returns a JSON array of 5 mock wallpaper items with dates going back 5 days.

**Request:**
```http
GET /bing/feed
```

**Response Example:**
```json
[
  {
    "title": "Sample Location 1",
    "copyright": "© Sample/Getty Image",
    "fullUrl": "http://localhost:3000/a1b2c3d4..._1920.jpg",
    "thumbUrl": "http://localhost:3000/a1b2c3d4..._640.jpg",
    "date": "2023-10-27"
  },
  ...
]
```

### 2. Get Dynamic Image
Dynamically generates a JPEG image. The background color is deterministically generated based on the hash in the filename.

**Request:**
```http
GET /<hash>_<resolution>.jpg
```
*Example: `http://localhost:3000/f3a12..._1920.jpg`*

- **Logic**: The server ignores the resolution (`_1920`, `_640`) and uses the hash prefix to calculate an HSL color.
- **Output**: A 1024x600 JPEG with the hash printed on the top right.
