#define btdwn 2
#define btset 3

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <display_SW.h>
#include <dht.h>

#define NT 20


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define PIN_WIRE_SDA  7
#define PIN_WIRE_SCL  8

dht DHT;

#define DHT11_PIN A0

//// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint32_t Tcounter = 53160;
int mins = 0, hrs = 0;
bool prev = 0;
bool stwt = 0;
uint32_t Tstop = 0;
int stemp = 0;
int nani = 0;

enum page_e{
  tclock,
  pstopwatch,
  ptemps,
};

enum page_e page = tclock;


/* 4 Tasks:
 *     T1 - T = 100ms   -> Led d1 toggle
 *     T2 - T = 200ms   -> Led d2 toggle
 *     T3 - T = 600ms   -> Led d3 toggle
 *     T4 - T = 100ms   -> Led d4 copied from button A1
 */

void temp_meas(){
  int chk = DHT.read11(DHT11_PIN);
  stemp = DHT.temperature;
  //Serial.println(stemp);
}

void time_counter(){
  Tcounter++;
  //Serial.println("Added");
}

void stop_counter(){
  if(stwt){
    Tstop++;
  }
  //Serial.println(Tstop);
}

void SWtemps() {
    display.clearDisplay();

    // Display count animation image (assuming heartAnimation is defined elsewhere in your code)
    nani++;
    if(nani == 2){
      nani = 0;
    }
    display.drawBitmap(0, 0, temp[nani], 128, 64, WHITE);


    display.setTextSize(2); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(55, 28); // Adjust this cursor to place text correctly

    // Print temp value
    display.println(stemp);

    // Print ºC
    display.drawCircle(86, 30, 2, SSD1306_WHITE); // Adjust cursor position for "º" and Draw
    display.setCursor(92, 28);
    display.print("C");

    display.display();
}

void SWclock() {
    display.clearDisplay();
    
    int secs;

    secs = Tcounter % 60;
    mins = (Tcounter / 60) % 60;
    hrs = Tcounter / 3600 % 24;
    
    // Display the clock
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 28);
    /*display.println(hrs);  //HOURS (CHANGE HERE)
    display.setCursor(60, 28);
    display.println(F(":"));
    display.setCursor(80, 28); //MINUTES (CHANGE HERE)
    display.println(mins);*/
    String stime;
    //stime = String(hrs) + " : " + String(mins) + " : " + String(secs);
    stime = String(hrs) + ":" + (mins < 10 ? "0" : "") + String(mins) + ":" + (secs < 10 ? "0" : "") + String(secs);
    display.println(stime);
    
    display.display();
}

void SWstopwatch() {
    display.clearDisplay();
    
    // Display count animation image (assuming heartAnimation is defined elsewhere in your code)
    display.drawBitmap(0, 0, stopwatch, 128, 64, WHITE);

        // Display heart rate "90" and "BPM" text
    display.setTextSize(2); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(55, 28);

    int hrs2, mins2, secs;

    secs = Tstop % 60;
    mins2 = (Tstop / 60) % 60;
    /*display.println(F("00"));
    display.setCursor(68, 28);
    display.println(F(":"));
    display.setCursor(76, 28);
    display.println(F("72"));
    display.setCursor(96, 28);
    display.println(F(":"));
    display.setCursor(104, 28);
    display.println(F("04"));*/
    String stime;
    //stime = String(hrs) + " : " + String(mins) + " : " + String(secs);
    stime = (mins2 < 10 ? "0" : "") + String(mins2) + ":" + (secs < 10 ? "0" : "") + String(secs);
    display.println(stime);
    
    display.display();
}


void drawcall() {
  switch (page){
    case tclock:
      SWclock();
      break;
    
    case pstopwatch:
      SWstopwatch();
      break;

    case ptemps:
      SWtemps();
      break;

  }

  //Serial.println("I drew");
}


typedef struct {
  /* period in ticks */
  int period;
  /* ticks until next activation */
  int delay;
  /* function pointer */
  void (*func)(void);
  /* activation counter */
  int exec;
} Sched_Task_t;


Sched_Task_t Tasks[NT];


int Sched_Init(void){
  for(int x=0; x<NT; x++)
    Tasks[x].func = 0;
  /* Also configures interrupt that periodically calls Sched_Schedule(). */
  noInterrupts(); // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
 
//OCR1A = 6250; // compare match register 16MHz/256/10Hz
//OCR1A = 31250; // compare match register 16MHz/256/2Hz
  OCR1A = 31;    // compare match register 16MHz/256/2kHz
  TCCR1B |= (1 << WGM12); // CTC mode
  TCCR1B |= (1 << CS12); // 256 prescaler
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  interrupts(); // enable all interrupts  
}

int Sched_AddT(void (*f)(void), int d, int p){
  for(int x=0; x<NT; x++)
    if (!Tasks[x].func) {
      Tasks[x].period = p;
      Tasks[x].delay = d;
      Tasks[x].exec = 0;
      Tasks[x].func = f;
      return x;
    }
  return -1;
}


void Sched_Schedule(void){
  for(int x=0; x<NT; x++) {
    if(Tasks[x].func){
      if(Tasks[x].delay){
        Tasks[x].delay--;
      } else {
        /* Schedule Task */
        Tasks[x].exec++;
        Tasks[x].delay = Tasks[x].period-1;
      }
    }
  }
}


void Sched_Dispatch(void){
  for(int x=0; x<NT; x++) {
    if((Tasks[x].func)&& (Tasks[x].exec)) {
      Tasks[x].exec=0;
      Tasks[x].func();

      /* Delete task if one-shot */
      if(!Tasks[x].period) Tasks[x].func = 0;
      return; // fixed priority version!
    }
  }
}

void bts() {
  if(digitalRead(btdwn)){
    if(!prev){
      //Serial.println("BTdwn Pressed");
      page = (page + 1) % 3;
      prev = 1;
    }
  } else if(digitalRead(btset)){
    if(!prev){
      if(page == pstopwatch){
        //Serial.println("BTset Pressed");
        stwt = !stwt;
        if(stwt){
          Tstop = 0;
        }
      }
      prev = 1;
    }
  } else{
    prev = 0;
  }
}


// the setup function runs once when you press reset or power the board
void setup() {

  Serial.begin(9600);

  //initialize display
  //  ------------------- For i2c -------------------
//  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    //Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  //Serial.println(F("Initialized!"));
 
  // Show initial display buffer contents on the screen --
  display.clearDisplay();
  display.drawBitmap(0, 0, logoxinasung, 128, 64, WHITE);
  display.display();
  delay(2000);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(btdwn, INPUT);
  pinMode(btset, INPUT);
  Sched_Init();

  Sched_AddT(bts, 1, 100);
  Sched_AddT(drawcall, 1, 300);
  Sched_AddT(temp_meas, 5, 10000);
  Sched_AddT(time_counter, 1, 2000);
  Sched_AddT(stop_counter, 2, 2000);
  
  
}



ISR(TIMER1_COMPA_vect){//timer1 interrupt
  Sched_Schedule();
}




// the loop function runs over and over again forever
void loop() {
  Sched_Dispatch();
}
