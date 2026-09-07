#include <pebble.h>
#include "reading.h"
#include "deck.h"

enum { MENU, SPREAD, CARD, MEANING, ABOUT };
static Window *window;
static Layer *canvas;
static ScrollLayer *scroll;
static TextLayer *text_layer;
static Reading reading;
static int screen = MENU, menu_item = 1, width, height;
static bool have_reading, reversals = true;
static uint8_t art[2188]; // Header + 104 * 168 monochrome pixels.
static char meaning[1200], packet[1024];
static int art_w, art_h, art_id = -1;
static GColor felt, paper, ink, gold;
static const char *SEATS[] = {"The situation", "The crossing", "The crown", "The foundation",
  "The past", "What approaches", "Yourself", "Your surroundings", "Hopes and fears", "The outcome"};
static const char *SEAT_NOTES[] = {
  "What surrounds the question now.", "What helps or challenges the situation.",
  "What you aim for, or what could become possible.", "What the situation rests on.",
  "An influence moving into the past.", "An influence beginning to enter.",
  "Your position and approach.", "People and conditions around you.",
  "What you hope for, or hesitate to face.", "Where the present course points."};
static const char *MENU_ITEMS[] = {"Continue reading", "One card", "Three cards", "Celtic Cross",
  "Open table: 5", "Open table: 6", "Open table: 7", "Reversals", "About & controls"};

