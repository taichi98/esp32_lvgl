#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <BasicLinearAlgebra.h>
#include <EEPROM.h>
#include "config.h"
#include "screen_manager.h" 

extern SPIClass touchscreenSPI;
extern XPT2046_Touchscreen touchscreen;
static lv_obj_t * back_btn;  // Khai báo toàn cục
int x, y, z;

// Phân khai báo liên quan tới calibrate
String s;

int ts_points[6][2];

/* define the screen points where touch samples will be taken */
const int scr_points[6][2] = { {13, 11}, {20, 220}, {167, 60}, {155, 180}, {300, 13}, {295, 225} };

struct point {
  int x;
  int y;
};

/* pS is a screen point; pT is a resistive touchscreen point */
struct point aS = {scr_points[0][0], scr_points[0][1] };
struct point bS = {scr_points[1][0], scr_points[1][1] };
struct point cS = {scr_points[2][0], scr_points[2][1] };
struct point dS = {scr_points[3][0], scr_points[3][1] };
struct point eS = {scr_points[4][0], scr_points[4][1] };
struct point fS = {scr_points[5][0], scr_points[5][1] };

struct point aT;
struct point bT;
struct point cT;
struct point dT;
struct point eT;
struct point fT;

//Giá trị được lưu sau khi hiệu chỉnh xong
float alphaX, betaX, deltaX, alphaY, betaY, deltaY;
float m_alphaY, m_betaY, m_deltaY;
float pref_alphaX, pref_betaX, pref_deltaX, pref_alphaY, pref_betaY, pref_deltaY;

//Giá trị xài trong chương trình đọc cảm ứng
//float alpha_x, beta_x, alpha_y, beta_y, delta_x, delta_y;

/* Declare function to compute the resistive touchscreen coordinates to display coordinates conversion coefficients */
void ts_calibration (
  const point, const point,
	const point, const point,
	const point, const point,
  const point, const point,
	const point, const point,
	const point, const point
);
void showCalibrationResultScreen();
void check_calibration_results(void);
void compute_transformation_coefficients(void);

// Declare function to get the Raw Touchscreen data
void touchscreen_read_pts(bool reset, bool *finished, int *x_avg, int *y_avg);

/* Declare function to display a user instruction upon startup */
void lv_display_instruction(void);

/* Declare function to display crosshair at indexed point */
void display_crosshair(int);

/* Declare function to display crosshairs at given coordinates */
void display_crosshairs(int, int);

/* Declare function to display 'X's at given coordinates */
void display_xs(int, int);


//PHẦN XỬ LÝ LƯU VÀO EEPROM===================================================
// Giá trị mặc định
const float DEFAULT_ALPHA_X = -0.000f;
const float DEFAULT_BETA_X = 0.088f;
const float DEFAULT_DELTA_X = -26.812f;
const float DEFAULT_ALPHA_Y = 0.068f;
const float DEFAULT_BETA_Y = 0.000f;
const float DEFAULT_DELTA_Y = -17.609f;

// Bắt đầu lưu từ địa Addr2 - địa chỉ 0 lưu biến brightness (chiếm 2 bytes rồi)
void saveCalibrationToEEPROM() { 
  EEPROM.put(SIGNATURE_ADDR, SIGNATURE);
  EEPROM.put(SIGNATURE_ADDR + 4, alphaX);
  EEPROM.put(SIGNATURE_ADDR + 8, betaX);
  EEPROM.put(SIGNATURE_ADDR + 12, deltaX);
  EEPROM.put(SIGNATURE_ADDR + 16, m_alphaY);
  EEPROM.put(SIGNATURE_ADDR + 20, m_betaY);
  EEPROM.put(SIGNATURE_ADDR + 24, m_deltaY);
  
  if(EEPROM.commit()) {
    Serial.println("Calibration saved to EEPROM");
  } else {
    Serial.println("Error saving to EEPROM");
  }
}

