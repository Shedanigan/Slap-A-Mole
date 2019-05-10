#include <Adafruit_NeoPixel.h>

//system vars
const unsigned int rows = 6;
const unsigned int cols = 4;
const unsigned int area = rows * cols;
unsigned int game_state = 0;
unsigned long current_time = 0;
unsigned long start_time = 0;
unsigned long last_ring_update = 0;
unsigned int animation_state = 0;
const unsigned int animation_lag = 100;

//output vars
const unsigned int led_offset = 2;
unsigned int led_pins[area];
unsigned int led_buffer[area];

const int ring_pin = A1;
const unsigned int ring_leds = 24;
Adafruit_NeoPixel ring(ring_leds, ring_pin, NEO_GRBW + NEO_KHZ800);
char ring_r[ring_leds];
char ring_g[ring_leds];
char ring_b[ring_leds];
char ring_w[ring_leds];

const int buzzer_pin = A0;
const int low_buzz = 523;
const int high_buzz = 784;
int buzz_tone[8];

//input vars
const unsigned int but_offset = 26;
unsigned int but_pins[area];
unsigned int curr_states[area];
unsigned int prev_states[area];
int red_but_pins[area / 2];
int r_curr_states[area / 2];
int blue_but_pins[area / 2];
int b_curr_states[area / 2];
int *blue_leds[area / 2];
int *red_leds[area / 2];

//system functions
void check_All_Buttons()
{
  for (int i = 0; i < area; i++)
  {
    prev_states[i] = curr_states[i];
    pinMode(but_pins[i], OUTPUT);
    digitalWrite(but_pins[i], HIGH);
    pinMode(but_pins[i], INPUT);
    curr_states[i] = digitalRead(but_pins[i]);
  }
}

void check_RB_Buttons()
{
  for (int i = 0; i < area / 2; i++)
  {
    pinMode(red_but_pins[i], OUTPUT);
    digitalWrite(red_but_pins[i], HIGH);
    pinMode(red_but_pins[i], INPUT);
    r_curr_states[i] = digitalRead(red_but_pins[i]);
    
    pinMode(blue_but_pins[i], OUTPUT);
    digitalWrite(blue_but_pins[i], HIGH);
    pinMode(blue_but_pins[i], INPUT);
    b_curr_states[i] = digitalRead(blue_but_pins[i]);
  }
}

//buffer functions
void print_Buffer()
{
  for (int i = 0; i < area; i++) digitalWrite(led_pins[i], led_buffer[i]);
}

void fill_Buffer(int state)
{
  for (int i = 0; i < area; i++) led_buffer[i] = state;
}

void fill_Row(unsigned int row, int state)
{
  for (int i = 0; i < cols; i++) led_buffer[i + cols * row] = state;
}

void fill_Col(unsigned int col, int state)
{
  for (int i = 0; i < rows; i++) led_buffer[i * cols + col] = state;
}

void toggle_Row(unsigned int row)
{
  for (int i = 0; i < cols; i++) 
  {
    if (led_buffer[i + cols * row] == HIGH) led_buffer[i + cols * row] = LOW;
    else led_buffer[i + cols * row] = HIGH;
  }
}

void toggle_Col(unsigned int col)
{
  for (int i = 0; i < rows; i++) 
  {
    if (led_buffer[i * cols + col] == HIGH) led_buffer[i * cols + col] = LOW;
    else led_buffer[i * cols + col] = HIGH;
  }
}

void roll_Buf_L()
{
  int temp_col[rows];
  for (int i = 0; i < rows; i++) temp_col[i] = led_buffer[i * cols];
  for (int y = 0; y < rows; y++)
  {
    for (int x = 0; x < cols - 1; x++)
    {
      int index = y * cols + x;
      led_buffer[index] = led_buffer[index + 1];
    }
    led_buffer[(y + 1) * cols - 1] = temp_col[y];
  }
}

void roll_Buf_R()
{
  int temp_col[rows];
  for (int i = 0; i < rows; i++) temp_col[i] = led_buffer[(i + 1) * cols - 1];
  for (int y = 0; y < rows; y++)
  {
    for (int x = cols - 1; x > 0; x--)
    {
      int index = y * cols + x;
      led_buffer[index] = led_buffer[index - 1];
    }
    led_buffer[y * cols] = temp_col[y];
  }
}

