#include "ArduboyVeritazz.h"

ArduboyVeritazz::ArduboyVeritazz()
{
  // frame management
  setFrameRate(60);
  frameCount = 0;
  nextFrameStart = 0;
  post_render = false;

  // init not necessary, will be reset after first use
  // lastFrameStart
  // lastFrameDurationMs
}

// functions called here should be public so users can create their
// own init functions if they need different behavior than `begin`
// provides by default
void ArduboyVeritazz::begin()
{
  boot(); // raw hardware

  // utils
  if(buttonsState() & UP_BUTTON) {
    doNothing();
  }
}

void ArduboyVeritazz::doNothing()
{
  blank();
  while(!(buttonsState() & DOWN_BUTTON)) {
    idle();
  }
}

/* Frame management */

void ArduboyVeritazz::setFrameRate(uint8_t rate)
{
  frameRate = rate;
  eachFrameMillis = 1000/rate;
}

uint8_t ArduboyVeritazz::everyXFrames(uint8_t frames)
{
  return frameCount % frames == 0;
}

uint8_t ArduboyVeritazz::nextFrame()
{
  long now = millis();
  uint8_t remaining;

  // post render
  if (post_render) {
    lastFrameDurationMs = now - lastFrameStart;
    frameCount++;
    post_render = false;
  }

  // if it's not time for the next frame yet
  if (now < nextFrameStart) {
    remaining = nextFrameStart - now;
    // if we have more than 1ms to spare, lets sleep
    // we should be woken up by timer0 every 1ms, so this should be ok
    if (remaining > 1)
      idle();

    return false;
  }

  // pre-render

  // next frame should start from last frame start + frame duration
  nextFrameStart = lastFrameStart + eachFrameMillis;
  // If we're running CPU at 100%+ (too slow to complete each loop within
  // the frame duration) then it's possible that we get "behind"... Say we
  // took 5ms too long, resulting in nextFrameStart being 5ms in the PAST.
  // In that case we simply schedule the next frame to start immediately.
  //
  // If we were to let the nextFrameStart slide further and further into
  // the past AND eventually the CPU usage dropped then frame management
  // would try to "catch up" (by speeding up the game) to make up for all
  // that lost time.  That would not be good.  We allow frames to take too
  // long (what choice do we have?), but we do not allow super-fast frames
  // to make up for slow frames in the past.

  if (nextFrameStart < now) {
    nextFrameStart = now;
  }

  lastFrameStart = now;

  post_render = true;
  return post_render;
}

int ArduboyVeritazz::cpuLoad()
{
  return lastFrameDurationMs*100 / eachFrameMillis;
}

void ArduboyVeritazz::initRandomSeed()
{
#ifndef HOST_TEST
  power_adc_enable(); // ADC on
  randomSeed(~rawADC(ADC_TEMP) * ~rawADC(ADC_VOLTAGE) * ~micros() + micros());
  power_adc_disable(); // ADC off
#endif
}

uint16_t ArduboyVeritazz::rawADC(uint8_t adc_bits)
{
#ifndef HOST_TEST
  ADMUX = adc_bits;
  // we also need MUX5 for temperature check
  if (adc_bits == ADC_TEMP) {
    ADCSRB = _BV(MUX5);
  }

  delay(2); // Wait for ADMUX setting to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  return ADC;
#endif
  return 0;
}

/* Graphics */

void ArduboyVeritazz::clear()
{
  fillScreen(BLACK);
}

void ArduboyVeritazz::drawPixel(int x, int y, uint8_t color)
{
  #ifdef PIXEL_SAFE_MODE
  if (x < 0 || x > (WIDTH-1) || y < 0 || y > (HEIGHT-1))
  {
    return;
  }
  #endif

  uint8_t row = (uint8_t)y / 8;
  if (color)
  {
    sBuffer[(row*WIDTH) + (uint8_t)x] |=   _BV((uint8_t)y % 8);
  }
  else
  {
    sBuffer[(row*WIDTH) + (uint8_t)x] &= ~ _BV((uint8_t)y % 8);
  }
}

uint8_t ArduboyVeritazz::getPixel(uint8_t x, uint8_t y)
{
  uint8_t row = y / 8;
  uint8_t bit_position = y % 8;
  return (sBuffer[(row*WIDTH) + x] & _BV(bit_position)) >> bit_position;
}

