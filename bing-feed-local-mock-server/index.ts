import { createCanvas } from "@napi-rs/canvas";

interface BingItem {
  title: string;
  copyright: string;
  fullUrl: string;
  thumbUrl: string;
  imageUrl: string;
  pageUrl: string;
  date: string;
}

const randomHash = (): string => {
  return Array.from({ length: 32 }, () =>
    Math.floor(Math.random() * 16).toString(16),
  ).join("");
};

const hashToColor = (hash: string): string => {
  let sum = 0;
  for (let i = 0; i < hash.length; i++) {
    sum += hash.charCodeAt(i);
  }
  const hue = sum % 360;
  return `hsl(${hue}, 70%, 60%)`;
};

const PORT = 3000;

Bun.serve({
  port: PORT,
  async fetch(req) {
    const url = new URL(req.url);

    // Route: /bing/feed
    if (url.pathname === "/bing/feed") {
      const items: BingItem[] = Array.from({ length: 5 }, (_, i) => {
        const hash = randomHash();
        const date = new Date();
        date.setDate(date.getDate() - i);
        const dateStr = date.toISOString().split("T")[0] ?? "";
        const baseUrl = url.origin;
        return {
          title: `Sample Location ${i + 1}`,
          copyright: "\u00A9 Sample/Getty Image",
          fullUrl: `${baseUrl}/${hash}_1920.jpg`,
          thumbUrl: `${baseUrl}/${hash}_640.jpg`,
          imageUrl: `${baseUrl}/${hash}.jpg`,
          pageUrl: `https://peapix.com/bing/${54000 + i}`,
          date: dateStr,
        };
      });
      return Response.json(items);
    }

    // Route: /*.jpg
    if (url.pathname.endsWith(".jpg")) {
      const filename = url.pathname.slice(1, -4);
      const hash = filename.split("_")[0] ?? filename;
      const color = hashToColor(hash);
      const canvas = createCanvas(1024, 600);
      const ctx = canvas.getContext("2d");

      ctx.fillStyle = color;
      ctx.fillRect(0, 0, 1024, 600);
      ctx.fillStyle = "white";
      ctx.font = "20px monospace";
      ctx.textAlign = "right";
      ctx.fillText(hash, 980, 30);

      const buffer = await canvas.encode("jpeg");

      return new Response(buffer, {
        headers: {
          "Content-Type": "image/jpeg",
        },
      });
    }
    return new Response("Not Found", { status: 404 });
  },
});

console.log(`Listening on http://localhost:${PORT}`);
