#include "esphome.h"

namespace esphome
{

  EHMTXQueue::EHMTXQueue(EHMTX *config)
  {
    this->config = config;
    this->endtime = 0;
    this->last_time = 0;
    this->screen_time = 0;
    this->mode = MODE_EMPTY;
    this->icon_name = "";
    this->icon = 0;
    this->text = "";
    this->default_font = true;
  }

  void EHMTXQueue::status()
  {
    switch (this->mode)
    {
    case MODE_EMPTY:
      ESP_LOGD(TAG, ("empty slot"));
      break;
    case MODE_BLANK:
      ESP_LOGD(TAG, "queue: blank screen for %d sec", this->screen_time);
      break;
    case MODE_CLOCK:
      ESP_LOGD(TAG, "queue: clock for: %d sec", this->screen_time);
      break;
    case MODE_DATE:
      ESP_LOGD(TAG, "queue: date for: %d sec", this->screen_time);
      break;
    case MODE_FULL_SCREEN:
      ESP_LOGD(TAG, "queue: full screen: \"%s\" for: %d sec", this->icon_name.c_str(), this->screen_time);
      break;
    case MODE_ICON_SCREEN:
      ESP_LOGD(TAG, "queue: icon screen: \"%s\" text: %s for: %d sec", this->icon_name.c_str(), this->text.c_str(), this->screen_time);
      break;
    case MODE_TEXT_SCREEN:
      ESP_LOGD(TAG, "queue: text text: \"%s\" for: %d sec", this->text.c_str(), this->screen_time);
      break;
    case MODE_RAINBOW_ICON:
      ESP_LOGD(TAG, "queue: rainbow icon: \"%s\" text: %s for: %d sec", this->icon_name.c_str(), this->text.c_str(), this->screen_time);
      break;
    case MODE_RAINBOW_TEXT:
      ESP_LOGD(TAG, "queue: rainbow text: \"%s\" for: %d sec", this->text.c_str(), this->screen_time);
      break;
    case MODE_RAINBOW_CLOCK:
      ESP_LOGD(TAG, "queue: clock for: %d sec", this->screen_time);
      break;
    case MODE_RAINBOW_DATE:
      ESP_LOGD(TAG, "queue: date for: %d sec", this->screen_time);
      break;

#ifndef USE_ESP8266
    case MODE_BITMAP_SCREEN:
      ESP_LOGD(TAG, "queue: bitmap for: %d sec", this->screen_time);
      break;
    case MODE_BITMAP_SMALL:
      ESP_LOGD(TAG, "queue: small bitmap for: %d sec", this->screen_time);
      break;
#endif
    default:
      ESP_LOGD(TAG, "queue: UPPS");
      break;
    }
  }

  std::string EHMTXQueue::get_mode_name()
  {
    switch (this->mode)
    {
    case MODE_EMPTY:
      return "empty";
    case MODE_BLANK:
      return "blank";
    case MODE_CLOCK:
    case MODE_RAINBOW_CLOCK:
      return "clock";
    case MODE_DATE:
    case MODE_RAINBOW_DATE:
      return "date";
    case MODE_FULL_SCREEN:
      return "full screen";
    case MODE_ICON_SCREEN:
    case MODE_RAINBOW_ICON:
      return "icon";
    case MODE_TEXT_SCREEN:
    case MODE_RAINBOW_TEXT:
      return "text";
#ifndef USE_ESP8266
    case MODE_BITMAP_SCREEN:
    case MODE_BITMAP_SMALL:
      return "bitmap";
#endif
    default:
      return "unknown";
    }
  }

  int EHMTXQueue::xpos()
  {
    uint8_t width = 32;
    uint8_t startx = 0;
    int result = 0;
    switch (this->mode)
    {
    case MODE_RAINBOW_ICON:
    case MODE_BITMAP_SMALL:
    case MODE_ICON_SCREEN:
      startx = 8;
      break;
    case MODE_TEXT_SCREEN:
    case MODE_RAINBOW_TEXT:
      // no correction
      break;
    default:
      break;
    }

    if (this->config->display_gauge)
    {
      startx += 2;
    }
    width -= startx;

#ifdef EHMTXv3_USE_RTL
    if (this->pixels < width)
    {
      result = 32 - ceil((width - this->pixels) / 2);
    }
    else
    {

      result = startx + this->config->scroll_step;
    }
#else
#ifdef EHMTXv3_SCROLL_SMALL_TEXT
    result = startx - this->config->scroll_step + width;
#else
    if (this->pixels < width)
    {
      result = startx + ceil((width - this->pixels) / 2);
    }
    else
    {
      result = startx - this->config->scroll_step + width;
    }
#endif
#endif
    return result;
  }

