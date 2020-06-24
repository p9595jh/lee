#include <Arduino_FreeRTOS.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <AccelStepper.h>

#define MODE_SET_SIZE        0
#define MODE_STRETCHING      1
#define MODE_TORSION         2
#define MODE_U_SHAPE_BENDING 3
#define MODE_FOLDING         4
#define MODE_RESTORE         5

#define WRITING_FALSE 0
#define WRITING_TRUE  1
#define WRITING_LAST  2

#define CW  0
#define CCW 1

#define STEP_ANGLE  1.8
#define STEP_PIN    4

#define SERVO_MIN  150  // 서보모터 최소 출력 값 = 0도
#define SERVO_HALF 375  // 90도
#define SERVO_MAX  600  // 서보모터 최대 출력 값 = 180도

#define M1 0
#define M2 14

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// 리니어 모터
AccelStepper linearMotor(STEP_PIN, 9, 8, 7, 6);
// 토션용 모터
AccelStepper tortionMotor(STEP_PIN, 5, 4, 3, 2);
// 서포팅 플레이트 이동 모터
AccelStepper plateMotor(STEP_PIN, 13, 12, 11, 10);

// sample size (mm)
int sz = 0;

float log_value = 0;
long standard_millis = 0;

bool servo_running = false;
long servo_msec = 0;

/**
 * 0: FALSE
 * 1: TRUE
 * 2: TRUE but this loop is the last and will be changed to FALSE
 */
unsigned short writingType = 0;