void roll_Buf_U()
{
  int temp_col[rows];
  for (int i = 0; i < cols; i++) temp_col[i] = led_buffer[i];
  for (int x = 0; x < cols; x++)
  {
    for (int y = 0; y < rows - 1; y++)
    {
      int index = y * cols + x;
      led_buffer[index] = led_buffer[index + cols];
    }
    led_buffer[(rows - 1) * cols + x] = temp_col[x];
  }
}

void roll_Buf_D()
{
  int temp_col[rows];
  for (int i = 0; i < cols; i++) temp_col[i] = led_buffer[i + cols * (rows - 1)];
  for (int x = 0; x < cols; x++)
  {
    for (int y = rows - 1; y > 0; y--)
    {
      int index = y * cols + x;
      led_buffer[index] = led_buffer[index + cols];
    }
    led_buffer[x] = temp_col[x];
  }
}

//ring functions
void print_Ring()
{
  for (int i = 0; i < ring_leds; i++) ring.setPixelColor(i, ring_r[i], ring_g[i], ring_b[i], ring_w[i]);
  ring.show();
}

void clear_Ring()
{
  for (int i = 0; i < ring_leds; i++)
  {
    ring_r[i] = 0;
    ring_g[i] = 0;
    ring_b[i] = 0;
    ring_w[i] = 0;
  }
}

void fill_Ring(char r, char g, char b, char w)
{
  for (int i = 0; i < ring_leds; i++)
  {
    ring_r[i] = r;
    ring_g[i] = g;
    ring_b[i] = b;
    ring_w[i] = w;
  }
}

void rainbow_Ring(char brightness)
{
  for (int i = 0; i < ring_leds / 3; i++)
  {
    ring_r[i] = brightness - (brightness * i / (ring_leds / 3));
    ring_g[i] = brightness * i / (ring_leds / 3);
    ring_b[i] = 0;
    ring_w[i] = 0;
  }
  for (int i = ring_leds / 3; i < 2 * ring_leds / 3; i++)
  {
    ring_r[i] = 0;
    ring_g[i] = brightness - brightness * (i - ring_leds / 3) / (ring_leds / 3);
    ring_b[i] = brightness * (i - ring_leds / 3) / (ring_leds / 3);
    ring_w[i] = 0;
  }
  for (int i = 2 * ring_leds / 3; i < ring_leds; i++)
  {
    ring_r[i] = brightness * (i - 2 * ring_leds / 3) / (ring_leds / 3);
    ring_g[i] = 0;
    ring_b[i] = brightness - brightness * (i - 2 * ring_leds / 3) / (ring_leds / 3);
    ring_w[i] = 0;
  }
}

void shift_Ring_CW()
{
  char temp_r = ring_r[0];
  char temp_g = ring_g[0];
  char temp_b = ring_b[0];
  char temp_w = ring_w[0];
  for (int i = 0; i < ring_leds - 1; i++)
  {
    ring_r[i] = ring_r[i+1];
    ring_g[i] = ring_g[i+1];
    ring_b[i] = ring_b[i+1];
    ring_w[i] = ring_w[i+1];
  }
  ring_r[ring_leds - 1] = temp_r;
  ring_g[ring_leds - 1] = temp_g;
  ring_b[ring_leds - 1] = temp_b;
  ring_w[ring_leds - 1] = temp_w;
  print_Ring();
}

void shift_Ring_CCW()
{
  char temp_r = ring_r[23];
  char temp_g = ring_g[23];
  char temp_b = ring_b[23];
  char temp_w = ring_w[23];
  for (int i = 23; i > 0; i--)
  {
    ring_r[i] = ring_r[i-1];
    ring_g[i] = ring_g[i-1];
    ring_b[i] = ring_b[i-1];
    ring_w[i] = ring_w[i-1];
  }
  ring_r[0] = temp_r;
  ring_g[0] = temp_g;
  ring_b[0] = temp_b;
  ring_w[0] = temp_w;
  print_Ring();
}

//game functions
void start_Game()
{
  fill_Buffer(LOW);
  print_Buffer();
  fill_Ring(30, 0, 0, 0);
  print_Ring();
  tone(buzzer_pin, low_buzz);
  delay(400);
  clear_Ring();
  print_Ring();
  noTone(buzzer_pin);
  delay(200);
  fill_Ring(20, 0, 0, 0);
  print_Ring();
  tone(buzzer_pin, low_buzz);
  delay(400);
  clear_Ring();
  print_Ring();
  noTone(buzzer_pin);
  delay(200);
  fill_Ring(10, 0, 0, 0);
  print_Ring();
  tone(buzzer_pin, low_buzz);
  delay(400);
  clear_Ring();
  noTone(buzzer_pin);
  delay(200);
  fill_Ring(0, 30, 0, 0);
  print_Ring();
  tone(buzzer_pin, high_buzz);
  delay(600);
  clear_Ring();
  print_Ring();
  noTone(buzzer_pin);
  delay(100);
}

