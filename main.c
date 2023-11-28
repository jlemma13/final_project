#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

#include "background.h"
#include "link.h"
#include "map.h"
#include "map2.h"

#define MODE0 0x00
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200

#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000

volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;

#define PALETTE_SIZE 256

#define NUM_SPRITES 128

volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;

volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;

volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;
volatile short* bg1_x_scroll = (unsigned short*) 0x4000014;
volatile short* bg1_y_scroll = (unsigned short*) 0x4000016;

#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 < 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

void wait_vblank() {
    while (*scanline_counter < 160) { }
}

unsigned char button_pressed(unsigned short button) {
    unsigned short pressed = *buttons & button;

    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}

volatile unsigned short* char_block(unsigned long block) {
    return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}

volatile unsigned short* screen_block(unsigned long block) {
    return (volatile unsigned short*) (0x6000000 + (block * 0x800));
}

#define DMA_ENABLE 0x80000000

#define DMA_16 0x00000000
#define DMA_32 0x04000000

volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;

volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;

volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;

void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma_source = (unsigned int) source;
    *dma_destination = (unsigned int) dest;
    *dma_count = amount | DMA_16 | DMA_ENABLE;
}

void setup_background() {
    memcpy16_dma((unsigned short*) bg_palette, (unsigned short*) background_palette, PALETTE_SIZE);

    memcpy16_dma((unsigned short*) char_block(0), (unsigned short*) background_data, (background_width * background_height) / 2);

    /*for (int i = 0; i < PALETTE_SIZE; i++) {
        bg_palette[i] = background_palette[i];
    }

    volatile unsigned short* dest = char_block(0);
    unsigned short* image = (unsigned short*) background_data;
    for (int i = 0; i < ((background_width * background_height) / 2); i++) {
        dest[i] = image[i];
    }*/

    *bg0_control = 1 |
        (0 << 2) |
        (0 << 6) |
        (1 << 7) |
        (16 << 8) |
        (1 << 13) |
        (0 << 14);

    /*dest = screen_block(16);
    for (int i=0; i < (map_width * map_height); i++) {
        dest[i] = map[i];
    }*/

    memcpy16_dma((unsigned short*) screen_block(16), (unsigned short*) map, map_width * map_height);

    *bg1_control = 0 |
        (0 << 2) |
        (0 << 6) |
        (1 << 7) |
        (24 << 8) |
        (1 << 13) |
        (0 << 14);

    /*dest = screen_block(24);
    for (int j = 0; j < (map_width * map_height); j++) {
        dest[j] = map2[j];
    }*/

    memcpy16_dma((unsigned short*) screen_block(24), (unsigned short*) map2, map_width * map_height);
}

void delay(unsigned int amount) {
    for (int i=0; i < amount * 10; i++);
}

struct Sprite {
    unsigned short attribute0;
    unsigned short attribute1;
    unsigned short attribute2;
    unsigned short attribute3;
};

struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

enum SpritesSize {
    SIZE_8_8,
    SIZE_16_16,
    SIZE_32_32,
    SIZE_64_64,
    SIZE_16_8,
    SIZE_32_8,
    SIZE_32_16,
    SIZE_64_32,
    SIZE_8_16,
    SIZE_8_32,
    SIZE_16_32,
    SIZE_32_64
};

struct Sprite* sprite_init(int x, int y, enum SpritesSize size, int horizontal_flip, int vertical_flip, int tile_index, int priority) {
    
    int index = next_sprite_index++;

    int size_bits, shape_bits;
    switch (size) {
        case SIZE_8_8:   size_bits = 0; shape_bits = 0; break;
        case SIZE_16_16: size_bits = 1; shape_bits = 0; break;
        case SIZE_32_32: size_bits = 2; shape_bits = 0; break;
        case SIZE_64_64: size_bits = 3; shape_bits = 0; break;
        case SIZE_16_8:  size_bits = 0; shape_bits = 1; break;
        case SIZE_32_8:  size_bits = 1; shape_bits = 1; break;
        case SIZE_32_16: size_bits = 2; shape_bits = 1; break;
        case SIZE_64_32: size_bits = 3; shape_bits = 1; break;
        case SIZE_8_16:  size_bits = 0; shape_bits = 2; break;
        case SIZE_8_32:  size_bits = 1; shape_bits = 2; break;
        case SIZE_16_32: size_bits = 2; shape_bits = 2; break;
        case SIZE_32_64: size_bits = 3; shape_bits = 2; break;
    }

    int h = horizontal_flip ? 1 : 0;
    int v = vertical_flip ? 1 : 0;

    sprites[index].attribute0 = y |
        (0 << 8) |
        (0 << 10) |
        (0 << 12) |
        (1 << 13) |
        (shape_bits << 14);

    sprites[index].attribute1 = x |
        (0 << 9) |
        (h << 12) |
        (v << 13) |
        (size_bits << 14);

    sprites[index].attribute2 = tile_index |
        (priority << 10) |
        (0 << 12);

    return &sprites[index];
}