void ArduboyVeritazz::drawCircle(int16_t x0, int16_t y0, uint8_t r, uint8_t color)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  drawPixel(x0, y0+r, color);
  drawPixel(x0, y0-r, color);
  drawPixel(x0+r, y0, color);
  drawPixel(x0-r, y0, color);

  while (x<y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }

    x++;
    ddF_x += 2;
    f += ddF_x;

    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void ArduboyVeritazz::drawCircleHelper
(int16_t x0, int16_t y0, uint8_t r, uint8_t cornername, uint8_t color)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x<y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }

    x++;
    ddF_x += 2;
    f += ddF_x;

    if (cornername & 0x4)
    {
      drawPixel(x0 + x, y0 + y, color);
      drawPixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2)
    {
      drawPixel(x0 + x, y0 - y, color);
      drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8)
    {
      drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1)
    {
      drawPixel(x0 - y, y0 - x, color);
      drawPixel(x0 - x, y0 - y, color);
    }
  }
}

void ArduboyVeritazz::fillCircle(int16_t x0, int16_t y0, uint8_t r, uint8_t color)
{
  drawFastVLine(x0, y0-r, 2*r+1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

void ArduboyVeritazz::fillCircleHelper
(int16_t x0, int16_t y0, uint8_t r, uint8_t cornername, int16_t delta,
 uint8_t color)
{
  // used to do circles and roundrects!
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x < y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }

    x++;
    ddF_x += 2;
    f += ddF_x;

    if (cornername & 0x1)
    {
      drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
    }

    if (cornername & 0x2)
    {
      drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}

void ArduboyVeritazz::drawLine
(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
  // bresenham's algorithm - thx wikpedia
  uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int8_t ystep;

  if (y0 < y1)
  {
    ystep = 1;
  }
  else
  {
    ystep = -1;
  }

  for (; x0 <= x1; x0++)
  {
    if (steep)
    {
      drawPixel(y0, x0, color);
    }
    else
    {
      drawPixel(x0, y0, color);
    }

    err -= dy;
    if (err < 0)
    {
      y0 += ystep;
      err += dx;
    }
  }
}

void ArduboyVeritazz::drawRect
(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color)
{
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y+h-1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x+w-1, y, h, color);
}

void ArduboyVeritazz::drawFastVLine
(int16_t x, int16_t y, uint8_t h, uint8_t color)
{
  int end = y+h;
  for (int a = max(0,y); a < min(end,HEIGHT); a++)
  {
    drawPixel(x,a,color);
  }
}

void ArduboyVeritazz::drawFastHLine
(int16_t x, int16_t y, uint8_t w, uint8_t color)
{
  // Do bounds/limit checks
  if (y < 0 || y >= HEIGHT) {
    return;
  }

  // make sure we don't try to draw below 0
  if (x < 0) {
    w += x;
    x = 0;
  }

  // make sure we don't go off the edge of the display
  if ((x + w) > WIDTH) {
    w = (WIDTH - x);
  }

  // if our width is now negative, punt
  if (w <= 0) {
    return;
  }

  // buffer pointer plus row offset + x offset
  register uint8_t *pBuf = sBuffer + ((y/8) * WIDTH) + x;

  // pixel mask
  register uint8_t mask = 1 << (y&7);

  switch (color)
  {
    case WHITE:
      while(w--) {
        *pBuf++ |= mask;
      };
      break;

    case BLACK:
      mask = ~mask;
      while(w--) {
        *pBuf++ &= mask;
      };
      break;
  }
}

void ArduboyVeritazz::fillRect
(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color)
{
  // stupidest version - update in subclasses if desired!
  for (int16_t i=x; i<x+w; i++)
  {
    drawFastVLine(i, y, h, color);
  }
}

void ArduboyVeritazz::fillScreen(uint8_t color)
{
#ifndef HOST_TEST
  // C version :
  //
  // if (color) color = 0xFF;  //change any nonzero argument to b11111111 and insert into screen array.
  // for(int16_t i=0; i<1024; i++)  { sBuffer[i] = color; }  //sBuffer = (128*64) = 8192/8 = 1024 bytes.

  asm volatile
  (
    // load color value into r27
    "mov r27, %1 \n\t"
    // if value is zero, skip assigning to 0xff
    "cpse r27, __zero_reg__ \n\t"
    "ldi r27, 0xff \n\t"
    // load sBuffer pointer into Z
    "movw  r30, %0\n\t"
    // counter = 0
    "clr __tmp_reg__ \n\t"
    "loopto:   \n\t"
    // (4x) push zero into screen buffer,
    // then increment buffer position
    "st Z+, r27 \n\t"
    "st Z+, r27 \n\t"
    "st Z+, r27 \n\t"
    "st Z+, r27 \n\t"
    // increase counter
    "inc __tmp_reg__ \n\t"
    // repeat for 256 loops
    // (until counter rolls over back to 0)
    "brne loopto \n\t"
    // input: sBuffer, color
    // modified: Z (r30, r31), r27
    :
    : "r" (sBuffer), "r" (color)
    : "r30", "r31", "r27"
  );
#else
  if (color) color = 0xFF;  //change any nonzero argument to b11111111 and insert into screen array.
  memset(sBuffer, color, WIDTH * HEIGHT / 8);
#endif
}

void ArduboyVeritazz::drawRoundRect
(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t color)
{
  // smarter version
  drawFastHLine(x+r, y, w-2*r, color); // Top
  drawFastHLine(x+r, y+h-1, w-2*r, color); // Bottom
  drawFastVLine(x, y+r, h-2*r, color); // Left
  drawFastVLine(x+w-1, y+r, h-2*r, color); // Right
  // draw four corners
  drawCircleHelper(x+r, y+r, r, 1, color);
  drawCircleHelper(x+w-r-1, y+r, r, 2, color);
  drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  drawCircleHelper(x+r, y+h-r-1, r, 8, color);
}

void ArduboyVeritazz::fillRoundRect
(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t r, uint8_t color)
{
  // smarter version
  fillRect(x+r, y, w-2*r, h, color);

  // draw four corners
  fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  fillCircleHelper(x+r, y+r, r, 2, h-2*r-1, color);
}

void ArduboyVeritazz::drawTriangle
(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color)
{
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
}

void ArduboyVeritazz::fillTriangle
(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color)
{

  int16_t a, b, y, last;
  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1)
  {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2)
  {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1)
  {
    swap(y0, y1); swap(x0, x1);
  }

  if(y0 == y2)
  { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)
    {
      a = x1;
    }
    else if(x1 > b)
    {
      b = x1;
    }
    if(x2 < a)
    {
      a = x2;
    }
    else if(x2 > b)
    {
      b = x2;
    }
    drawFastHLine(a, y0, b-a+1, color);
    return;
  }

  int16_t dx01 = x1 - x0,
      dy01 = y1 - y0,
      dx02 = x2 - x0,
      dy02 = y2 - y0,
      dx12 = x2 - x1,
      dy12 = y2 - y1,
      sa = 0,
      sb = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if (y1 == y2)
  {
    last = y1;   // Include y1 scanline
  }
  else
  {
    last = y1-1; // Skip it
  }


  for(y = y0; y <= last; y++)
  {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;

    if(a > b)
    {
      swap(a,b);
    }

    drawFastHLine(a, y, b-a+1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);

  for(; y <= y2; y++)
  {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;

    if(a > b)
    {
      swap(a,b);
    }

    drawFastHLine(a, y, b-a+1, color);
  }
}

