#include <Tone.h>

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

Tone notePlayer[2];

//int notes[] = { NOTE_A3,
//                NOTE_B3,
//                NOTE_C4,
//                NOTE_D4,
//                NOTE_E4,
//                NOTE_F4,
//                NOTE_G4 };


int notes[] = {
 15, 25, 31,33,35,37,39,41,44,46,49,52,55,58,62,65,69,73,78,82,87,93,98,104,110,117,123,131,139,147,156,165,175,185,196,208,220,233,247,262,277,294,311,330,349,370,392,415,440,466,494,523,554,587,622,659,698,740,784,831,880,932,988,1047,1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,1976,2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951,4186,4435,4699,4978
};

#define N_NOTES 70

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

void init_direction_pins(){
    bit_vec_t LEFT_BACKWARD_CTRL_PINS = (CTRL_PIN_LEFT_C0 | CTRL_PIN_LEFT_C1);
    bit_vec_t RIGHT_BACKWARD_CTRL_PINS = (CTRL_PIN_RIGHT_C0 | CTRL_PIN_RIGHT_C1);
    bit_vec_t LEFT_FORWARD_CTRL_PINS = (CTRL_PIN_LEFT_C2 | CTRL_PIN_LEFT_C3);
    bit_vec_t RIGHT_FORWARD_CTRL_PINS = (CTRL_PIN_RIGHT_C2 | CTRL_PIN_RIGHT_C3);
    
    DIRECTION_FORWARD_CTRL_PINS = LEFT_FORWARD_CTRL_PINS | RIGHT_FORWARD_CTRL_PINS;
    DIRECTION_BACK_CTRL_PINS = LEFT_BACKWARD_CTRL_PINS | RIGHT_BACKWARD_CTRL_PINS;
    DIRECTION_LEFT_CTRL_PINS = LEFT_BACKWARD_CTRL_PINS | RIGHT_FORWARD_CTRL_PINS;
    DIRECTION_RIGHT_CTRL_PINS = LEFT_FORWARD_CTRL_PINS | RIGHT_BACKWARD_CTRL_PINS;

    DIRECTION_CTRL_PINS = DIRECTION_FORWARD_CTRL_PINS | DIRECTION_BACK_CTRL_PINS | DIRECTION_LEFT_CTRL_PINS | DIRECTION_RIGHT_CTRL_PINS;

    direction_to_bit_vector[FORWARD] = DIRECTION_FORWARD_CTRL_PINS;
    direction_to_bit_vector[BACK] = DIRECTION_BACK_CTRL_PINS;
    direction_to_bit_vector[LEFT] = DIRECTION_LEFT_CTRL_PINS;
    direction_to_bit_vector[RIGHT] = DIRECTION_RIGHT_CTRL_PINS;

    for (int pin = CTRL_PIN_MIN_INDEX; pin <= CTRL_PIN_MAX_INDEX; pin++) {
        if ((1 << pin) & DIRECTION_CTRL_PINS) {
            pinMode(pin, OUTPUT);
        }
    }
}

void write_direction_pins(bit_vec_t ctrl_pin_bit_vector, int val){
    for (int pin = CTRL_PIN_MIN_INDEX; pin <= CTRL_PIN_MAX_INDEX; pin++) {
        if ((1 << pin) & ctrl_pin_bit_vector) {
            digitalWrite(pin, val);
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
  delay(duration);
  stopMove();
}

void moveRandomly(){
  if (hasObstacle) return;
  int _direction = random(FORWARD, BACK+1);
  int _duration = random(minMoveDuration, maxMoveDuration);
  move(_direction, _duration);
}

void avoidObstacle(){
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
  distCentimeters = (durationUs/2) * 0.0343;
  if (distCentimeters < minAllowedDistCm){
    avoidObstacle();
  }
  else {
    resetObstacleState();
  }
}

void readCommand() {
  if (Serial.available() > 0) {
    cmd_t command;
//    char buffer[40];
    Serial.readBytes((char*)&command, cmdSize);
//    sprintf(buffer, "[buffer: %x][command_type: %d][op_code: %d][value: %d]", (*(int*)&command), command.command_type, command.op_code, command.value);
//    Serial.println(buffer);
    switch (command.command_type) {
        case SET_MODE:
            mode = command.op_code;
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

void setAlarm(short alarmDeltaMinutes){
  alarmTimeMillis = millis() + (unsigned long)(alarmDeltaMinutes) * 60 * 1000;
  alarmSet = 1;
}
void disableAlarm(){
  alarmSet = 0;
}
void activateAlarm(){
  alarmSet = 0;
  mode = ANARCHY_MODE;
}

void checkAlarm(){
    unsigned long currTimeMillis = millis();
    if (currTimeMillis > alarmTimeMillis) {
      activateAlarm();
    }
    delay(2000);
}


void setup() {
  init_direction_pins();

  pinMode(sensorTrigPin, OUTPUT);
  pinMode(sensorEchoPin, INPUT);
  pinMode(speakerPin, OUTPUT);

  notePlayer[0].begin(13);
  notePlayer[1].begin(A1);
  Serial.begin(9600);
}

int i = 0;
int randNote;
int randIter;

void playNightmareFuel()
{
  int randNote = random(N_NOTES);
  notePlayer[0].play(notes[random(N_NOTES)]);
  randIter = random(10) * 2;
  while (--randIter > 0) {
      notePlayer[1].play(notes[random(N_NOTES)]);    
      delay(200); 
  }
}

void raiseHell(){
  checkAndAvoidObstacles();
  int _direction = random(FORWARD, BACK+1);
  stopMove();
  startMove(_direction);
  playNightmareFuel();
}

void loop() {
  if (alarmSet){
    checkAlarm();
  }
  if (mode == ANARCHY_MODE) {
    raiseHell();
  }
  readCommand();
}