void end_2p(int winner)
{
  fill_Buffer(LOW);
  print_Buffer();
  
  if (winner == 1) 
  {
    fill_Ring(30, 0, 0, 0);
    for (int i = 0; i < area / 2; i++) *red_leds[i] = HIGH;
  }
  else if (winner == 2) 
  {
    fill_Ring(0, 0, 30, 0);
    for (int i = 0; i < area / 2; i++) *blue_leds[i] = HIGH;
  }
  else 
  {
    fill_Ring(0, 30, 0, 0);
    fill_Buffer(HIGH);
  }
  print_Ring();
  print_Buffer();
  tone(buzzer_pin, high_buzz);
  delay(200);
  clear_Ring();
  print_Ring();
  fill_Buffer(LOW);
  print_Buffer();
  noTone(buzzer_pin);
  delay(100);
  if (winner == 1) 
  {
    fill_Ring(30, 0, 0, 0);
    for (int i = 0; i < area / 2; i++) *red_leds[i] = HIGH;
  }
  else if (winner == 2) 
  {
    fill_Ring(0, 0, 30, 0);
    for (int i = 0; i < area / 2; i++) *blue_leds[i] = HIGH;
  }
  else 
  {
    fill_Ring(0, 30, 0, 0);
    fill_Buffer(HIGH);
  }
  print_Ring();
  print_Buffer();
  tone(buzzer_pin, high_buzz);
  delay(200);
  clear_Ring();
  print_Ring();
  fill_Buffer(LOW);
  print_Buffer();
  noTone(buzzer_pin);
  delay(100);
  if (winner == 1) 
  {
    fill_Ring(30, 0, 0, 0);
    for (int i = 0; i < area / 2; i++) *red_leds[i] = HIGH;
  }
  else if (winner == 2) 
  {
    fill_Ring(0, 0, 30, 0);
    for (int i = 0; i < area / 2; i++) *blue_leds[i] = HIGH;
  }
  else 
  {
    fill_Ring(0, 30, 0, 0);
    fill_Buffer(HIGH);
  }
  print_Ring();
  print_Buffer();
  tone(buzzer_pin, high_buzz);
  delay(200);
  clear_Ring();
  print_Ring();
  fill_Buffer(LOW);
  print_Buffer();
  noTone(buzzer_pin);
}

void end_1p(unsigned int score)
{
  fill_Buffer(LOW);
  print_Buffer();
  fill_Ring(30, 0, 0, 0);
  print_Ring();
  tone(buzzer_pin, high_buzz);
  delay(200);
  clear_Ring();
  print_Ring();
  noTone(buzzer_pin);
  delay(100);
  fill_Ring(30, 0, 0, 0);
  print_Ring();
  tone(buzzer_pin, high_buzz);
  delay(200);
  clear_Ring();
  print_Ring();
  noTone(buzzer_pin);
  delay(100);
  fill_Ring(30, 0, 0, 0);
  print_Ring();
  tone(buzzer_pin, high_buzz);
  delay(200);
  clear_Ring();
  print_Ring();
  noTone(buzzer_pin);
  fill_Buffer(LOW);
  print_Buffer();
  clear_Ring();
  print_Ring();
  start_time = 0;
  current_time = millis();
  char r = 0;
  char g = 0;
  char b = 0;
  char w = 0;
      
  for (int i = 0; i < score;)
  {
    current_time = millis();
    if (current_time - start_time >= animation_lag)
    {
      start_time = millis();
      if (i % (3 * ring_leds) < ring_leds)
      {
        r = 0;
        g = 50;
        b = 0;
        w = 0;
      }
      else if (i % (3 * ring_leds) < 2 * ring_leds)
      {
        r = 0;
        g = 0;
        b = 50;
        w = 0;
      }
      else
      {
        r = 50;
        g = 0;
        b = 0;
        w = 0;
      }
      ring_r[i % ring_leds] = r;
      ring_g[i % ring_leds] = g;
      ring_b[i % ring_leds] = b;
      ring_w[i % ring_leds] = w;
      print_Ring();
      tone(buzzer_pin, buzz_tone[i % 8], 3 * animation_lag / 4);
      i++;
    }
  }
  delay(2000);
}

