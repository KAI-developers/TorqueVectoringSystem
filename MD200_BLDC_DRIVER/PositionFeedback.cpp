// 모터 CW 회전 시 차량 조향각 오른쪽으로 돌아감
// https://m.blog.naver.com/PostView.naver?isHttpsRedirect=true&blogId=roboholic84&logNo=221044375678 참고함


#include "mbed.h"
#include "MD200.h"



#define KP_POSITION                     30.0

#define DEFAULT_VOLTAGE_INPUT           0.5
#define MAX_RESISTOR_ANGLE              300.0
#define MAX_RESISTOR_LIMITED_ANGLE      240.0
#define MAX_HANDLE_ANGLE                300.0
#define MAX_STEERING_BIG_ANGLE          67.238
#define MAX_STEERING_SMALL_ANGLE        55.267
#define MAX_STEERING_ANGLE              (MAX_STEERING_BIG_ANGLE + MAX_STEERING_SMALL_ANGLE) / 2.0




float Handle2WheelSteeringAngle(float f_handling_sensor_value);



int main()
{
    // Serial pc(USBTX, USBRX);


    
    MD200 driver(p8, p9, p10, p11, p21);     // INT_SPEED, CW/CCW, RUN/BRAKE, START/STOP, SPEED(0~5V)
    driver.setINT_SPEED(EXTERNAL_SPEED);
    driver.enableBrake(BRAKE_ON);            // 모터가 구동을 안할 때 잠김

    AnalogIn handle_sensor(p15);             // 조향각 읽는 저항
    float target_steering_deg = 0.0;
    float measured_steering_deg = 0.0;

    float error = 0.0;
    float motor_control_speed_RPM = 0.0;

    
    while(1)
    {
        // 여기 목표 각도 publish 필요
        measured_steering_deg = Handle2WheelSteeringAngle(handle_sensor.read());        // 왼쪽 조향이 양수 각
        error = target_steering_deg - measured_steering_deg;                            // 양수면 왼쪽으로 더 돌아야 함
        motor_control_speed_RPM = KP_POSITION * error;                                   

        if (motor_control_speed_RPM >= 0.0)             // 왼쪽으로 더 돌아야 함, 모터는 CCW회전
            driver.runMotor(CCW, RUN, (abs(motor_control_speed_RPM) < 3300.0 ? abs(motor_control_speed_RPM) : 3300.0));
        else if (motor_control_speed_RPM < 0.0)         // 오른쪽으로 더 돌아야 함, 모터는 CW회전
            driver.runMotor(CW, RUN, (abs(motor_control_speed_RPM) < 3300.0 ? abs(motor_control_speed_RPM) : 3300.0));
        

        // pc.printf("target steering angle : %f \r\n", target_steering_deg);           // 어짜피 ros로 받으면 시리얼출력 확인이 안되긴 함
        // pc.printf("measured steering angle : %f \r\n", measured_steering_deg);
        // pc.printf("motor control speed(RPM) : %f \r\n", motor_control_speed_RPM);
    }

}






float Handle2WheelSteeringAngle(float f_handling_sensor_value)
{
    // float resistor_angle = (f_handling_sensor_value - DEFAULT_VOLTAGE_INPUT) * MAX_RESISTOR_ANGLE;
    // float handle_angle = resistor_angle * (MAX_HANDLE_ANGLE / MAX_RESISTOR_LIMITED_ANGLE);
    // return handle_angle * (MAX_STEERING_ANGLE / MAX_HANDLE_ANGLE);

    float resistor_angle;
    float handle_angle;
    
    resistor_angle = (f_handling_sensor_value - DEFAULT_VOLTAGE_INPUT) * MAX_RESISTOR_ANGLE;
    handle_angle = resistor_angle * (MAX_HANDLE_ANGLE / MAX_RESISTOR_LIMITED_ANGLE);
    handle_angle = handle_angle * (MAX_STEERING_ANGLE / MAX_HANDLE_ANGLE);

    return handle_angle;
}