void loadCalibrationFromEEPROM() {
  // Kiểm tra signature để biết EEPROM có dữ liệu chưa
  uint32_t sig = 0;
  EEPROM.get(SIGNATURE_ADDR, sig);

  if(sig != SIGNATURE) { // Nếu chưa có dữ liệu
    Serial.println("EEPROM trống, dùng giá trị mặc định");
    alphaX = DEFAULT_ALPHA_X;
    betaX = DEFAULT_BETA_X;
    deltaX = DEFAULT_DELTA_X;
    alphaY = DEFAULT_ALPHA_Y;
    betaY = DEFAULT_BETA_Y;
    deltaY = DEFAULT_DELTA_Y;
  } else {
    EEPROM.get(SIGNATURE_ADDR + 4, alphaX);
    EEPROM.get(SIGNATURE_ADDR + 8, betaX);
    EEPROM.get(SIGNATURE_ADDR + 12, deltaX);
    EEPROM.get(SIGNATURE_ADDR + 16, alphaY);
    EEPROM.get(SIGNATURE_ADDR + 20, betaY);
    EEPROM.get(SIGNATURE_ADDR + 24, deltaY);
      
    Serial.println("Loaded calibration from EEPROM");
    // Serial.print("alpha_x = "); Serial.println(alphaX, 6);
    // Serial.print("beta_x = "); Serial.println(betaX, 6);
    // Serial.print("delta_x = "); Serial.println(deltaX, 6);
    // Serial.print("alpha_y = "); Serial.println(alphaY, 6);
    // Serial.print("beta_y = "); Serial.println(betaY, 6);
    // Serial.print("delta_y = "); Serial.println(deltaY, 6);
  }
}
// =======================
// ĐỌC CẢM ỨNG
// =======================
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();

    x = alphaY * p.x + betaY * p.y + deltaY;
    // clamp x between 0 and SCREEN_WIDTH - 1
    x = max(0, x);
    x = min(SCREEN_WIDTH - 1, x);

    y = alphaX * p.x + betaX * p.y + deltaX;
    // clamp y between 0 and SCREEN_HEIGHT - 1
    y = max(0, y);
    y = min(SCREEN_HEIGHT - 1, y);
    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;

    // Set the coordinates
    data->point.x = x;
    data->point.y = y;
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// =======================
// HIỆU CHỈNH CẢM ỨNG
// =======================

void gather_cal_data(void) {
  //Function to draw the crosshairs and collect data
  bool reset, finished;
  int x_avg, y_avg;

  for (int i = 0; i < 6; i++) {
    lv_obj_clean(lv_scr_act());
    //lv_draw_cross(i);
    display_crosshair(i);

    reset = true;
    x_avg = 0;
    y_avg = 0;
    
    touchscreen_read_pts(reset, &finished, &x_avg, &y_avg);

    reset = false;
    while (!finished) {
      touchscreen_read_pts(reset, &finished, &x_avg, &y_avg);

      /* found out the hard way that if I don't do this, the screen doesn't update */
      lv_task_handler();
      lv_tick_inc(10);
      delay(10);
    }

    lv_obj_clean(lv_scr_act());
    lv_task_handler();
    lv_tick_inc(10);
    delay(10);

    ts_points[i][0] = x_avg;
    ts_points[i][1] = y_avg;

    delay(1000);
  }
  // Clear màn hình trước khi chuyển
  lv_obj_clean(lv_scr_act());
  lv_task_handler();
  delay(10);  // Đủ thời gian hiển thị thông báo

  compute_transformation_coefficients();
}