  void EHMTXQueue::update_screen()
  {
    if (millis() - this->config->last_rainbow_time >= EHMTXv3_RAINBOW_INTERVALL)
    {
      this->config->hue_++;
      if (this->config->hue_ == 360)
      {
        this->config->hue_ = 0;
      }
      float red, green, blue;
      esphome::hsv_to_rgb(this->config->hue_, 0.8, 0.8, red, green, blue);
      this->config->rainbow_color = Color(uint8_t(255 * red), uint8_t(255 * green), uint8_t(255 * blue));
      this->config->last_rainbow_time = millis();
    }

    if (millis() - this->config->last_anim_time >= this->config->icons[this->icon]->frame_duration)
    {
      this->config->icons[this->icon]->next_frame();
      this->config->last_anim_time = millis();
    }
  }

  void EHMTXQueue::draw()
  {
    display::BaseFont *font = this->default_font ? this->config->default_font : this->config->special_font;
    int8_t yoffset = this->default_font ? EHMTXv3_DEFAULT_FONT_OFFSET_Y : EHMTXv3_SPECIAL_FONT_OFFSET_Y;
    int8_t xoffset = this->default_font ? EHMTXv3_DEFAULT_FONT_OFFSET_X : EHMTXv3_SPECIAL_FONT_OFFSET_X;

    Color color_;
    if (this->config->is_running)
    {
      switch (this->mode)
      {
      case MODE_EMPTY:
        break;
      case MODE_BLANK:
        break;
#ifndef USE_ESP8266
      case MODE_BITMAP_SCREEN:
        for (uint8_t x = 0; x < 32; x++)
        {
          for (uint8_t y = 0; y < 8; y++)
          {
            this->config->display->draw_pixel_at(x, y, this->config->bitmap[x + y * 32]);
          }
        }
        break;
      case MODE_BITMAP_SMALL:
        color_ = this->text_color;
#ifdef EHMTXv3_USE_RTL
        this->config->display->print(this->xpos() + xoffset, yoffset, font, color_, esphome::display::TextAlign::BASELINE_RIGHT,
                                      this->text.c_str());
#else
        this->config->display->print(this->xpos() + xoffset, yoffset, font, color_, esphome::display::TextAlign::BASELINE_LEFT,
                                      this->text.c_str());
#endif
        if (this->config->display_gauge)
        {
          this->config->display->line(10, 0, 10, 7, esphome::display::COLOR_OFF);
          for (uint8_t x = 0; x < 8; x++)
          {
            for (uint8_t y = 0; y < 8; y++)
            {
              this->config->display->draw_pixel_at(x + 2, y, this->config->sbitmap[x + y * 8]);
            }
          }
        }
        else
        {
          this->config->display->line(8, 0, 8, 7, esphome::display::COLOR_OFF);
          for (uint8_t x = 0; x < 8; x++)
          {
            for (uint8_t y = 0; y < 8; y++)
            {
              this->config->display->draw_pixel_at(x, y, this->config->sbitmap[x + y * 8]);
            }
          }
        }

        break;
#endif
      case MODE_RAINBOW_CLOCK:
      case MODE_CLOCK:
        if (this->config->clock->now().is_valid()) // valid time
        {
          color_ = (this->mode == MODE_RAINBOW_CLOCK) ? this->config->rainbow_color : this->text_color;
          time_t ts = this->config->clock->now().timestamp;

          auto xo = this->config->get_show_day_of_month() ? 20 : 15;

          this->config->display->strftime(xoffset + xo, yoffset, font, color_,
                                           display::TextAlign::BASELINE_CENTER,
                                           EHMTXv3_TIME_FORMAT,
                                           this->config->clock->now());

          // TODO: use get_text_bounds to render without time separator
          if (strcmp(EHMTXv3_TIME_FORMAT, "%H:%M") == 0 && this->config->clock->now().second % 2 == 1)
          {
            auto c_tm = this->config->clock->now().to_c_tm();
            size_t buffer_length = 80;
            char temp_buffer[buffer_length];
            strftime(temp_buffer, buffer_length, EHMTXv3_TIME_FORMAT, &c_tm);

            int x1 = 0;
            int y1 = 0;
            int width = 0;
            int height = 0;

            this->config->display->get_text_bounds(xoffset + xo, yoffset, temp_buffer, font, display::TextAlign::BASELINE_CENTER, &x1, &y1, &width, &height);

            int xc = x1 + (width - (width % 2)) / 2;

            this->config->display->filled_rectangle(xc - 2, y1, 3, width, esphome::display::COLOR_OFF);
          }

          if (this->mode != MODE_RAINBOW_CLOCK)
          {
            this->config->draw_day_of_month();
            this->config->draw_day_of_week(false);
          }
        }
        else
        {
          this->config->display->print(15 + xoffset, yoffset, font, this->config->alarm_color, display::TextAlign::BASELINE_CENTER, "!t!");
        }
        break;
      case MODE_RAINBOW_DATE:
      case MODE_DATE:
        if (this->config->clock->now().is_valid())
        {
          color_ = (this->mode == MODE_RAINBOW_DATE) ? this->config->rainbow_color : this->text_color;
          time_t ts = this->config->clock->now().timestamp;
          this->config->display->strftime(xoffset + 15, yoffset, font, color_, display::TextAlign::BASELINE_CENTER, EHMTXv3_DATE_FORMAT,
                                           this->config->clock->now());

          if (this->mode != MODE_RAINBOW_DATE)
          {
            this->config->draw_day_of_week(true);
          }
        }
        else
        {
          this->config->display->print(xoffset + 15, yoffset, font, this->config->alarm_color, display::TextAlign::BASELINE_CENTER, "!d!");
        }
        break;
      case MODE_FULL_SCREEN:
        this->config->display->image(0, 0, this->config->icons[this->icon]);
        break;
      case MODE_ICON_SCREEN:
      case MODE_RAINBOW_ICON:
      {
        color_ = (this->mode == MODE_RAINBOW_ICON) ? this->config->rainbow_color : this->text_color;
#ifdef EHMTXv3_USE_RTL
        this->config->display->print(this->xpos() + xoffset, yoffset, font, color_, esphome::display::TextAlign::BASELINE_RIGHT,
                                      this->text.c_str());
#else
        this->config->display->print(this->xpos() + xoffset, yoffset, font, color_, esphome::display::TextAlign::BASELINE_LEFT,
                                      this->text.c_str());
#endif
        if (this->config->display_gauge)
        {
          this->config->display->image(2, 0, this->config->icons[this->icon]);
          this->config->display->line(10, 0, 10, 7, esphome::display::COLOR_OFF);
        }
        else
        {
          this->config->display->line(8, 0, 8, 7, esphome::display::COLOR_OFF);
          this->config->display->image(0, 0, this->config->icons[this->icon]);
        }
      }
      break;
      case MODE_TEXT_SCREEN:
      case MODE_RAINBOW_TEXT:
        color_ = (this->mode == MODE_RAINBOW_TEXT) ? this->config->rainbow_color : this->text_color;
#ifdef EHMTXv3_USE_RTL
        this->config->display->print(this->xpos() + xoffset, yoffset, font, color_, esphome::display::TextAlign::BASELINE_RIGHT,
                                      this->text.c_str());
#else
        this->config->display->print(this->xpos() + xoffset, yoffset, font, color_, esphome::display::TextAlign::BASELINE_LEFT,
                                      this->text.c_str());
#endif
        break;
      default:
        break;
      }
      this->update_screen();
    }
  }