void single_Player()
{
  int lit_led_1 = rand() % area;
  int temp_lit_led = rand() % area;
  while (temp_lit_led == lit_led_1) temp_lit_led = rand() % area;
  int lit_led_2 = temp_lit_led;
  int score = 0;
  
  start_Game();
  fill_Ring(0, 30, 0, 0);
  print_Ring();
  
  led_buffer[lit_led_1] = HIGH;
  led_buffer[lit_led_2] = HIGH;
  print_Buffer();
  current_time = millis();
  start_time = current_time;
  last_ring_update = current_time;

  for(;;)
  {
    current_time = millis();
    if (current_time - start_time >= 24000) break;
    
    if (current_time - last_ring_update >= 1000)
    {
      unsigned long d_time = (current_time - start_time) / 1000;
      last_ring_update = current_time;
      if (d_time < 8)
      {
        for (int i = d_time; i < 24; i++) 
        {
          ring_r[i] = 0;
          ring_g[i] = 30;
          ring_b[i] = 0;
          ring_w[i] = 0;
        }
        for (int i = 0; i < d_time; i++) 
        {
          ring_r[i] = 0;
          ring_g[i] = 0;
          ring_b[i] = 0;
          ring_w[i] = 0;
        }
      }
      else if (d_time < 16)
      {
        for (int i = d_time; i < 24; i++) 
        {
          ring_r[i] = 20;
          ring_g[i] = 10;
          ring_b[i] = 0;
          ring_w[i] = 0;
        }
        for (int i = 0; i < d_time; i++) 
        {
          ring_r[i] = 0;
          ring_g[i] = 0;
          ring_b[i] = 0;
          ring_w[i] = 0;
        }
      }
      else
      {
        for (int i = d_time; i < 24; i++) 
        {
          ring_r[i] = 30;
          ring_g[i] = 0;
          ring_b[i] = 0;
          ring_w[i] = 0;
        }
        for (int i = 0; i < d_time; i++) 
        {
          ring_r[i] = 0;
          ring_g[i] = 0;
          ring_b[i] = 0;
          ring_w[i] = 0;
        }
      }
      print_Ring();
    }
    check_All_Buttons();
    if (curr_states[lit_led_1] == LOW)
    {
      tone(buzzer_pin, 784, 100);
      led_buffer[lit_led_1] = LOW;
      score++;
      int new_lit = rand() % area;
      while (new_lit == lit_led_1 || new_lit == lit_led_2) new_lit = rand() % area;
      lit_led_1 = new_lit;
      led_buffer[lit_led_1] = HIGH;
      print_Buffer();
    }
    if (curr_states[lit_led_2] == LOW)
    {
      tone(buzzer_pin, 784, 100);
      led_buffer[lit_led_2] = LOW;
      score++;
      int new_lit = rand() % area;
      while (new_lit == lit_led_1 || new_lit == lit_led_2) new_lit = rand() % area;
      lit_led_2 = new_lit;
      led_buffer[lit_led_2] = HIGH;
      print_Buffer();
    }
  }
  end_1p(score);
}

void two_Player()
{  
  start_Game();
  int timeout = 10000;
  unsigned long last_button_press = millis();
  int red_lit = rand() % (area / 2);
  *red_leds[red_lit] = HIGH;
  int blue_lit = rand() % (area / 2);
  *blue_leds[blue_lit] = HIGH;
  print_Buffer();
  for (int i = 0; i < ring_leds / 2; i++)
  {
    ring_r[i] = 30;
    ring_g[i] = 0;
    ring_b[i] = 0;
    ring_w[i] = 0;
  }
  for (int i = ring_leds / 2; i < ring_leds; i++)
  {
    ring_r[i] = 0;
    ring_g[i] = 0;
    ring_b[i] = 30;
    ring_w[i] = 0;
  }
  print_Ring();

  int red_score = 12;
  int blue_score = 12;
  current_time = millis();
  
  while (blue_score < 24 && red_score < 24)
  {
    current_time = millis();
    if (current_time - last_button_press > timeout) return;
    
    check_RB_Buttons();
    if (r_curr_states[red_lit] == LOW)
    {
      last_button_press = millis();
      *red_leds[red_lit] = LOW;
      tone(buzzer_pin, high_buzz, 100);
      ring_r[red_score] = 30;
      ring_g[red_score] = 0;
      ring_b[red_score] = 0;
      int new_lit = rand() % (area / 2);
      while (new_lit == red_lit) new_lit = rand() % (area / 2);
      red_lit = new_lit;
      *red_leds[red_lit] = HIGH;
      print_Buffer();
      print_Ring();
      red_score++;
      blue_score--;
    }
    if (b_curr_states[blue_lit] == LOW)
    {
      last_button_press = millis();
      *blue_leds[blue_lit] = LOW;
      tone(buzzer_pin, high_buzz, 100);
      ring_r[23 - blue_score] = 0;
      ring_g[23 - blue_score] = 0;
      ring_b[23 - blue_score] = 30;
      int new_lit = rand() % (area / 2);
      while (new_lit == blue_lit) new_lit = rand() % (area / 2);
      blue_lit = new_lit;
      *blue_leds[blue_lit] = HIGH;
      print_Buffer();
      print_Ring();
      blue_score++;
      red_score--;
    }
  }
  if (red_score >= 24) end_2p(1);
  else if (blue_score >= 24) end_2p(2);
}

