#include <bc_gfx.h>

void bc_gfx_init(bc_gfx_t *self, void *display, const bc_gfx_driver_t *driver)
{
    memset(self, 0, sizeof(*self));
    self->_display = display;
    self->_driver = driver;
    self->_font = &bc_font_ubuntu_15;

    self->_caps = driver->get_caps(display);
}

bool bc_gfx_display_is_ready(bc_gfx_t *self)
{
    return self->_driver->is_ready(self->_display);
}

void bc_gfx_clear(bc_gfx_t *self)
{
    self->_driver->clear(self->_display);
}

void bc_gfx_set_font(bc_gfx_t *self, const bc_font_t *font)
{
    self->_font = font;
}

void bc_gfx_set_rotation(bc_gfx_t *self, bc_gfx_rotation_t rotation)
{
    self->_rotation = rotation;
}

bc_gfx_rotation_t bc_gfx_get_rotation(bc_gfx_t *self)
{
    return self->_rotation;
}

void bc_gfx_draw_pixel(bc_gfx_t *self, int x, int y, uint32_t color)
{
    if (x >= self->_caps.width || y >= self->_caps.height || x < 0 || y < 0)
    {
        return;
    }

    int tmp;

    switch (self->_rotation)
    {
        case BC_GFX_ROTATION_90:
        {
            tmp = x;
            x = 127 - y;
            y = tmp;
            break;
        }
        case BC_GFX_ROTATION_180:
        {
            x = 127 - x;
            y = 127 - y;
            break;
        }
        case BC_GFX_ROTATION_270:
        {
            tmp = y;
            y = 127 - x;
            x = tmp;
            break;
        }
        case BC_GFX_ROTATION_0:
        {
            break;
        }
        default:
        {
            break;
        }
    }

    self->_driver->draw_pixel(self->_display, x, y, color);
}

int bc_gfx_draw_char(bc_gfx_t *self, int left, int top, uint8_t ch, uint32_t color)
{
    const bc_font_t *font = self->_font;

    int w = 0;
    uint8_t h = 0;
    uint16_t i;
    uint16_t x;
    uint16_t y;
    uint8_t bytes;

    for (i = 0; i < font->length; i++)
    {
        if (font->chars[i].code == ch)
        {
            w = font->chars[i].image->width;
            h = font->chars[i].image->heigth;

            bytes = (w + 7) / 8;

            for (y = 0; y < h; y++)
            {
                for (x = 0; x < w; x++)
                {
                    uint32_t byteIndex = x / 8;
                    byteIndex += y * bytes;

                    uint8_t bitMask = 1 << (7 - (x % 8));

                    if ((font->chars[i].image->image[byteIndex] & bitMask) == 0)
                    {
                        bc_gfx_draw_pixel(self, left + x, top + y, color);
                    }
                }
            }
        }
    }

    return w;
}

int bc_gfx_calc_char_width(bc_gfx_t *self, uint8_t ch)
{
    const bc_font_t *font = self->_font;

    for (int i = 0; i < font->length; i++)
    {
        if (font->chars[i].code == ch)
        {
            return font->chars[i].image->width;
        }
    }

    return 0;
}

int bc_gfx_draw_string(bc_gfx_t *self, int left, int top, char *str, uint32_t color)
{
    while(*str)
    {
        left += bc_gfx_draw_char(self, left, top, *str, color);
        str++;
    }
    return left;
}

int bc_gfx_calc_string_width(bc_gfx_t *self,  char *str)
{
    int width = 0;
    while(*str)
    {
        width += bc_gfx_calc_char_width(self, *str);
        str++;
    }
    return width;
}

void bc_gfx_draw_line(bc_gfx_t *self, int x0, int y0, int x1, int y1, uint32_t color)
{
    int16_t step = abs(y1 - y0) > abs(x1 - x0);
    int16_t tmp;

    if (step)
    {
        tmp = x0;
        x0 = y0;
        y0 = tmp;

        tmp = x1;
        x1 = y1;
        y1 = tmp;
    }

    if (x0 > x1)
    {
        tmp = x0;
        x0 = x1;
        x1 = tmp;

        tmp = y0;
        y0 = y1;
        y1 = tmp;
    }

    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    ystep = y0 < y1 ? 1 : -1;

    for (; x0 <= x1; x0++)
    {
        if (step)
        {
            bc_gfx_draw_pixel(self, y0, x0, color);
        }
        else
        {
            bc_gfx_draw_pixel(self, x0, y0, color);
        }

        err -= dy;

        if (err < 0)
        {
            y0 += ystep;
            err += dx;
        }
    }
}

void bc_gfx_draw_rectangle(bc_gfx_t *self, int x0, int y0, int x1, int y1, uint32_t color)
{
    bc_gfx_draw_line(self, x0, y0, x0, y1, color);
    bc_gfx_draw_line(self, x0, y1, x1, y1, color);
    bc_gfx_draw_line(self, x1, y0, x1, y1, color);
    bc_gfx_draw_line(self, x1, y0, x0, y0, color);
}

// Using Midpoint circle algorithm
void bc_gfx_draw_circle(bc_gfx_t *self, int x0, int y0, int radius, uint32_t color)
{
    int x = radius-1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (radius << 1);

    while (x >= y)
    {
        bc_gfx_draw_pixel(self, x0 + x, y0 + y, color);
        bc_gfx_draw_pixel(self, x0 + y, y0 + x, color);
        bc_gfx_draw_pixel(self, x0 - y, y0 + x, color);
        bc_gfx_draw_pixel(self, x0 - x, y0 + y, color);
        bc_gfx_draw_pixel(self, x0 - x, y0 - y, color);
        bc_gfx_draw_pixel(self, x0 - y, y0 - x, color);
        bc_gfx_draw_pixel(self, x0 + y, y0 - x, color);
        bc_gfx_draw_pixel(self, x0 + x, y0 - y, color);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }
        if (err > 0)
        {
            x--;
            dx += 2;
            err += (-radius << 1) + dx;
        }
    }
}

bool bc_gfx_update(bc_gfx_t *self)
{
    return self->_driver->update(self->_display);
}
