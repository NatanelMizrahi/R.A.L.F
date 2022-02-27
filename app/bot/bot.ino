
#define ANARCHY_MODE 1
#define REMOTE_CONTROL_MODE 2

#define DIRECTION_UNSET 0b00
#define DIRECTION_LEFT  0b01
#define DIRECTION_RIGHT 0b10
#define DIRECTION_STRAIGHT (DIRECTION_LEFT | DIRECTION_RIGHT)

typedef enum {
    MOVE,
    STOP,
    SET_MODE
} command_type_t;

typedef enum {
    ANARCHY,
    REMOTE_CONTROL
} operation_mode_t;

typedef enum {
    LEFT,
    RIGHT,
    STRAIGHT
} direction_t;
    
typedef struct {
    unsigned char command_type;
    unsigned char op_code;
    short value;
} cmd_t;

int cmdSize = sizeof(cmd_t);
  
int avoidObstacleTurnDuration = 500;
int avoidObstacleDirection;
long minAllowedDistCm = 20;
int mode = REMOTE_CONTROL_MODE;
//int mode = ANARCHY_MODE;

int leftCtrlPin = 12;
int rightCtrlPin = 10; 

int sensorTrigPin = 9;
int sensorEchoPin = 8; 

int minMoveDuration = 700;
int maxMoveDuration = 2500;
int hasObstacle = 0;

long distCentimeters;
long durationUs;


void startMove(int _direction) {
  if(_direction & DIRECTION_LEFT) digitalWrite(leftCtrlPin, HIGH);
  if(_direction & DIRECTION_RIGHT) digitalWrite(rightCtrlPin, HIGH);
}

void stopMove(int _direction) {
//  if(_direction & DIRECTION_LEFT) digitalWrite(leftCtrlPin, LOW);
//  if(_direction & DIRECTION_RIGHT) digitalWrite(rightCtrlPin, LOW);
  digitalWrite(leftCtrlPin, LOW);
  digitalWrite(rightCtrlPin, LOW);

}

void move(int _direction, int duration) {
  startMove(_direction);
  delay(duration);
  stopMove(_direction);
}

void moveRandomly(){
  if (hasObstacle) return;
  int _direction = random(DIRECTION_LEFT, DIRECTION_STRAIGHT+1);
  int _duration = random(minMoveDuration, maxMoveDuration);
  move(_direction, _duration);
}

void avoidObstacle(){
  hasObstacle = 1;
//  move(avoidObstacleDirection, avoidObstacleTurnDuration);
}

void resetObstacleState () {
  hasObstacle = 0;
  avoidObstacleDirection = random(2) ? DIRECTION_LEFT : DIRECTION_RIGHT;
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
        case MOVE:
             stopMove(0);
             delayMicroseconds(5);
             startMove(command.op_code);
             break;
        case STOP:
            stopMove(command.op_code);
            break;
        case SET_MODE:
            mode = command.op_code;
            break;
    }
  }
}

void setup() {
  pinMode(leftCtrlPin, OUTPUT);
  pinMode(rightCtrlPin, OUTPUT);
  
  pinMode(sensorTrigPin, OUTPUT);
  pinMode(sensorEchoPin, INPUT);
  Serial.begin(9600);

}

// the loop function runs over and over again forever
void loop() {
  
  if (mode == ANARCHY_MODE) {
    checkAndAvoidObstacles();
    moveRandomly();
  }
  else {
    readCommand();
  }
}