  void EHMTXQueue::hold_slot(uint8_t _sec)
  {
    this->endtime += _sec;
    ESP_LOGD(TAG, "hold for %d secs", _sec);
  }

  // TODO void EHMTXQueue::set_mode_icon()

  void EHMTXQueue::calc_scroll_time(std::string text, uint16_t screen_time)
  {
    int x, y, w, h;
    float display_duration;

    uint8_t width = 32;
    uint8_t startx = 0;
    uint16_t max_steps = 0;

    if (this->default_font)
    {
      this->config->display->get_text_bounds(0, 0, text.c_str(), this->config->default_font, display::TextAlign::LEFT, &x, &y, &w, &h);
    }
    else
    {
      this->config->display->get_text_bounds(0, 0, text.c_str(), this->config->special_font, display::TextAlign::LEFT, &x, &y, &w, &h);
    }

    this->pixels = w;

    switch (this->mode)
    {
    case MODE_RAINBOW_TEXT:
    case MODE_TEXT_SCREEN:
#ifdef EHMTXv3_SCROLL_SMALL_TEXT
      max_steps = (EHMTXv3_SCROLL_COUNT + 1) * (width - startx) + EHMTXv3_SCROLL_COUNT * this->pixels;
      display_duration = ceil((max_steps * EHMTXv3_SCROLL_INTERVALL) / 1000);
      this->screen_time = (display_duration > screen_time) ? display_duration : screen_time;
#else
      if (this->pixels < 32)
      {
        this->screen_time = screen_time;
      }
      else
      {
        max_steps = (EHMTXv3_SCROLL_COUNT + 1) * (width - startx) + EHMTXv3_SCROLL_COUNT * this->pixels;
        display_duration = ceil((max_steps * EHMTXv3_SCROLL_INTERVALL) / 1000);
        this->screen_time = (display_duration > screen_time) ? display_duration : screen_time;
      }
#endif
      break;
    case MODE_RAINBOW_ICON:
    case MODE_BITMAP_SMALL:
    case MODE_ICON_SCREEN:
      startx = 8;
      if (this->pixels < 23)
      {
        this->screen_time = screen_time;
      }
      else
      {
        max_steps = (EHMTXv3_SCROLL_COUNT + 1) * (width - startx) + EHMTXv3_SCROLL_COUNT * this->pixels;
        display_duration = ceil((max_steps * EHMTXv3_SCROLL_INTERVALL) / 1000);
        this->screen_time = (display_duration > screen_time) ? display_duration : screen_time;
      }
      break;
    default:
      break;
    }

    this->scroll_reset = (width - startx) + this->pixels;
    ;

    ESP_LOGD(TAG, "calc_scroll_time: mode: %d text: \"%s\" pixels %d calculated: %d defined: %d max_steps: %d", this->mode, text.c_str(), this->pixels, this->screen_time, screen_time, this->scroll_reset);
  }
}
