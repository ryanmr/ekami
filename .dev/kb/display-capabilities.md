# E-Paper Display Capabilities

## Display Specs

- 3.97 inches diagonal
- 800 × 480 pixel resolution
- 4 grayscale levels (black, dark gray, light gray, white)
- Wide viewing angle
- Readable in direct sunlight (reflective, no backlight)
- Image persists without power (bistable)

## Refresh Modes

### Full Refresh (~3.5s)
- Flashes the entire screen (black-white-black transition)
- Best image quality, no ghosting
- Use for: initial draw, mode switching, periodic cleanup

### Partial Refresh (~0.6s)
- Updates only changed pixels
- No full-screen flash
- Some ghosting accumulates over many partial refreshes
- Use for: clock updates, sensor readings, small UI changes
- Periodically do a full refresh to clear accumulated ghosting

## Key API Functions (from demo code)

```cpp
// Full init and display
EPD_3IN97_Init_Fast();
EPD_3IN97_Display_Base(BlackImage);

// Drawing primitives
Paint_SelectImage(BlackImage);
Paint_Clear(WHITE);
Paint_DrawPoint(x, y, color, size, style);
// Also: lines, rectangles, circles, strings, images
```

## Layout Considerations at 800×480

- **Landscape orientation**: 800px wide × 480px tall
- Good for badge layout: name on left, QR code on right
- Clock mode: large centered time with smaller date/weather below
- At typical e-paper DPI (~120), text is crisp and readable
- QR codes should be at least 150×150px for reliable scanning