void ArduboyVeritazz::drawBitmap
(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h,
 uint8_t color)
{
  // no need to dar at all of we're offscreen
  if (x+w < 0 || x > WIDTH-1 || y+h < 0 || y > HEIGHT-1)
    return;

  int yOffset = abs(y) % 8;
  int sRow = y / 8;
  if (y < 0) {
    sRow--;
    yOffset = 8 - yOffset;
  }
  int rows = h/8;
  if (h%8!=0) rows++;
  for (int a = 0; a < rows; a++) {
    int bRow = sRow + a;
    if (bRow > (HEIGHT/8)-1) break;
    if (bRow > -2) {
      for (int iCol = 0; iCol<w; iCol++) {
        if (iCol + x > (WIDTH-1)) break;
        if (iCol + x >= 0) {
          if (bRow >= 0) {
            if      (color == WHITE) this->sBuffer[ (bRow*WIDTH) + x + iCol ] |= pgm_read_byte(bitmap+(a*w)+iCol) << yOffset;
            else if (color == BLACK) this->sBuffer[ (bRow*WIDTH) + x + iCol ] &= ~(pgm_read_byte(bitmap+(a*w)+iCol) << yOffset);
            else                     this->sBuffer[ (bRow*WIDTH) + x + iCol ] ^= pgm_read_byte(bitmap+(a*w)+iCol) << yOffset;
          }
          if (yOffset && bRow<(HEIGHT/8)-1 && bRow > -2) {
            if      (color == WHITE) this->sBuffer[ ((bRow+1)*WIDTH) + x + iCol ] |= pgm_read_byte(bitmap+(a*w)+iCol) >> (8-yOffset);
            else if (color == BLACK) this->sBuffer[ ((bRow+1)*WIDTH) + x + iCol ] &= ~(pgm_read_byte(bitmap+(a*w)+iCol) >> (8-yOffset));
            else                     this->sBuffer[ ((bRow+1)*WIDTH) + x + iCol ] ^= pgm_read_byte(bitmap+(a*w)+iCol) >> (8-yOffset);
          }
        }
      }
    }
  }
}