void compute_transformation_coefficients(void) {
  /* finished collecting data, now compute correction coefficients */
  /* first initialize the function call parameters */
  aT = { ts_points[0][0], ts_points[0][1] };
  bT = { ts_points[1][0], ts_points[1][1] };
  cT = { ts_points[2][0], ts_points[2][1] };
  dT = { ts_points[3][0], ts_points[3][1] };
  eT = { ts_points[4][0], ts_points[4][1] };
  fT = { ts_points[5][0], ts_points[5][1] };

  /* compute the resisitve touchscreen to display coordinates conversion coefficients */
  ts_calibration(aS, aT, bS, bT, cS, cT, dS, dT, eS, eT, fS, fT);
  delay(1000);
  check_calibration_results();
}

void check_calibration_results(void) {
  int x_touch, y_touch, x_scr, y_scr, error;

  for (int i = 0; i < 6; i++) {
    x_touch = ts_points[i][0];
    y_touch = ts_points[i][1];

    x_scr = alphaX * x_touch + betaX * y_touch + deltaX;
    y_scr = alphaY * x_touch + betaY * y_touch + deltaY;

    display_crosshairs(scr_points[i][0], scr_points[i][1]);
    display_xs(x_scr, y_scr);
  }

  Serial.println("******************************************************************");
  Serial.println("USE THE FOLLOWING COEFFICIENT VALUES TO CALIBRATE YOUR TOUCHSCREEN");
  String s;
  s = String("Computed X:  alpha_x = " + String(alphaX, 3) + ", beta_x = " + String(betaX, 3) + ", delta_x = " + String(deltaX, 3) );
  Serial.println(s);
  s = String("Computed Y:  alpha_y = " + String(m_alphaY, 3) + ", beta_y = " + String(m_betaY, 3) + ", delta_y = " + String(m_deltaY, 3) );
  Serial.println(s);
  Serial.println("******************************************************************");
  // Lưu hệ số vào EEPROM
  saveCalibrationToEEPROM();

  // Sau khi lưu xong thì trả về biên mà hàm đọc cảm ứng có thể dùng được.
  alphaY = m_alphaY;
  betaY = m_betaY;
  deltaY = m_deltaY;

  // Lên lịch chuyển màn hình sau 2s
  lv_timer_create([](lv_timer_t* timer){
    showCalibrationResultScreen();
    lv_timer_del(timer);
  }, 2000, NULL);   

}

void touchscreen_read_pts(bool reset, bool *finished, int *x_avg, int *y_avg) {
  static int i, nr_samples, good_samples;
  static uint32_t samples[100][2];

  static float mean_x, mean_y, filt_mean_x, filt_mean_y, stdev_x, stdev_y;

  if (reset) {
    nr_samples = 0;
    *x_avg = 0;
    *y_avg = 0;
    *finished = false;
  }

  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    samples[nr_samples][0] = p.x;
    samples[nr_samples][1] = p.y;

    nr_samples++;

    if (nr_samples >= 100) {
      mean_x = 0;
      mean_y = 0;
      for (i = 0; i < 100; i++) {
        mean_x += (float)samples[i][0];
        mean_y += (float)samples[i][1];
      }
      mean_x = mean_x / (float)nr_samples;
      mean_y = mean_y / (float)nr_samples;

      stdev_x = 0;
      stdev_y = 0;
      for (i = 0; i < 100; i++) {
        stdev_x += sq((float)samples[i][0] - mean_x);
        stdev_y += sq((float)samples[i][1] - mean_y);
      }
      stdev_x = sqrt(stdev_x / (float)nr_samples);
      stdev_y = sqrt(stdev_y / (float)nr_samples);

      good_samples = 0;
      filt_mean_x = 0;
      filt_mean_y = 0;
      for (i = 0; i < 100; i++) {
        if ((abs((float)samples[i][0] - mean_x) < stdev_x) && (abs((float)samples[i][1] - mean_y) < stdev_y)) {
          filt_mean_x += (float)samples[i][0];
          filt_mean_y += (float)samples[i][1];
          good_samples++;
        }        
      }

      filt_mean_x = filt_mean_x / (float)good_samples;
      filt_mean_y = filt_mean_y / (float)good_samples;

      *x_avg = (int)mean_x;
      *y_avg = (int)mean_y;

      *finished = true;
    }
  }
}

