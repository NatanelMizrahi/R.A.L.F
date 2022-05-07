
#include "theme.h"

Playtune p;

int tone_chan1_pin = 13;
int tone_chan2_pin = A1;
int tone_chan3_pin = A3;

typedef enum {
  MOVE,
  STOP,
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
  BACK = 3,
} direction_t;


int speakerPin = 13;

int sensorTrigPin = 12;
int sensorEchoPin = 11;

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

/*
  // Each motor (left/right) has the following control sub-circuit
  Ci represents an NPN junction where the base is connected to the corresponding pin.

  -------- Vcc(+) ---------
  |                       |
  C2                      C0
  |                       |
  ---------(Motor)--------|
  |                       |
  C1                      C3
  |                       |
  -------- Gnd(-) ---------

  Therefore:
  C0 + C1 -> Motor rotates clockwise
  C2 + C3 -> Motor rotates counter-clockwise
*/

typedef int bit_vec_t;

#define CTRL_PIN_OFFSET_LEFT_MOTOR 7
#define CTRL_PIN_OFFSET_RIGHT_MOTOR 3
#define CTRL_PIN_MAX_INDEX 10
#define CTRL_PIN_MIN_INDEX 3

bit_vec_t CTRL_PIN_LEFT_C0 = 1 << (CTRL_PIN_OFFSET_LEFT_MOTOR + 0);
bit_vec_t CTRL_PIN_LEFT_C1 = 1 << (CTRL_PIN_OFFSET_LEFT_MOTOR + 1);
bit_vec_t CTRL_PIN_LEFT_C2 = 1 << (CTRL_PIN_OFFSET_LEFT_MOTOR + 2);
bit_vec_t CTRL_PIN_LEFT_C3 = 1 << (CTRL_PIN_OFFSET_LEFT_MOTOR + 3);

bit_vec_t CTRL_PIN_RIGHT_C0 = 1 << (CTRL_PIN_OFFSET_RIGHT_MOTOR + 0);
bit_vec_t CTRL_PIN_RIGHT_C1 = 1 << (CTRL_PIN_OFFSET_RIGHT_MOTOR + 1);
bit_vec_t CTRL_PIN_RIGHT_C2 = 1 << (CTRL_PIN_OFFSET_RIGHT_MOTOR + 2);
bit_vec_t CTRL_PIN_RIGHT_C3 = 1 << (CTRL_PIN_OFFSET_RIGHT_MOTOR + 3);

// initialized in init_direction_pins
bit_vec_t DIRECTION_CTRL_PINS;
bit_vec_t   DIRECTION_FORWARD_CTRL_PINS,
            DIRECTION_BACK_CTRL_PINS,
            DIRECTION_LEFT_CTRL_PINS,
            DIRECTION_RIGHT_CTRL_PINS;

bit_vec_t direction_to_bit_vector[4];

// bit vector representing the ctrl pin indexes for bases of PNP junctions (C0, C2 above)
bit_vec_t NORMALLY_HIGH_DIRECTION_CTRL_PINS_MASK;

void init_direction_pins() {
  bit_vec_t LEFT_BACKWARD_CTRL_PINS = (CTRL_PIN_LEFT_C0 | CTRL_PIN_LEFT_C1);
  bit_vec_t RIGHT_BACKWARD_CTRL_PINS = (CTRL_PIN_RIGHT_C0 | CTRL_PIN_RIGHT_C1);
  bit_vec_t LEFT_FORWARD_CTRL_PINS = (CTRL_PIN_LEFT_C2 | CTRL_PIN_LEFT_C3);
  bit_vec_t RIGHT_FORWARD_CTRL_PINS = (CTRL_PIN_RIGHT_C2 | CTRL_PIN_RIGHT_C3);

  DIRECTION_FORWARD_CTRL_PINS = LEFT_FORWARD_CTRL_PINS | RIGHT_FORWARD_CTRL_PINS;
  DIRECTION_BACK_CTRL_PINS = LEFT_BACKWARD_CTRL_PINS | RIGHT_BACKWARD_CTRL_PINS;
  DIRECTION_LEFT_CTRL_PINS = LEFT_BACKWARD_CTRL_PINS | RIGHT_FORWARD_CTRL_PINS;
  DIRECTION_RIGHT_CTRL_PINS = LEFT_FORWARD_CTRL_PINS | RIGHT_BACKWARD_CTRL_PINS;

  DIRECTION_CTRL_PINS = DIRECTION_FORWARD_CTRL_PINS | DIRECTION_BACK_CTRL_PINS | DIRECTION_LEFT_CTRL_PINS | DIRECTION_RIGHT_CTRL_PINS;

  NORMALLY_HIGH_DIRECTION_CTRL_PINS_MASK = CTRL_PIN_LEFT_C0 | CTRL_PIN_RIGHT_C0 | CTRL_PIN_LEFT_C2 | CTRL_PIN_RIGHT_C2;

  direction_to_bit_vector[FORWARD] = DIRECTION_FORWARD_CTRL_PINS;
  direction_to_bit_vector[BACK] = DIRECTION_BACK_CTRL_PINS;
  direction_to_bit_vector[LEFT] = DIRECTION_LEFT_CTRL_PINS;
  direction_to_bit_vector[RIGHT] = DIRECTION_RIGHT_CTRL_PINS;

  for (int pin = CTRL_PIN_MIN_INDEX; pin <= CTRL_PIN_MAX_INDEX; pin++) {
    if ((1 << pin) & DIRECTION_CTRL_PINS) {
      pinMode(pin, OUTPUT);
    }
  }
  write_direction_pins(DIRECTION_CTRL_PINS, LOW);
}

void write_direction_pins(bit_vec_t ctrl_pin_bit_vector, int val) {
  int pin_val;
  for (int pin = CTRL_PIN_MIN_INDEX; pin <= CTRL_PIN_MAX_INDEX; pin++) {
    if ((1 << pin) & ctrl_pin_bit_vector) {
      pin_val = ((NORMALLY_HIGH_DIRECTION_CTRL_PINS_MASK >> pin) ^ val) & HIGH; // flip bit for normally high ctrl pins
      digitalWrite(pin, pin_val);
    }
  }
}


void startMove(int _direction) {
  write_direction_pins(direction_to_bit_vector[_direction], HIGH);
}

void stopMove() {
  write_direction_pins(DIRECTION_CTRL_PINS, LOW);
}

void move(int _direction, int duration) {
  startMove(_direction);
  smartDelay(duration);
  stopMove();
}

void moveRandomly() {
  if (hasObstacle) return;
  int _direction = random(FORWARD, BACK + 1);
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
      case STOP:
        if (mode == ANARCHY_MODE)
          return;
        stopMove();
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
  pinMode(speakerPin, OUTPUT);

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
