#include "config.h"

static lv_obj_t *clockLabel;
static lv_obj_t *gridLabel;
static lv_obj_t *latLabel;
static lv_obj_t *lonLabel;
static lv_obj_t *tabview;
static lv_obj_t *tabGPS;
static lv_obj_t *tabSettings;
static lv_obj_t *imgLogo;
static lv_obj_t *kepsInput;
static lv_obj_t *kepsUpdate;
static lv_obj_t *sqfInput; 
static lv_obj_t *sqfUpdate; 
static lv_obj_t *kb; 

uint8_t brightness = 178;

TTGOClass *ttgo;
PCF8563_Class *rtc;
S7XG_Class *s7xg;

LV_FONT_DECLARE(morganite_bold_64);
LV_FONT_DECLARE(morganite_bold_32);
LV_IMG_DECLARE(amsat);
LV_IMG_DECLARE(earth);

void setup()
{
  Serial.begin(115200);
  ttgo = TTGOClass::getWatch();
  ttgo->begin();
  ttgo->openBL();
  ttgo->lvgl_begin();
  rtc = ttgo->rtc;
  //Lower the brightness, move to settings
  ttgo->bl->adjust(brightness);

#ifdef LILYGO_WATCH_2019_WITH_TOUCH
  //! Open s7xg power
  ttgo->enableLDO4();
  ttgo->enableLDO3();

  ttgo->s7xg_begin();
  s7xg = ttgo->s7xg;

  int len = 0;
  int retry = 0;
  do {
    len = s7xg->getHardWareModel().length();
    if (len == 0 && retry++ == 5) {
      s7xg->reset();
      retry = 0;
      Serial.println("Reset s7xg chip");
    }
    if (len == 0)
      delay(1000);
  } while (len == 0);

  s7xg->gpsReset();
  s7xg->gpsSetLevelShift(true);
  s7xg->gpsSetStart();
  s7xg->gpsSetSystem(0);
  s7xg->gpsSetPositioningCycle(1000);
  s7xg->gpsSetPortUplink(20);
  s7xg->gpsSetFormatUplink(1);
  s7xg->gpsSetMode(1);
#endif

  showTabView();
}

static void event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_VALUE_CHANGED) {
        brightness = lv_slider_get_value(obj);
        ttgo->bl->adjust(brightness);
    }
}

static void dropdown_handler(lv_obj_t *obj, lv_event_t event)
{
    if(event == LV_EVENT_VALUE_CHANGED)
    {
      char buf[32];
      lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
    }
}

static void ta_event_cb(lv_obj_t * ta, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) {
        /* Focus on the clicked text area */
        if(kb != NULL)
            lv_keyboard_set_textarea(kb, ta);
    }

    else if(event == LV_EVENT_INSERT) {
        const char * str;// = lv_event_get_data();
        if(str[0] == '\n') {
            printf("Ready\n");
        }
    }
}