/* Function to display a user instruction on startup */
lv_obj_t *create_calibration_screen() {
    // Tạo một màn hình mới
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Label hướng dẫn
    lv_obj_t *text_label = lv_label_create(scr);
    lv_label_set_text(text_label, "Tap each crosshair until it disappears.");
    lv_obj_align(text_label, LV_ALIGN_CENTER, 0, 0);

    static lv_style_t style_text_label;
    lv_style_init(&style_text_label);
    lv_style_set_text_font(&style_text_label, &lv_font_montserrat_14);
    lv_obj_add_style(text_label, &style_text_label, 0);

    return scr;
}

/* function to display crosshair at given index of coordinates array */
void display_crosshair(int cross_nr) {

  static lv_point_precise_t h_line_points[] = { {0, 0}, {10, 0} };
  static lv_point_precise_t v_line_points[] = { {0, 0}, {0, 10} };

  static lv_style_t style_line;
  lv_style_init(&style_line);
  lv_style_set_line_width(&style_line, 2);
  lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_RED));
  lv_style_set_line_rounded(&style_line, true);

  // Create crosshair lines
  lv_obj_t* crosshair_h = lv_line_create(lv_screen_active());
  lv_obj_t* crosshair_v = lv_line_create(lv_screen_active());

  lv_line_set_points(crosshair_h, h_line_points, 2); // Set the coordinates for the crosshair_h line
  lv_obj_add_style(crosshair_h, &style_line, 0);

  lv_line_set_points(crosshair_v, v_line_points, 2); // Set the coordinates for the crosshair_h line
  lv_obj_add_style(crosshair_v, &style_line, 0);

  lv_obj_set_pos(crosshair_h, scr_points[cross_nr][0] - 5, scr_points[cross_nr][1]);
  lv_obj_set_pos(crosshair_v, scr_points[cross_nr][0], scr_points[cross_nr][1] - 5);
}

/* function to display crosshairs at given coordinates */
void display_crosshairs(int x, int y) {

  static lv_point_precise_t h_line_points[] = { {0, 0}, {10, 0} };
  static lv_point_precise_t v_line_points[] = { {0, 0}, {0, 10} };

  static lv_style_t style_line;
  lv_style_init(&style_line);
  lv_style_set_line_width(&style_line, 2);
  lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_BLUE));
  lv_style_set_line_rounded(&style_line, true);

  // Create crosshair lines
  lv_obj_t* crosshair_h = lv_line_create(lv_screen_active());
  lv_obj_t* crosshair_v = lv_line_create(lv_screen_active());

  lv_line_set_points(crosshair_h, h_line_points, 2); // Set the coordinates for the crosshair_h line
  lv_obj_add_style(crosshair_h, &style_line, 0);

  lv_line_set_points(crosshair_v, v_line_points, 2); // Set the coordinates for the crosshair_h line
  lv_obj_add_style(crosshair_v, &style_line, 0);

  lv_obj_set_pos(crosshair_h, x - 5, y);
  lv_obj_set_pos(crosshair_v, x, y - 5);

}

/* function to display 'X's at given coordinates */
void display_xs(int x, int y) {

  static lv_point_precise_t u_line_points[] = { {0, 0}, {10, 10} };  //upsloping
  static lv_point_precise_t d_line_points[] = { {0, 10}, {10, 0} };  //downsloping

  static lv_style_t style_line;
  lv_style_init(&style_line);
  lv_style_set_line_width(&style_line, 2);
  lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_RED));
  lv_style_set_line_rounded(&style_line, true);

  // Create crosshair lines
  lv_obj_t* x_u = lv_line_create(lv_screen_active());
  lv_obj_t* x_d = lv_line_create(lv_screen_active());

  lv_line_set_points(x_u, u_line_points, 2); // Set the coordinates for the upsloping line
  lv_obj_add_style(x_u, &style_line, 0);

  lv_line_set_points(x_d, d_line_points, 2); // Set the coordinates for the downsloping line
  lv_obj_add_style(x_d, &style_line, 0);

  lv_obj_set_pos(x_u, x - 5, y - 5);
  lv_obj_set_pos(x_d, x - 5, y - 5);
}

