
#include "theme.h"

Playtune p;

int tone_chan1_pin = A1;
int tone_chan2_pin = A2;
int tone_chan3_pin = A3;

typedef enum {
  MOVE,
  SET_MODE,
  SET_ALARM,
  DISABLE_ALARM
} command_type_t;

typedef struct {
  unsigned char command_type;
  unsigned char op_code;
  short value;
} cmd_t;

int cmdSize = sizeof(cmd_t);

typedef enum {
  ANARCHY_MODE,
  REMOTE_CONTROL_MODE
} operation_mode_t;

typedef enum {
  FORWARD = 0,
  LEFT = 1,
  RIGHT = 2,
  REVERSE = 3,
  STOP = 4
} direction_t;

int direction_control_pin_values[5][4] = {
// A1 A2 B1 B2
  {1, 0, 1, 0}, // forward
  {1, 0, 0, 1}, // left
  {0, 1, 1, 0}, // right
  {0, 1, 0, 1}, // reverse
  {0, 0, 0, 0}  // stop
};

int speakerPin = 13;

int sensorTrigPin = 8;
int sensorEchoPin = 7;

int minMoveDuration = 700;
int maxMoveDuration = 2500;
int hasObstacle = 0;

long distCentimeters;
long durationUs;

int avoidObstacleTurnDuration = 700;
int avoidObstacleDirection;
long minAllowedDistCm = 20;

int mode = REMOTE_CONTROL_MODE;
//int mode = ANARCHY_MODE;

int alarmSet = 0;
int ignoreCommands = 0;

unsigned long alarmTimeMillis;

typedef int bit_vec_t;

#define MOTOR_CTRL_PIN_OFFSET 3
#define NUM_MOTOR_CTRL_PINS 4

void init_direction_pins() {
  for (int pin = MOTOR_CTRL_PIN_OFFSET; pin < MOTOR_CTRL_PIN_OFFSET + NUM_MOTOR_CTRL_PINS; pin++)
    pinMode(pin, OUTPUT);
}

void startMove(direction_t _direction) {
  int* direction_controls_values = direction_control_pin_values[_direction];
  for(int pin = 0; pin < NUM_MOTOR_CTRL_PINS; pin++) 
    digitalWrite(MOTOR_CTRL_PIN_OFFSET + pin, direction_controls_values[pin]);
}

void stopMove() {
  startMove(STOP);
}

void move(int _direction, int duration) {
  startMove(_direction);
  smartDelay(duration);
  stopMove();
}

void moveRandomly() {
  if (hasObstacle) return;
  int _direction = random(FORWARD, REVERSE + 1);
  int _duration = random(minMoveDuration, maxMoveDuration);
  move(_direction, _duration);
}

void avoidObstacle() {
  hasObstacle = 1;
  move(avoidObstacleDirection, avoidObstacleTurnDuration);
}

void resetObstacleState () {
  hasObstacle = 0;
  avoidObstacleDirection = random(2) ? LEFT : RIGHT;
}

void checkAndAvoidObstacles() {
  // The ultrasonic sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(sensorTrigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(sensorTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(sensorTrigPin, LOW);

  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(sensorEchoPin, INPUT);
  durationUs = pulseIn(sensorEchoPin, HIGH);

  // Convert the time into a distance
  distCentimeters = (durationUs / 2) * 0.0343;
  if (distCentimeters < minAllowedDistCm) {
    avoidObstacle();
  }
  else {
    resetObstacleState();
  }
}

void smartDelay(int ms) {
  if (mode == ANARCHY_MODE) {
    p.tune_delay(ms);
  } else {
    delay(ms);
  }
}

void set_mode(operation_mode_t _mode) {
  stopMove();
  if (_mode == ANARCHY_MODE) {
    init_tone_channels();
  } else {
    p.tune_stopscore();
    p.tune_stopchans();
  }
  mode = _mode;
}

void readCommand() {
  if (Serial.available() > 0 && !ignoreCommands) {
    cmd_t command;
    Serial.readBytes((char*)&command, cmdSize);
//    char buffer[40];
//    sprintf(buffer, "[buffer: %x][command_type: %d][op_code: %d][value: %d]", (*(int*)&command), command.command_type, command.op_code, command.value);
//    Serial.println(buffer);
    switch (command.command_type) {
      case SET_MODE:
        set_mode(command.op_code);
        break;
      case MOVE:
        if (mode == ANARCHY_MODE)
          return;
        stopMove();
        delayMicroseconds(5);
        startMove(command.op_code);
        break;
      case SET_ALARM:
        setAlarm(command.value);
        break;
      case DISABLE_ALARM:
        disableAlarm();
        break;
    }
  }
}

void setAlarm(short alarmDeltaMinutes) {
  alarmTimeMillis = millis() + (unsigned long)(alarmDeltaMinutes) * 60 * 1000;
  alarmSet = 1;
  char buffer[40];
  sprintf(buffer, "[alarmDeltaMinutes: %d][alarmTimeMillis: %lu]", alarmDeltaMinutes, alarmTimeMillis);
  Serial.println(buffer);
}

void disableAlarm() {
  alarmSet = 0;
}

void activateAlarm() {
  alarmSet = 0;
  ignoreCommands = 1;
  set_mode(ANARCHY_MODE);
}

void checkAlarm() {
  if (!alarmSet) {
    return;
  }
  unsigned long currTimeMillis = millis();
  if (currTimeMillis > alarmTimeMillis) {
    activateAlarm();
  }
  smartDelay(2000);
}

void init_tone_channels() {
  p.tune_initchan(tone_chan1_pin);
  p.tune_initchan(tone_chan2_pin);
  p.tune_initchan(tone_chan3_pin);
}


void setup() {
  init_direction_pins();
  // init_tone_channels();

  pinMode(sensorTrigPin, OUTPUT);
  pinMode(sensorEchoPin, INPUT);
  Serial.begin(9600);
}

void playTheme() {
  if (!p.tune_playing) {
    p.tune_playscore(getRandomTheme());
  }
}

void raiseHell() {
  checkAndAvoidObstacles();
  moveRandomly();
  playTheme();
}

void loop() {
  checkAlarm();
  if (mode == ANARCHY_MODE) {
    raiseHell();
  }    
  readCommand();
}