int main_Menu()
{
  animation_state = 0;
  rainbow_Ring(50);
  print_Ring();
  int counter = 0;
  for (;;)
  {
    check_All_Buttons();
    if (curr_states[0] == LOW && curr_states[1] == LOW) return 1;
    else if (curr_states[2] == LOW && curr_states[3] == LOW) return 2;
    else if (curr_states[4] == LOW && curr_states[5] == LOW) return 3;
    else if (curr_states[6] == LOW && curr_states[7] == LOW) return 4;
    current_time = millis();
    if (current_time - start_time >= animation_lag)
    {
      start_time = millis();
      shift_Ring_CW();
      print_Ring();
      counter++;
      if ((counter - 4) % 8 == 0)
      {
        if (counter < 8 * 5)
        {
          led_buffer[0] = HIGH;
          led_buffer[1] = HIGH;
        }
        else
        {
          led_buffer[2] = HIGH;
          led_buffer[3] = HIGH;
        }
      }
      else if (counter % 8 == 0)
      {
        led_buffer[0] = LOW;
        led_buffer[1] = LOW;
        led_buffer[2] = LOW;
        led_buffer[3] = LOW;
      }
      
      if (counter % (8 * 10) == 0)
      {
        counter = 0;
      }
      print_Buffer();
    }
  }
}

//default functions
void setup() 
{
  pinMode(buzzer_pin, OUTPUT);
  ring.begin();
  clear_Ring();
  print_Ring();
  for (int i = 0; i < area; i++)
  {
    led_pins[i] = i + led_offset;
    pinMode(led_pins[i], OUTPUT);
    digitalWrite(led_pins[i], LOW);
    
    but_pins[i] = i + but_offset;
    pinMode(but_pins[i], INPUT);
    curr_states[i] = LOW;
  }
  buzz_tone[0] = 523;
  buzz_tone[1] = 587;
  buzz_tone[2] = 659;
  buzz_tone[3] = 698;
  buzz_tone[4] = 784;
  buzz_tone[5] = 880;
  buzz_tone[6] = 988;
  buzz_tone[7] = 1047;

  int blue_offset = 0;
  int red_offset = 0;

  for (int y = 0; y < rows; y++)
  {
    if (y % 2 == 0)
    {
      for (int x = 0; x < cols; x += 2)
      {
        blue_but_pins[blue_offset] = but_pins[y * cols + x];
        blue_leds[blue_offset] = &led_buffer[y * cols + x];
        red_but_pins[red_offset] = but_pins[y * cols + x + 1];
        red_leds[red_offset] = &led_buffer[y * cols + x + 1];
        blue_offset++;
        red_offset++;
      }
    }
    else
    {
      for (int x = 0; x < cols; x += 2)
      {
        red_but_pins[red_offset] = but_pins[y * cols + x];
        red_leds[red_offset] = &led_buffer[y * cols + x];
        blue_but_pins[blue_offset] = but_pins[y * cols + x + 1];
        blue_leds[blue_offset] = &led_buffer[y * cols + x + 1];
        blue_offset++;
        red_offset++;
      }
    }
  }
  for (int i = 0; i < area / 2; i++)
  {
    r_curr_states[i] = LOW;
    b_curr_states[i] = LOW;
  }
  
}

void loop() 
{
  clear_Ring();
  print_Ring();
  fill_Buffer(LOW);
  print_Buffer();
  
  if (game_state == 1) 
  {
    single_Player();
    game_state = 0;
  }
  else if (game_state == 2) 
  {
    two_Player();
    game_state = 0;
  }
  else game_state = main_Menu();
}