void showTabView()
{
  // Setup styles
  static lv_style_t tabStyle;
  lv_style_init(&tabStyle);
  lv_style_set_pad_left(&tabStyle, LV_STATE_DEFAULT, 0);
  lv_style_set_pad_right(&tabStyle, LV_STATE_DEFAULT, 0);
  lv_style_set_pad_top(&tabStyle, LV_STATE_DEFAULT, 0);
  lv_style_set_pad_bottom(&tabStyle, LV_STATE_DEFAULT, 0);
  lv_style_set_margin_left(&tabStyle, LV_STATE_DEFAULT, 0);
  lv_style_set_margin_right(&tabStyle, LV_STATE_DEFAULT, 0);
  lv_style_set_margin_top(&tabStyle, LV_STATE_DEFAULT, 0);
  lv_style_set_margin_bottom(&tabStyle, LV_STATE_DEFAULT, 0);
  lv_style_set_pad_inner(&tabStyle, LV_STATE_DEFAULT, 0);

  static lv_style_t styleLarge;
  lv_style_init(&styleLarge);
  lv_style_set_text_color(&styleLarge, LV_STATE_DEFAULT, LV_COLOR_RED);
  lv_style_set_text_font(&styleLarge, LV_STATE_DEFAULT, &morganite_bold_64);

  static lv_style_t styleSmall;
  lv_style_init(&styleSmall);
  lv_style_set_text_color(&styleSmall, LV_STATE_DEFAULT, LV_COLOR_RED);
  lv_style_set_text_font(&styleSmall, LV_STATE_DEFAULT, &morganite_bold_32);

  // Setup Tab View
  tabview = lv_tabview_create(lv_scr_act(), NULL);
  lv_tabview_set_btns_pos(tabview, LV_TABVIEW_TAB_POS_NONE);
  tabSettings = lv_tabview_add_tab(tabview, "");
  lv_obj_t *tabWatch = lv_tabview_add_tab(tabview, "");
  tabGPS = lv_tabview_add_tab(tabview, "");
  lv_obj_t *tabMap = lv_tabview_add_tab(tabview, "");

  // Style pages to remove margins
  lv_obj_add_style(tabview, LV_TABVIEW_PART_TAB_BG, &tabStyle);
  lv_obj_add_style(tabview, LV_TABVIEW_PART_BG, &tabStyle);
  lv_obj_add_style(tabWatch, LV_PAGE_PART_BG, &tabStyle);
  lv_obj_add_style(tabWatch, LV_PAGE_PART_SCROLLBAR, &tabStyle);
  lv_obj_add_style(tabWatch, LV_PAGE_PART_SCROLLABLE, &tabStyle);
  lv_obj_add_style(tabWatch, LV_PAGE_PART_EDGE_FLASH, &tabStyle);
  lv_obj_add_style(tabMap, LV_PAGE_PART_BG, &tabStyle);
  lv_obj_add_style(tabMap, LV_PAGE_PART_SCROLLBAR, &tabStyle);
  lv_obj_add_style(tabMap, LV_PAGE_PART_SCROLLABLE, &tabStyle);
  lv_obj_add_style(tabMap, LV_PAGE_PART_EDGE_FLASH, &tabStyle);
  lv_obj_add_style(tabSettings, LV_PAGE_PART_BG, &tabStyle);
  lv_obj_add_style(tabSettings, LV_PAGE_PART_SCROLLBAR, &tabStyle);
  lv_obj_add_style(tabSettings, LV_PAGE_PART_SCROLLABLE, &tabStyle);
  lv_obj_add_style(tabSettings, LV_PAGE_PART_EDGE_FLASH, &tabStyle);

  // Setup settings page
  lv_obj_t *brightnessLabel = lv_label_create(tabSettings, NULL);
  lv_obj_add_style(brightnessLabel, LV_OBJ_PART_MAIN, &styleSmall);
  lv_label_set_text(brightnessLabel, "Screen Brightness");
  lv_obj_align(brightnessLabel, tabSettings, LV_ALIGN_IN_TOP_LEFT, 10, 10);

  lv_obj_t *slider = lv_slider_create(tabSettings, NULL);
  lv_obj_set_width(slider, LV_HOR_RES - 40);
  lv_obj_align(slider, brightnessLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 20, 10);
  lv_obj_set_event_cb(slider, event_handler);
  lv_slider_set_range(slider, 100 , 255);
  lv_slider_set_value(slider, brightness, LV_ANIM_OFF);

  lv_obj_t *kepsLabel = lv_label_create(tabSettings, NULL);
  lv_obj_add_style(kepsLabel, LV_OBJ_PART_MAIN, &styleSmall);
  lv_label_set_text(kepsLabel, "Orbital Elements URL");
  lv_obj_align(kepsLabel, slider, LV_ALIGN_IN_TOP_LEFT, -20, 30);

  kepsInput = lv_textarea_create(tabSettings, NULL);
  lv_obj_add_style(kepsInput, LV_OBJ_PART_MAIN, &styleSmall);
  lv_textarea_set_one_line(kepsInput, true);
  lv_obj_set_width(kepsInput, LV_HOR_RES - 20);
  lv_obj_align(kepsInput, kepsLabel, LV_ALIGN_IN_TOP_LEFT, 0, 30);
  lv_textarea_set_text(kepsInput, "https://www.amsat.org/amsat/ftp/keps/current/nasabare.txt");
  lv_textarea_set_cursor_hidden(kepsInput, true);
  lv_obj_set_event_cb(kepsInput, ta_event_cb);

  kepsUpdate = lv_btn_create(tabSettings, NULL);
  lv_obj_set_width(kepsUpdate, LV_HOR_RES - 20);
  lv_obj_align(kepsUpdate, kepsInput, LV_ALIGN_IN_TOP_LEFT, 0, 50);
  lv_obj_add_style(kepsUpdate, LV_BTN_PART_MAIN, &styleSmall);
  lv_obj_set_style_local_value_str(kepsUpdate, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, "Update Elements");
    
  //kb = lv_keyboard_create(tabSettings, NULL);
  //lv_obj_set_size(kb,  LV_HOR_RES, LV_VER_RES / 2);
  //lv_keyboard_set_textarea(kb, kepsInput); /* Focus it on one of the text areas to start */
  //lv_keyboard_set_cursor_manage(kb, true); /* Automatically show/hide cursors on text areas */

  lv_obj_t *sqfLabel = lv_label_create(tabSettings, NULL);
  lv_obj_add_style(sqfLabel, LV_OBJ_PART_MAIN, &styleSmall);
  lv_label_set_text(sqfLabel, "SQF File URL");
  lv_obj_align(sqfLabel, kepsUpdate, LV_ALIGN_IN_TOP_LEFT, 0, 60);

  sqfInput = lv_textarea_create(tabSettings, NULL);
  lv_obj_add_style(sqfInput, LV_OBJ_PART_MAIN, &styleSmall);
  lv_textarea_set_one_line(sqfInput, true);
  lv_obj_set_width(sqfInput, LV_HOR_RES - 20);
  lv_obj_align(sqfInput, sqfLabel, LV_ALIGN_IN_TOP_LEFT, 0, 30);
  lv_textarea_set_text(sqfInput, "https://some.url.to.be.determined/doppler.sqf");
  lv_textarea_set_cursor_hidden(sqfInput, true);
  lv_obj_set_event_cb(sqfInput, ta_event_cb);

  sqfUpdate = lv_btn_create(tabSettings, NULL);
  lv_obj_set_width(sqfUpdate, LV_HOR_RES - 20);
  lv_obj_align(sqfUpdate, sqfInput, LV_ALIGN_IN_TOP_LEFT, 0, 50);
  lv_obj_add_style(sqfUpdate, LV_BTN_PART_MAIN, &styleSmall);
  lv_obj_set_style_local_value_str(sqfUpdate, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, "Update SQF");
  
  // Setup main (clock) page
  imgLogo = lv_img_create(tabWatch, NULL);
  lv_img_set_src(imgLogo, &amsat);
  lv_obj_align(imgLogo, NULL, LV_ALIGN_CENTER, 0, 0);

  clockLabel = lv_label_create(imgLogo, nullptr);
  lv_obj_add_style(clockLabel, LV_OBJ_PART_MAIN, &styleLarge);
  lv_label_set_text(clockLabel, "00:00:00");
  lv_obj_align(clockLabel, imgLogo, LV_ALIGN_IN_TOP_MID, 0, 15);

#ifdef LILYGO_WATCH_2019_WITH_TOUCH

  // Setup GPS/Grid page
  gridLabel = lv_label_create(tabGPS, nullptr);
  lv_obj_add_style(gridLabel, LV_OBJ_PART_MAIN, &styleLarge);
  lv_label_set_text(gridLabel, "No Fix");
  lv_obj_align(gridLabel, tabGPS, LV_ALIGN_CENTER, 0, -70);

  lv_obj_t *latTitleLabel = lv_label_create(tabGPS, nullptr);
  lv_obj_add_style(latTitleLabel, LV_OBJ_PART_MAIN, &styleSmall);
  lv_label_set_text(latTitleLabel, "Latitude");
  lv_obj_align(latTitleLabel, tabGPS, LV_ALIGN_CENTER, 0, 0);
  
  latLabel = lv_label_create(tabGPS, nullptr);
  lv_obj_add_style(latLabel, LV_OBJ_PART_MAIN, &styleSmall);
  lv_label_set_text(latLabel, "No Fix");
  lv_obj_align(latLabel, tabGPS, LV_ALIGN_CENTER, 0, 30);

  lv_obj_t *lonTitleLabel = lv_label_create(tabGPS, nullptr);
  lv_obj_add_style(lonTitleLabel, LV_OBJ_PART_MAIN, &styleSmall);
  lv_label_set_text(lonTitleLabel, "Longitude");
  lv_obj_align(lonTitleLabel, tabGPS, LV_ALIGN_CENTER, 0, 60);
  
  lonLabel = lv_label_create(tabGPS, nullptr);
  lv_obj_add_style(lonLabel, LV_OBJ_PART_MAIN, &styleSmall);
  lv_label_set_text(lonLabel, "No Fix");
  lv_obj_align(lonLabel, tabGPS, LV_ALIGN_CENTER, 0, 90);
#endif

  //Setup earth page
  lv_obj_t *imgEarth = lv_img_create(tabMap, NULL);
  lv_img_set_src(imgEarth, &earth);
  lv_obj_align(imgEarth, NULL, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t *ddlist = lv_dropdown_create(tabMap, NULL);
  // Placeholder items, fill from doppler.sqf
  lv_dropdown_set_options(ddlist, "AO-07\n"
                                  "AO-27\n"
                                  "AO-91\n"
                                  "AO-92\n"
                                  "CAS-4A\n"
                                  "CAS-4B\n"
                                  "EO-88\n"
                                  "FO-29\n"
                                  "HuskySat-1\n"
                                  "ISS\n"
                                  "PO-101\n"
                                  "RS-44\n"
                                  "SO-50");

  lv_obj_align(ddlist, imgEarth, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_event_cb(ddlist, dropdown_handler);
  lv_obj_set_drag(ddlist, true);

  // Default tab
  lv_tabview_set_tab_act(tabview, 1, LV_ANIM_OFF);

  // Updates for GUI
  lv_task_create([](lv_task_t *t) {
    uint16_t currentTab = lv_tabview_get_tab_act(tabview);
    RTC_Date curr_datetime = rtc->getDateTime();
    lv_label_set_text_fmt(clockLabel, "%02u:%02u:%02u", curr_datetime.hour,
                                                        curr_datetime.minute,
                                                        curr_datetime.second);
    lv_obj_align(clockLabel, imgLogo, LV_ALIGN_IN_TOP_MID, 0, 15);

#ifdef LILYGO_WATCH_2019_WITH_TOUCH
    if(currentTab == 2)
    {
      GPS_Class gps =  s7xg->gpsGetData();
      if (gps.isVaild()) {
        double lat = gps.lat();
        double lon = gps.lng();
        lv_label_set_text(gridLabel, string2char(gridSquare(lat,lon)));
        lv_obj_align(gridLabel, tabGPS, LV_ALIGN_CENTER, 0, -60);

        // LVGL has text formatter bug it seems so convert to a string ourself
        char latC[15];
        dtostrf(lat, 11, 6, latC);

        char lonC[15];
        dtostrf(lon, 11, 6, lonC);

        lv_label_set_text_fmt(lonLabel, lonC);
        lv_label_set_text_fmt(latLabel, latC);
        lv_obj_align(latLabel, tabGPS, LV_ALIGN_CENTER, 0, 30);
        lv_obj_align(lonLabel, tabGPS, LV_ALIGN_CENTER, 0, 90);

        //sprintf(buff, "Date: %d/%d/%d",  gps.year(), gps.month(), gps.day());        
        //sprintf(buff, "Time: %d:%d:%d",  gps.hour(), gps.minute(), gps.second());
      } 
    }
 #endif
  }, 1000, LV_TASK_PRIO_MID, nullptr);
  //setCpuFrequencyMhz(20);
}

void loop()
{
    lv_task_handler();
}

//Convenience
char* string2char(String command)
{
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length()-1;
    
    for(int i=0; i<=maxIndex && found<=index; i++)
    {
        if(data.charAt(i)==separator || i==maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1]+1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    
    return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

String gridSquare(float latitude, float longitude)
{
    String grid;
    char m[11];
    const int pairs=5;
    const double scaling[]={360.,360./18.,(360./18.)/10., \
        ((360./18.)/10.)/24.,(((360./18.)/10.)/24.)/10., \
        ((((360./18.)/10.)/24.)/10.)/24., \
        (((((360./18.)/10.)/24.)/10.)/24.)/10.,
        ((((((360./18.)/10.)/24.)/10.)/24.)/10.)/24.};
    int i;
    int index;
    
    for (i=0;i<pairs;i++)
    {
        index = (int)floor(fmod((180.0+longitude), scaling[i])/scaling[i+1]);
        m[i*2] = (i&1) ? 0x30+index : (i&2 || i&5) ? 0x61+index : 0x41+index;
        index = (int)floor(fmod((90.0+latitude), (scaling[i]/2))/(scaling[i+1]/2));
        m[i*2+1] = (i&1) ? 0x30+index : (i&2 || i&5) ? 0x61+index : 0x41+index;
    }
    m[pairs*2]=0;
    return String(m);
}

void readSettings()
{
  
}

void writeSettings()
{
  
}