/* function to compute the transformation equation coefficients from resistive touchscreen
   coordinates to display coordinates...
  This was based on the Texas Instruments appnote at:
  https://www.ti.com/lit/an/slyt277/slyt277.pdf
  It implements Equation 7 of that appnote, which computes a least-squares set of coefficients. */
void ts_calibration(
  const point aS, const point aT,
  const point bS, const point bT,
  const point cS, const point cT,
  const point dS, const point dT,
  const point eS, const point eT,
  const point fS, const point fT) 
{
  BLA::Matrix<6, 3> A = {
    aT.x, aT.y, 1,
    bT.x, bT.y, 1,
    cT.x, cT.y, 1,
    dT.x, dT.y, 1,
    eT.x, eT.y, 1,
    fT.x, fT.y, 1
  };

  BLA::Matrix<6> X = { aS.x, bS.x, cS.x, dS.x, eS.x, fS.x };
  BLA::Matrix<6> Y = { aS.y, bS.y, cS.y, dS.y, eS.y, fS.y };

  BLA::Matrix<3, 6> At = ~A;
  BLA::Matrix<3, 3> B = At * A;

  if (!Invert(B)) {
    Serial.println("Matrix inversion failed!");
    return;
  }

  BLA::Matrix<3, 6> C = B * At;

  BLA::Matrix<3> X_coeff = C * X;
  BLA::Matrix<3> Y_coeff = C * Y;

  alphaX = X_coeff(0); betaX = X_coeff(1); deltaX = X_coeff(2);
  alphaY = Y_coeff(0); betaY = Y_coeff(1); deltaY = Y_coeff(2);
  
  m_alphaY = -alphaY;
  m_betaY = -betaY;
  m_deltaY = SCREEN_WIDTH - deltaY;

}

static void ok_btn_callback(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_async_call([](void *) {
            cleanup_screens();
            switch_to_screen(SCREEN_MAIN);
        }, nullptr);
    }
}


void showCalibrationResultScreen() {
  // Xóa màn hình hiện tại
  lv_obj_clean(lv_scr_act());
  
  // Tạo tiêu đề
  lv_obj_t * title = lv_label_create(lv_scr_act());
  lv_label_set_text(title, "Calibration Results");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
  
  // Hiển thị các hệ số
  lv_obj_t * coeff_label = lv_label_create(lv_scr_act());
  String coeff_text = "X Values:\n";
  coeff_text += String("  alpha_x = ") + String(alphaX, 3) + "\n";
  coeff_text += String("  beta_x = ") + String(betaX, 3) + "\n";
  coeff_text += String("  delta_x = ") + String(deltaX, 3) + "\n\n";
  coeff_text += "Y Values:\n";
  coeff_text += String("  alpha_y = ") + String(m_alphaY, 3) + "\n";
  coeff_text += String("  beta_y = ") + String(m_betaY, 3) + "\n";
  coeff_text += String("  delta_y = ") + String(m_deltaY, 3);
  
  lv_label_set_text(coeff_label, coeff_text.c_str());
  lv_obj_align(coeff_label, LV_ALIGN_CENTER, 0, -10);
  
  // Tạo nút OK
  lv_obj_t * ok_btn = lv_btn_create(lv_scr_act());
  lv_obj_set_size(ok_btn, 60, 30);
  lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_obj_add_event_cb(ok_btn, ok_btn_callback, LV_EVENT_CLICKED, NULL);
  
  lv_obj_t * btn_label = lv_label_create(ok_btn);
  lv_label_set_text(btn_label, "OK");
  lv_obj_center(btn_label);
}