void setup() {
    Serial.begin(9600);

    pwm.begin();
    pwm.setPWMFreq(60);

    linearMotor.setMaxSpeed(20000);
    linearMotor.setSpeed(200);
    linearMotor.setCurrentPosition(0);

    tortionMotor.setMaxSpeed(1000);
    tortionMotor.setSpeed(200);
    tortionMotor.setCurrentPosition(0);

    plateMotor.setMaxSpeed(1000);
    plateMotor.setSpeed(200);
    plateMotor.setCurrentPosition(0);

    delay(20);

    xTaskCreate(_main, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(servoMotor, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(logExec, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();
    while(1);
}

static void _main(void *arg) {
    while (1) {
        if ( Serial.available() ) {
            String s = Serial.readStringUntil('\n');
            short type = s.substring(0, 1).toInt();

            // set size
            if ( type == MODE_SET_SIZE ) {
                // sz is size of the length, as 'mm'
                sz = s.substring(1, s.indexOf('|')).toInt();
                linearMotor.moveTo(sz * 45);
                linearMotor.setSpeed(200);
                while (linearMotor.currentPosition() != linearMotor.targetPosition()) {
                    linearMotor.runSpeedToPosition();
                }
                linearMotor.setCurrentPosition(0);

            // stretching
            } else if ( type == MODE_STRETCHING ) {
                const int len = s.substring(1, s.indexOf('|')).toInt();
                const int duration = s.substring(s.indexOf('|') + 1, s.lastIndexOf('|')).toInt();
                const int numberOfTest = s.substring(s.lastIndexOf('|') + 1).toInt();

                writingType = WRITING_TRUE;
                standard_millis = millis();
                for (int i=0; i<numberOfTest; i++) {
                    const long motorPosition = linearMotor.currentPosition();
                    const float spd = (float) len * 45 / (float) duration;
                    linearMotor.moveTo(len * 45);
                    linearMotor.setSpeed(spd);
                    while (linearMotor.currentPosition() != linearMotor.targetPosition()) {
                        linearMotor.runSpeedToPosition();
                        log_value = (float) linearMotor.currentPosition() / 45.0;
                    }
                    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);
                    linearMotor.moveTo(motorPosition);
                    linearMotor.setSpeed(spd);
                    while (linearMotor.currentPosition() != linearMotor.targetPosition()) {
                        linearMotor.runSpeedToPosition();
                        log_value = (float) linearMotor.currentPosition() / 45.0;
                    }
                    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);
                }
                writingType = WRITING_LAST;

            // torsion
            } else if ( type == MODE_TORSION ) {
                const int angle = s.substring(1, s.indexOf('|')).toInt();
                const int duration = s.substring(s.indexOf('|') + 1, s.lastIndexOf('|')).toInt();
                const int numberOfTest = s.substring(s.lastIndexOf('|') + 1).toInt();
                const float spd = (647.0 * (float) angle) / (252.0 * (float) duration);

                plateMotor.moveTo(273);
                plateMotor.setSpeed(200);
                while (plateMotor.currentPosition() != plateMotor.targetPosition()) {
                    plateMotor.runSpeedToPosition();
                }
                vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);

                writingType = WRITING_TRUE;
                standard_millis = millis();
                for (int i=0; i<numberOfTest; i++) {
                    tortionMotor.moveTo((long) (647.0 * (float) angle / 252.0));
                    tortionMotor.setSpeed(spd);
                    while (tortionMotor.currentPosition() != tortionMotor.targetPosition()) {
                        tortionMotor.runSpeedToPosition();
                        log_value = 252.0 * (float) tortionMotor.currentPosition() / 647.0;
                    }
                    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);
                    tortionMotor.moveTo(0);
                    tortionMotor.setSpeed(spd);
                    while (tortionMotor.currentPosition() != tortionMotor.targetPosition()) {
                        tortionMotor.runSpeedToPosition();
                        log_value = 252.0 * (float) tortionMotor.currentPosition() / 647.0;
                    }
                    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);
                }
                writingType = WRITING_LAST;

                plateMotor.moveTo(0);
                plateMotor.setSpeed(200);
                while (plateMotor.currentPosition() != plateMotor.targetPosition()) {
                    plateMotor.runSpeedToPosition();
                }

            // u-shape bending
            } else if ( type == MODE_U_SHAPE_BENDING ) {
                const int len = s.substring(1, s.indexOf('|')).toInt();
                const int duration = s.substring(s.indexOf('|') + 1, s.lastIndexOf('|')).toInt();
                const int numberOfTest = s.substring(s.lastIndexOf('|') + 1).toInt();
                const float spd = (float) (sz - len) * 45 / (float) duration;

                writingType = WRITING_TRUE;
                standard_millis = millis();

                servo_msec = (long) ((float) duration * 11.11);
                servo_running = true;
                
                for (int i=0; i<numberOfTest; i++) {
                    const long motorPosition = linearMotor.currentPosition();
                    linearMotor.moveTo((sz - len) * 45);
                    linearMotor.setSpeed(spd);
                    while (linearMotor.currentPosition() != linearMotor.targetPosition()) {
                        linearMotor.runSpeedToPosition();
                        log_value = (float) linearMotor.currentPosition() / 45.0;
                    }
                    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);
                    linearMotor.moveTo(motorPosition);
                    linearMotor.setSpeed(spd);
                    while (linearMotor.currentPosition() != linearMotor.targetPosition()) {
                        linearMotor.runSpeedToPosition();
                        log_value = (float) linearMotor.currentPosition() / 45.0;
                    }
                    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);
                }
                servo_running = false;
                writingType = WRITING_LAST;

            // folding
            } else if ( type == MODE_FOLDING ) {
                const int duration = s.substring(1, s.indexOf('|')).toInt();
                const int numberOfTest = s.substring(s.indexOf('|') + 1).toInt();
                const float spd = (float) sz * 45 / (float) duration;

                writingType = WRITING_TRUE;
                standard_millis = millis();

                servo_msec = (long) ((float) duration * 11.11);
                servo_running = true;
                
                for (int i=0; i<numberOfTest; i++) {
                    const long motorPosition = linearMotor.currentPosition();
                    linearMotor.moveTo(sz * 45);
                    linearMotor.setSpeed(spd);
                    while (linearMotor.currentPosition() != linearMotor.targetPosition()) {
                        linearMotor.runSpeedToPosition();
                        log_value = (float) linearMotor.currentPosition() / 45.0;
                    }
                    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);
                    linearMotor.moveTo(motorPosition);
                    linearMotor.setSpeed(spd);
                    while (linearMotor.currentPosition() != linearMotor.targetPosition()) {
                        linearMotor.runSpeedToPosition();
                        log_value = (float) linearMotor.currentPosition() / 45.0;
                    }
                    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);
                }
                servo_running = false;
                writingType = WRITING_LAST;

            // restore the linear motor position
            } else if ( type == MODE_RESTORE ) {
                linearMotor.moveTo(-sz * 45);
                linearMotor.setSpeed(200);
                while (linearMotor.currentPosition() != linearMotor.targetPosition()) {
                    linearMotor.runSpeedToPosition();
                }
                linearMotor.setCurrentPosition(0);
              
            }
        }
    }
}

static void servoMotor(void *arg) {
    while (1) {
        if ( servo_running ) {
            for (int i=SERVO_MIN; i<=SERVO_HALF; i++) {
                pwm.setPWM(M1, 0, i);
                pwm.setPWM(M2, 0, i);
                vTaskDelay((servo_msec * configTICK_RATE_HZ) / 1000L);
            }
            vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);
            for (int i=SERVO_HALF; i>=SERVO_MIN; i--) {
                pwm.setPWM(M1, 0, i);
                pwm.setPWM(M2, 0, i);
                vTaskDelay((servo_msec * configTICK_RATE_HZ) / 1000L);
            }
            vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);
        }
    }
}

static void logExec(void *arg) {
    while (1) {
        switch (writingType) {
            case WRITING_TRUE:
                String s = "";
                s += millis() - standard_millis;
                s += ",";
                s += log_value;
                s += "\n\n";
                char ch[100];
                s.toCharArray(ch, s.length());
                Serial.write(ch);
                vTaskDelay((10L * configTICK_RATE_HZ) / 1000L);
                break;
            case WRITING_LAST:
                // "00000" string is for informing to the frontend that it is the end of writing
                Serial.write("00000\n\n");
                writingType = WRITING_FALSE;
                break;
        }
    }
}

void loop() {}