void ArduboyVeritazz::drawSlowXYBitmap
(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color)
{
  // no need to dar at all of we're offscreen
  if (x+w < 0 || x > WIDTH-1 || y+h < 0 || y > HEIGHT-1)
    return;

  int16_t xi, yi, byteWidth = (w + 7) / 8;
  for(yi = 0; yi < h; yi++) {
    for(xi = 0; xi < w; xi++ ) {
      if(pgm_read_byte(bitmap + yi * byteWidth + xi / 8) & (128 >> (xi & 7))) {
        drawPixel(x + xi, y + yi, color);
      }
    }
  }
}

#ifdef HOST_TEST
extern void update_screen();
#endif

void ArduboyVeritazz::display()
{
#ifndef HOST_TEST
  this->paintScreen(sBuffer);
#else
  update_screen();
#endif
}

unsigned char* ArduboyVeritazz::getBuffer()
{
  return sBuffer;
}

void ArduboyVeritazz::swap(int16_t& a, int16_t& b)
{
  int temp = a;
  a = b;
  b = temp;
}

/* simple_buttons */

void ArduboyVeritazz::poll()
{
  previousButtonState = currentButtonState;
  currentButtonState = buttonsState();
  #if defined(SOFT_RESET) && !defined(HOST_TEST)
  if (currentButtonState==(LEFT_BUTTON|RIGHT_BUTTON|UP_BUTTON|DOWN_BUTTON)) {
    *(uint16_t *)0x0800 = 0x7777;
    wdt_enable(WDTO_15MS);
    while(true) {}
  }
  #endif
}

// returns true if the button mask passed in is pressed
//
//   if (pressed(LEFT_BUTTON + A_BUTTON))
uint8_t ArduboyVeritazz::pressed(uint8_t buttons)
{
 return (currentButtonState & buttons) == buttons;
}

// returns true if a button that has been held down was just released
// this function only reliably works with a single button. You should not
// pass it multiple buttons as you can with some of the other button
// functions.
//
// This can be used for confirmations or other times when you want to take
// an action AFTER the user finishes the pressing rather than immediately
// when the button goes down.  Not that there is any good way for someone
// to change their mind, but the experience can feel very different.
uint8_t ArduboyVeritazz::justReleased(uint8_t button)
{
 return ((previousButtonState & button) && !(currentButtonState & button));
}

// returns true if a button has just been pressed
// if the button has been held down for multiple frames this will return
// false.  You should only use this to poll a single button.
uint8_t ArduboyVeritazz::justPressed(uint8_t button)
{
 return (!(previousButtonState & button) && (currentButtonState & button));
}

// returns true if the button mask passed in not pressed
//
//   if (not_pressed(LEFT_BUTTON))
uint8_t ArduboyVeritazz::notPressed(uint8_t buttons)
{
  return (currentButtonState & buttons) == 0;
}

uint8_t ArduboyVeritazz::up() {
  return pressed(UP_BUTTON);
}

uint8_t ArduboyVeritazz::down() {
  return pressed(DOWN_BUTTON);
}

uint8_t ArduboyVeritazz::right() {
  return pressed(RIGHT_BUTTON);
}

uint8_t ArduboyVeritazz::left() {
  return pressed(LEFT_BUTTON);
}

uint8_t ArduboyVeritazz::a() {
  return pressed(A_BUTTON);
}

uint8_t ArduboyVeritazz::b() {
  return pressed(B_BUTTON);
}



uint8_t ArduboyVeritazz::pressedUp() {
  return justPressed(UP_BUTTON);
}

uint8_t ArduboyVeritazz::pressedDown() {
  return justPressed(DOWN_BUTTON);
}

uint8_t ArduboyVeritazz::pressedRight() {
  return justPressed(RIGHT_BUTTON);
}

uint8_t ArduboyVeritazz::pressedLeft() {
  return justPressed(LEFT_BUTTON);
}

uint8_t ArduboyVeritazz::pressedA() {
  return justPressed(A_BUTTON);
}

uint8_t ArduboyVeritazz::pressedB() {
  return justPressed(B_BUTTON);
}