static void save(void) {
  if (have_reading && persist_write_data(1, &reading, sizeof(reading)) != sizeof(reading))
    APP_LOG(APP_LOG_LEVEL_ERROR, "Reading could not be saved");
  persist_write_bool(2, reversals);
}
static void redraw(void) {
  layer_set_hidden(scroll_layer_get_layer(scroll), screen != MEANING && screen != ABOUT);
  layer_mark_dirty(canvas);
  APP_LOG(APP_LOG_LEVEL_INFO, "Tarot screen=%d menu=%d count=%d selected=%d revealed=%u card=%d reverse=%d",
    screen, menu_item, reading.count, reading.selected, reading.revealed,
    reading.cards[reading.selected], reading.reversed[reading.selected]);
}
static const char *seat(void) {
  if (reading.count == 10) return SEATS[reading.selected];
  if (reading.count == 3) {
    static const char *three[] = {"Past", "Present", "What is emerging"};
    return three[reading.selected];
  }
  if (reading.count == 1) return "A moment to reflect";
  int x = reading.x[reading.selected];
  return x < 39 ? "Past influences" : x > 61 ? "Emerging possibilities" : "The present";
}
static void label(GContext *ctx, const char *text, GRect rect, const char *font, GColor color) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, text, fonts_get_system_font(font), rect, GTextOverflowModeWordWrap,
                     GTextAlignmentCenter, NULL);
}
static void footer(GContext *ctx, const char *text) {
  int inset = PBL_IF_ROUND_ELSE(36, 6);
  label(ctx, text, GRect(inset, height-25, width-2*inset, 20), FONT_KEY_GOTHIC_14, paper);
}
static void backdrop(GContext *ctx) {
  graphics_context_set_fill_color(ctx, felt); graphics_fill_rect(ctx, GRect(0,0,width,height),0,GCornerNone);
#ifdef PBL_COLOR
  graphics_context_set_stroke_color(ctx, GColorDarkGreen);
  for (int x = -height; x < width; x += 9) graphics_draw_line(ctx, GPoint(x,0), GPoint(x+height,height));
#endif
}
static void load_art(void) {
  int id = reading.cards[reading.selected];
  if (id == art_id) return;
  ResHandle handle = resource_get_handle(DECK[id].art);
  size_t size = resource_size(handle);
  art_w = art_h = 0;
  if (size > sizeof(art) || resource_load(handle,art,sizeof(art)) != size || size < 4) return;
  art_w = art[0] | (art[1]<<8); art_h = art[2] | (art[3]<<8);
  if (art_w*art_h/8+4 != (int)size || art_w%8) { art_w = art_h = 0; return; }
  art_id = id;
}
static void card_art(GContext *ctx) {
  load_art();
  if (!art_w) { label(ctx,"Card art unavailable",GRect(20,60,width-40,60),FONT_KEY_GOTHIC_18,paper); return; }
  int left=(width-art_w)/2, top=(height-art_h)/2+2;
  graphics_context_set_fill_color(ctx,gold);
  graphics_fill_rect(ctx,GRect(left-3,top-3,art_w+6,art_h+6),3,GCornersAll);
  graphics_context_set_fill_color(ctx,paper);
  graphics_fill_rect(ctx,GRect(left,top,art_w,art_h),0,GCornerNone);
  graphics_context_set_stroke_color(ctx,ink);
  bool reverse=reading.reversed[reading.selected];
  for (int y=0; y<art_h; y++) {
    int run=-1;
    for (int x=0; x<=art_w; x++) {
      int sx=reverse?art_w-1-x:x, sy=reverse?art_h-1-y:y;
      bool black=x<art_w && !(art[4+sy*(art_w/8)+sx/8] & (0x80>>(sx%8)));
      if (black && run<0) run=x;
      if (!black && run>=0) { graphics_draw_line(ctx,GPoint(left+run,top+y),GPoint(left+x-1,top+y)); run=-1; }
    }
  }
}
static void card_back(GContext *ctx, GRect box, bool selected, const char *number) {
  graphics_context_set_fill_color(ctx, selected ? paper : PBL_IF_COLOR_ELSE(GColorImperialPurple,GColorBlack));
  graphics_fill_rect(ctx,box,3,GCornersAll);
  graphics_context_set_stroke_color(ctx,gold); graphics_draw_round_rect(ctx,box,3);
  GColor detail=selected?ink:paper;
  graphics_context_set_stroke_color(ctx,detail);
  int cx=box.origin.x+box.size.w/2, cy=box.origin.y+box.size.h/2;
  if (number) label(ctx,number,GRect(box.origin.x,cy-9,box.size.w,20),FONT_KEY_GOTHIC_14_BOLD,detail);
  else {
    int r=box.size.w/3;
    graphics_draw_circle(ctx,GPoint(cx,cy),r);
    for (int i=0;i<12;i++) {
      int angle=i*TRIG_MAX_ANGLE/12;
      graphics_draw_line(ctx,GPoint(cx,cy),GPoint(cx+sin_lookup(angle)*r/TRIG_MAX_RATIO,cy+cos_lookup(angle)*r/TRIG_MAX_RATIO));
    }
    graphics_draw_round_rect(ctx,GRect(box.origin.x+5,box.origin.y+5,box.size.w-10,box.size.h-10),1);
  }
}
static void draw_spread(GContext *ctx) {
  static const int cross[10][2]={{36,49},{36,49},{36,18},{36,81},{12,49},{60,49},{85,85},{85,62},{85,38},{85,15}};
  int inset=PBL_IF_ROUND_ELSE(25,10), top=42, area_h=height-82, area_w=width-2*inset;
  int cw=width>=200?20:15, ch=width>=200?29:22;
  for (int pass=0;pass<2;pass++) for (int i=0;i<reading.count;i++) {
    bool active=i==reading.selected;
    if (active != (pass==1)) continue;
    int x,y;
    if (reading.count==10) { x=cross[i][0];y=cross[i][1]; }
    else if (reading.count<=3) { x=reading.count==1?50:20+i*30;y=50; }
    else { x=reading.x[i];y=(reading.y[i]-10)*100/65; }
    int bw=cw,bh=ch;
    if (reading.count==10 && i==1) { bw=ch;bh=cw; }
    GRect box=GRect(inset+x*area_w/100-bw/2,top+y*area_h/100-bh/2,bw,bh);
    char number[4];snprintf(number,sizeof(number),"%d",i+1);
    card_back(ctx,box,active,number);
    if (reading.revealed & (1u<<i)) {
      graphics_context_set_fill_color(ctx,active?ink:paper);
      graphics_fill_circle(ctx,GPoint(box.origin.x+box.size.w-3,box.origin.y+3),2);
    }
  }
  char title[60];snprintf(title,sizeof(title),"%d/%d  %s",reading.selected+1,reading.count,seat());
  label(ctx,title,GRect(PBL_IF_ROUND_ELSE(35,8),PBL_IF_ROUND_ELSE(13,2),width-PBL_IF_ROUND_ELSE(70,16),36),FONT_KEY_GOTHIC_14,paper);
  footer(ctx,"SELECT: open card");
}
static void update(Layer *layer,GContext *ctx) {
  backdrop(ctx);
  if (screen==MENU) {
    label(ctx,"TAROT",GRect(25,PBL_IF_ROUND_ELSE(17,10),width-50,34),FONT_KEY_GOTHIC_24_BOLD,paper);
    int start=menu_item-1;if(start<0)start=0;if(start>6)start=6;
    int row=width>=200?36:30, top=(height-row*3)/2+6;
    for(int i=start;i<start+3;i++) {
      int y=top+(i-start)*row;
      if(i==menu_item) {graphics_context_set_fill_color(ctx,paper);graphics_fill_rect(ctx,GRect(PBL_IF_ROUND_ELSE(24,8),y,width-PBL_IF_ROUND_ELSE(48,16),row),3,GCornersAll);}
      char value[40];snprintf(value,sizeof(value),"%s%s",MENU_ITEMS[i],i==7?(reversals?": on":": off"):"");
      label(ctx,value,GRect(PBL_IF_ROUND_ELSE(26,10),y+3,width-PBL_IF_ROUND_ELSE(52,20),row-3),FONT_KEY_GOTHIC_18,i==menu_item?ink:paper);
    }
    footer(ctx,"UP / DOWN   SELECT");
  } else if(screen==SPREAD) draw_spread(ctx);
  else if(screen==CARD) {
    bool revealed=reading.revealed & (1u<<reading.selected);
    int aw=width>=200?104:72, ah=width>=200?168:116;
    if(revealed) card_art(ctx); else card_back(ctx,GRect((width-aw)/2,(height-ah)/2+2,aw,ah),false,NULL);
    const char *name=revealed?DECK[reading.cards[reading.selected]].name:seat();
    graphics_context_set_text_color(ctx,paper);
    graphics_draw_text(ctx,name,fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(PBL_IF_ROUND_ELSE(38,6),PBL_IF_ROUND_ELSE(13,0),width-PBL_IF_ROUND_ELSE(76,12),19),
      GTextOverflowModeTrailingEllipsis,GTextAlignmentCenter,NULL);
    footer(ctx,revealed?(reading.reversed[reading.selected]?"REVERSED  |  meaning":"UPRIGHT  |  meaning"):"SELECT: reveal");
  } else {
    label(ctx,screen==ABOUT?"Tarot Reader":"THE READING",GRect(25,PBL_IF_ROUND_ELSE(14,7),width-50,24),FONT_KEY_GOTHIC_14_BOLD,paper);
    footer(ctx,"UP / DOWN: scroll");
  }
}
static void show_text(bool about) {
  if(about) snprintf(meaning,sizeof(meaning),
    "Tarot Reader\nLuke Steuber\n\nChoose a spread. Up and Down move between cards. Select opens a card, reveals it, then opens its meaning. Back returns one step.\n\nYour last reading stays on the watch. New draws replace it. Reversals can be turned off for future draws.\n\nArt: Pamela Colman Smith. Text: A. E. Waite, The Pictorial Key to the Tarot (1910), via ekelen/tarot-api. Original artwork and text are public domain.\n\nMeanings quote the historical text, including its dated language. Read them as prompts for reflection.\n\ndatapoems.io");
  else {
    const Card *card=&DECK[reading.cards[reading.selected]];
    memset(packet,0,sizeof(packet));
    resource_load(resource_get_handle(card->text),(uint8_t*)packet,sizeof(packet)-1);
    const char *quote=reading.reversed[reading.selected]?packet+strlen(packet)+1:packet;
    const char *note=reading.count==10?SEAT_NOTES[reading.selected]:reading.count>=5?
      (reading.y[reading.selected]<39?"Higher on the table: visible or outward influences.":"Lower on the table: quieter or underlying influences."):"";
    snprintf(meaning,sizeof(meaning),"%s\n%s\n\n%s\n%s\n\nWaite's meaning\n%s",card->name,
      reading.reversed[reading.selected]?"Reversed":"Upright",seat(),note,quote);
  }
  GRect bounds=layer_get_bounds(scroll_layer_get_layer(scroll));
  GFont font=fonts_get_system_font(FONT_KEY_GOTHIC_18);
  GSize size=graphics_text_layout_get_content_size(meaning,font,GRect(0,0,bounds.size.w-8,2000),GTextOverflowModeWordWrap,GTextAlignmentLeft);
  text_layer_set_text(text_layer,meaning);
  layer_set_frame(text_layer_get_layer(text_layer),GRect(4,0,bounds.size.w-8,size.h+16));
  scroll_layer_set_content_size(scroll,GSize(bounds.size.w,size.h+16));
  scroll_layer_set_content_offset(scroll,GPoint(0,0),false);
  screen=about?ABOUT:MEANING;redraw();
}
static void move(int direction) {
  if(screen==MENU) {menu_item=(menu_item+direction+9)%9;if(!have_reading && menu_item==0)menu_item=direction>0?1:8;}
  else if(screen==CARD || screen==SPREAD) {reading.selected=(reading.selected+direction+reading.count)%reading.count;save();}
  else {GPoint offset=scroll_layer_get_content_offset(scroll);offset.y-=direction*42;scroll_layer_set_content_offset(scroll,offset,false);}
  redraw();
}
static void up(ClickRecognizerRef r,void *c) {move(-1);}
static void down(ClickRecognizerRef r,void *c) {move(1);}
static void select_click(ClickRecognizerRef r,void *c) {
  if(screen==MENU) {
    if(menu_item==8) {show_text(true);return;}
    if(menu_item==7) {reversals=!reversals;save();redraw();return;}
    if(menu_item==0) {if(have_reading)screen=SPREAD;}
    else {
      static const uint8_t counts[]={0,1,3,10,5,6,7};
      time_t seconds;uint16_t ms;time_ms(&seconds,&ms);
      uint32_t seed=(uint32_t)seconds ^ ((uint32_t)ms<<20) ^ (reading.seed*1664525u+1013904223u);
      reading_draw(&reading,counts[menu_item],seed,reversals);have_reading=true;art_id=-1;save();screen=SPREAD;
    }
  } else if(screen==SPREAD) screen=CARD;
  else if(screen==CARD) {
    if(reading.revealed & (1u<<reading.selected)) {show_text(false);return;}
    reading.revealed|=1u<<reading.selected;save();vibes_short_pulse();
  }
  redraw();
}
static void back(ClickRecognizerRef r,void *c) {
  if(screen==MENU) {window_stack_pop(true);return;}
  if(screen==MEANING)screen=CARD;
  else if(screen==CARD)screen=SPREAD;
  else {screen=MENU;menu_item=have_reading?0:1;}
  redraw();
}
static void clicks(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP,180,up);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN,180,down);
  window_single_click_subscribe(BUTTON_ID_SELECT,select_click);
  window_single_click_subscribe(BUTTON_ID_BACK,back);
}
static void init(void) {
  memset(&reading,0,sizeof(reading));
  have_reading=persist_read_data(1,&reading,sizeof(reading))==sizeof(reading) && reading_valid(&reading);
  if(!have_reading)memset(&reading,0,sizeof(reading));
  reversals=persist_exists(2)?persist_read_bool(2):true;
  menu_item=have_reading?0:1;
  felt=PBL_IF_COLOR_ELSE(GColorDarkGreen,GColorBlack);
  paper=GColorWhite;
  ink=GColorBlack;gold=PBL_IF_COLOR_ELSE(GColorBrass,GColorWhite);
  window=window_create();Layer *root=window_get_root_layer(window);GRect bounds=layer_get_bounds(root);
  width=bounds.size.w;height=bounds.size.h;
  canvas=layer_create(bounds);layer_set_update_proc(canvas,update);layer_add_child(root,canvas);
  int margin=PBL_IF_ROUND_ELSE(28,6);
  scroll=scroll_layer_create(GRect(margin,38,width-2*margin,height-69));
  layer_add_child(root,scroll_layer_get_layer(scroll));
  text_layer=text_layer_create(GRect(4,0,width-2*margin-8,100));
  text_layer_set_font(text_layer,fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(text_layer,GColorClear);text_layer_set_text_color(text_layer,paper);
  scroll_layer_add_child(scroll,text_layer_get_layer(text_layer));
  window_set_click_config_provider(window,clicks);window_stack_push(window,true);redraw();
}
static void deinit(void) {
  save();text_layer_destroy(text_layer);scroll_layer_destroy(scroll);layer_destroy(canvas);window_destroy(window);
}
int main(void) {init();app_event_loop();deinit();}