void sprite_update_all() {
    memcpy16_dma((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

void sprite_clear() {
    next_sprite_index = 0;

    for (int i = 0; i < NUM_SPRITES; i++) {
        sprites[i].attribute0 = SCREEN_HEIGHT;
        sprites[i].attribute1 = SCREEN_WIDTH;
    }
}

void sprite_position(struct Sprite* sprite, int x, int y) {
    sprite->attribute0 &= 0xff00;
    sprite->attribute0 |= (y & 0xff);
    sprite->attribute1 &= 0xfe00;
    sprite->attribute1 |= (x & 0x1ff);
}

void sprite_move(struct Sprite* sprite, int dx, int dy) {
    int y = sprite->attribute0 & 0xff;
    int x = sprite->attribute1 & 0x1ff;
    sprite_position(sprite, x + dx, y + dy);
}

void sprite_set_vertical_flip(struct Sprite* sprite, int vertical_flip) {
    if (vertical_flip) {
        sprite->attribute1 |= 0x2000;
    } else {
        sprite->attribute1 &= 0xdfff;
    }
}

void sprite_set_horizontal_flip(struct Sprite* sprite, int horizontal_flip) {
    if (horizontal_flip) {
        sprite->attribute1 |= 0x1000;
    } else {
        sprite->attribute1 &= 0xefff;
    }
}

void sprite_set_offset(struct Sprite* sprite, int offset) {
    sprite->attribute2 &= 0xfc00;
    sprite->attribute2 |= (offset & 0x03ff);
}

void setup_sprite_image() {
    // fill out once images are ready
    memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) link_palette, PALETTE_SIZE);
    memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) link_data, (link_width * link_height) / 2);
}

struct Link {
    struct Sprite* sprite;
    int x, y;
    int yvel;
    int gravity;
    int frame;
    int animation_delay;
    int counter;
    int move;
    int border;
    int falling;
};

void link_init(struct Link* link) {
    link->x = 100;
    link->y = 113;
    link->yvel = 0;
    link->gravity = 50;
    link->border = 40;
    link->frame = 0;
    link->move = 0;
    link->counter = 0;
    link->falling = 0;
    link->animation_delay = 8;
    link->sprite = sprite_init(link->x, link->y, SIZE_16_32, 0, 0, link->frame, 0);
}

int link_left(struct Link* link) {
    sprite_set_horizontal_flip(link->sprite, 1);
    link->move = 1;
    if (link->x < link->border) {
        return 1;
    } else {
        link->x--;
        return 0;
    }
}

int link_right(struct Link* link) {
    sprite_set_horizontal_flip(link->sprite, 0);
    link->move = 1;
    if (link->x > (SCREEN_WIDTH - 16 - link->border)) {
        return 1;
    } else {
        link->x++;
        return 0;
    }
}

void link_stop(struct Link* link) {
    link->move = 0;
    link->frame = 0;
    link->counter = 7;
    sprite_set_offset(link->sprite, link->frame);
}

void link_jump(struct Link* link) {
    if (!link->falling) {
        link->yvel = -1350;
        link->falling = 1;
    }
}

unsigned short tile_lookup(int x, int y, int xscroll, int yscroll, const unsigned short* tilemap, int tilemap_w, int tilemap_h) {
    x += xscroll;
    y += yscroll;

    x >>= 3;
    y >>= 3;

    while (x >= tilemap_w) {
        x -= tilemap_w;
    }
    while (y >= tilemap_h) {
        y -= tilemap_h;
    }
    while (x < 0) {
        x += tilemap_w;
    }
    while (y < 0) {
        y += tilemap_h;
    }

    int offset = 0;

    if (tilemap_w == 64 && x >= 32) {
        x -= 32;
        offset += 0x400;
    }

    if (tilemap_h == 64 && y >= 32) {
        y -= 32;
        if (tilemap_w == 64) {
            offset += 0x800;
        } else {
            offset += 0x400;
        }
    }

    int index = y * 32 + x;
    return tilemap[index + offset];
}

void link_update(struct Link* link, int xscroll) {
    if (link->falling) {
        link->y += (link->yvel >> 8);
        link->yvel += link->gravity;
    }

    unsigned short tile = tile_lookup(link->x + 8, link->y + 32, xscroll, 0, map2, map2_width, map2_height);

    if ((tile >= 1 && tile <= 6) || (tile >= 12 && tile <= 17)) {
        link->falling = 0;
        link->yvel = 0;

        link->y &= ~0x3;

        link->y++;
    } else {
        link->falling = 1;
    }

    if (link->move) {
        link->counter++;
        if (link->counter >= link->animation_delay) {
            link->frame = link->frame + 16;
            if (link->frame > 16) {
                link->frame = 0;
            }
            sprite_set_offset(link->sprite, link->frame);
            link->counter = 0;
        }
    }

    sprite_position(link->sprite, link->x, link->y);
}

int main() {
    *display_control = MODE0 | BG0_ENABLE | BG1_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;
    setup_background();
    setup_sprite_image();
    sprite_clear();
    struct Link link;
    link_init(&link);
    int xscroll = 0;
    while (1) {
        link_update(&link, xscroll);
        if (button_pressed(BUTTON_RIGHT)) {
            if (link_right(&link)) {
                xscroll++;
            }
        } else if (button_pressed(BUTTON_LEFT)) {
            if (link_left(&link)) {
                xscroll--;
            }
        } else {
            link_stop(&link);
        }
        
        if (button_pressed(BUTTON_UP)) {
            link_jump(&link);
        }

        wait_vblank();
        *bg0_x_scroll = xscroll;
        *bg1_x_scroll = 2 * xscroll;
        sprite_update_all();
        delay(300);
    }
}

// fill out rest once images are ready to be used
