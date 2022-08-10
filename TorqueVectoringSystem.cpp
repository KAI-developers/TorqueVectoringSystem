#include "TorqueVectoringSystem.h"

#include "HallSensor.h"
#include "MPU6050.h"
#include <math.h>



TorqueVectoringSystem::TorqueVectoringSystem()
{
    f_motor_current_FL_A = 0.0;
    f_motor_current_FR_A = 0.0;
    f_motor_current_RL_A = 0.0;
    f_motor_current_RR_A = 0.0;


    f_motor_RPM_FL = 0.0;
    f_motor_RPM_FR = 0.0;
    f_motor_RPM_RL = 0.0;
    f_motor_RPM_RR = 0.0;

    //pedal box
    f_pedal_sensor_value = 0.0;
    //handle
    f_steering_sensor_value = 0.0;


    f_yaw_rate_meas_filtered_degs = 0.0;
    //temp value
    f_wheel_angle_deg = 0.0;
    
    f_vel_FL_ms = 0.0;
    f_vel_FR_ms = 0.0;
    f_vel_RL_ms = 0.0;
    f_vel_RR_ms = 0.0;

    f_vehicle_vel_ms = 0.0;

    f_yawrate_input_deg = 0.0;
    f_wheel_torque_FL_Nm = 0.0;
    f_wheel_torque_FR_Nm = 0.0;
    f_wheel_torque_RL_Nm = 0.0;
    f_wheel_torque_RR_Nm = 0.0;
    
    f_PID_yaw_rate2torque_FL_Nm = 0.0;
    f_PID_yaw_rate2torque_FR_Nm = 0.0;
    f_PID_yaw_rate2torque_RL_Nm = 0.0;
    f_PID_yaw_rate2torque_RR_Nm = 0.0;

    f_measured_torque_FL_Nm = 0.0;
    f_measured_torque_FR_Nm = 0.0;
    f_measured_torque_RL_Nm = 0.0;
    f_measured_torque_RR_Nm = 0.0;

    f_torque_FL_Nm = 0.0;               // targetted torque
    f_torque_FR_Nm = 0.0;
    f_torque_RL_Nm = 0.0;
    f_torque_RR_Nm = 0.0;

    f_output_throttle_FL = 0.0;
    f_output_throttle_FR = 0.0;
    f_output_throttle_RL = 0.0;
    f_output_throttle_RR = 0.0;

    f_PID_throttle_FL = 0.0;
    f_PID_throttle_FR = 0.0;
    f_PID_throttle_RL = 0.0;
    f_PID_throttle_RR = 0.0;

    f_PWM_input_FL = 0.0;
    f_PWM_input_FR = 0.0;
    f_PWM_input_RL = 0.0;
    f_PWM_input_RR = 0.0;

    gx = 0.0, gy = 0.0, gz = 0.0, ax = 0.0, ay = 0.0, az = 0.0;
    f_yawrate_meas_degs = 0.0;
    
}


/*
- RPM 구하는 함수
- rising edge 시간 차를 이용해 계산
- input
    ??

- output
    f_motor_RPM
- configuration
    MOTOR_POLE = 7

float TorqueVectoringSystem::CalRPM(HallSensor hall)
{
    //pc.printf("rpm: %f\n", hall.getRPM());        // for debuging
    float rpm = hall.getRPM();
    return rpm;
}
*/



/*
- motor RPM --> m/s 변환하는 함수.
- input
    f_motor_RPM: float, motor(RPM)
- output
    f_vel_ms: float, velocity(m/s)
- configuration
    WHEEL_RADIUS = 0.15m
    GEAR_RATIO = 5.27
*/
float TorqueVectoringSystem::CvtRPM2Vel(float f_motor_RPM)
{
    float f_vel_ms = 2. * PI * WHEEL_RADIUS * (f_motor_RPM / GEAR_RATIO) / 60;
    return f_vel_ms;
}


/*
- 평균속도 구하는 함수.
- input
    f_vel_RR_ms: float, velocity(m/s), RR
    f_vel_RL_ms: float, velocity(m/s), RL
- output
    f_avg_vel_ms: float, velocity(m/s), average
- configuration: X
*/


float TorqueVectoringSystem::CalAvgVel(float f_velocity1_ms, float f_velocity2_ms)
{
    float f_avg_vel_ms = (f_velocity1_ms + f_velocity2_ms) / 2;
    return f_avg_vel_ms;
}



/*
- 가변 저항 센서 값을 조향 각으로 변환하는 함수.

- configuration


*/
float TorqueVectoringSystem::CalHandlingVolt2WheelSteeringAngle(float f_handling_sensor_value)
{
    float resistor_angle = (f_handling_sensor_value - DEFAULT_VOLTAGE_INPUT) * MAX_RESISTOR_ANGLE;
    float handle_angle = resistor_angle * (MAX_HANDLE_ANGLE / MAX_RESISTOR_LIMITED_ANGLE);
    return handle_angle * (MAX_STEERING_ANGLE / MAX_HANDLE_ANGLE);
}



/*
- 차량 평균 속도와 바퀴 회전각을 이용하여 yawrate를 반환하는 함수.
- input
    f_wheel_steering_angle_deg: float, degree
    f_avg_vel_ms: float, velocity(m/s), average
- output
    f_input_yaw_rate_radps: float, yawrate(rad/s)
- configuration
    WHEEL_BASE = 1.390m
*/
float TorqueVectoringSystem::CalInputYawRate(float f_avg_vel_ms, float f_wheel_steering_angle_deg)
{
    float f_input_yaw_rate_radps = f_avg_vel_ms * sin(f_wheel_steering_angle_deg) / WHEEL_BASE;
    return f_input_yaw_rate_radps;
}



/*
- imu로 측정한 yawrate를 지수감쇠필터를 이용해 노이즈를 제거한 yawrate를 반환하는 함수.
- input
    f_IMU_yaw_rate_radps: from imu, yawrate(rad/s)
- output
    f_filtered_yaw_rate_radps: filtered, yawrate(rad/s)
- configuration
    ALPHA = 0.85
*/
float TorqueVectoringSystem::IMUFilter(float i_IMU_yaw_rate_radps)
{
    static float f_prev_yaw_rate_radps = 0.0;
    float f_filtered_yaw_rate_radps = ((1.0 - ALPHA) * f_prev_yaw_rate_radps) + (ALPHA * i_IMU_yaw_rate_radps);
    f_prev_yaw_rate_radps = f_filtered_yaw_rate_radps;
    return f_filtered_yaw_rate_radps;
}




/*
- 회전 반경, phi, 조향각을 이용하여 팔 길이를 구한 후, 팔 길이의 비율로 토크를 분배하는 함수.
- 팔 길이 계산
    fl = R * sin(phi - f_wheel_steering_angle_deg);
    fr = R * sin(phi + f_wheel_steering_angle_deg);
    rl = (-1) * R * sin(phi);
    rr = R * sin(phi)
- weight = 팔 길이 * 조향각 * throttle + throttle
- sum = weight(4방향)
- torque(1방향) = throttle / sum * weight(1방향)
- input
    f_wheel_steering_angle_deg: float, degree
    f_pedal_sensor_value : float, voltage(V)
- output
    f_wheel_torque_(4방향)_Nm: 4개 출력, float, torque(N*m)
- configuration
    WHEEL_BASE = 1.390m
    TRACK = 1.300m
*/
void TorqueVectoringSystem::WheelSteeringAngle2Torque(float f_wheel_steering_angle_deg, float f_pedal_sensor_value,
    float& f_wheel_torque_FL_Nm, float& f_wheel_torque_FR_Nm, float& f_wheel_torque_RL_Nm, float& f_wheel_torque_RR_Nm)

{
    float pedal_throttle_voltage = f_pedal_sensor_value * 5.0;          //  need to set by configuration
    float R = sqrt(pow(WHEEL_BASE / 2.0, 2.0) + pow(TRACK / 2.0, 2.0));

    float phi = atan((TRACK / 2.) / (WHEEL_BASE / 2.));

    float fl = (-1.) * R * sin(phi - f_wheel_steering_angle_deg);
    float fr = R * sin(phi + f_wheel_steering_angle_deg);
    float rl = (-1.) * R * sin(phi);
    float rr = R * sin(phi);

    float weight[4] = { 0 };

    float sum = 0.;

    weight[FL] = fl * f_wheel_steering_angle_deg * pedal_throttle_voltage + pedal_throttle_voltage;
    weight[FR] = fr * f_wheel_steering_angle_deg * pedal_throttle_voltage + pedal_throttle_voltage;
    weight[RL] = rl * f_wheel_steering_angle_deg * pedal_throttle_voltage + pedal_throttle_voltage;
    weight[RR] = rr * f_wheel_steering_angle_deg * pedal_throttle_voltage + pedal_throttle_voltage;


    sum = weight[FL] + weight[FR] + weight[RL] + weight[RR];

    f_wheel_torque_FL_Nm = 4 * (pedal_throttle_voltage / sum) * weight[FL] * (MAX_TORQUE / CONTROLLER_INPUT_VOLT_RANGE);
    f_wheel_torque_FR_Nm = 4 * (pedal_throttle_voltage / sum) * weight[FR] * (MAX_TORQUE / CONTROLLER_INPUT_VOLT_RANGE);
    f_wheel_torque_RL_Nm = 4 * (pedal_throttle_voltage / sum) * weight[RL] * (MAX_TORQUE / CONTROLLER_INPUT_VOLT_RANGE);
    f_wheel_torque_RR_Nm = 4 * (pedal_throttle_voltage / sum) * weight[RR] * (MAX_TORQUE / CONTROLLER_INPUT_VOLT_RANGE);

    if (f_wheel_torque_FL_Nm < 0.0)  f_wheel_torque_FL_Nm = 0.0;
    if (f_wheel_torque_FR_Nm < 0.0)  f_wheel_torque_FL_Nm = 0.0;
    if (f_wheel_torque_FL_Nm < 0.0)  f_wheel_torque_FL_Nm = 0.0;
    if (f_wheel_torque_FL_Nm < 0.0)  f_wheel_torque_FL_Nm = 0.0;
}


/*
- 차량 평균 속도와 바퀴 회전각을 이용해 계산한 yawrate와 imu로 측정하여 필터링된 yawrate를 입력으로 받아 PID 제어기로 torque 계산하는 함수.
- PID 제어기를 위한 error = 입력값(f_input_yaw_rate_radps) - 측정값(f_filtered_yaw_rate_radps)
- f_PID_yaw_rate2torque_Nm = KP * error (각각 4개)
- input
    f_input_yaw_rate_radps: float, yawrate(rad/s), 차량 평균 속도와 바퀴 회전각으로 계산.
    f_filtered_yaw_rate_radps: float, yawrate(rad/s), imu로 측정한 값을 필터링함.
- output
    f_PID_yaw_rate2torque_Nm (4개 각각 출력): float, torque(N*m)
- configuration
    KP_FOR_TORQUE (4개)
*/
void TorqueVectoringSystem::PIDYawRate2Torque(float f_input_yaw_rate_radps, float f_filtered_yaw_rate_radps,
    float& f_PID_yaw_rate2torque_FL_Nm, float& f_PID_yaw_rate2torque_FR_Nm,
    float& f_PID_yaw_rate2torque_RL_Nm, float& f_PID_yaw_rate2torque_RR_Nm)
{
    float f_yaw_rate_error_radps = f_input_yaw_rate_radps - f_filtered_yaw_rate_radps;


    if (f_yaw_rate_error_radps < 0.0)
    {
        f_PID_yaw_rate2torque_FL_Nm = KP_FOR_TORQUE_FL * f_yaw_rate_error_radps;
        f_PID_yaw_rate2torque_RL_Nm = KP_FOR_TORQUE_RL * f_yaw_rate_error_radps;

        f_PID_yaw_rate2torque_FR_Nm = 0;
        f_PID_yaw_rate2torque_RR_Nm = 0;
    }


    else
    {
        f_PID_yaw_rate2torque_RL_Nm = KP_FOR_TORQUE_RL * f_yaw_rate_error_radps;
        f_PID_yaw_rate2torque_RR_Nm = KP_FOR_TORQUE_RR * f_yaw_rate_error_radps;

        f_PID_yaw_rate2torque_FL_Nm = 0;
        f_PID_yaw_rate2torque_RL_Nm = 0;
    }

}

/*
- opamp 이용 전압 측정
- 션트 저항 양단 전압 증폭값을 읽음
- 옴의 법칙 이용 전류로 환산
- configuration
    amp rate(200)
    shunt resistance(50u)
*/
float TorqueVectoringSystem::OpAmp2Current(float f_opamp_ADC)
{
    float opamp_voltage = f_opamp_ADC * ANALOG_RANGE;
    float shunt_voltage = opamp_voltage / AMP_RATE_MOTOR;
    float motor_current = shunt_voltage / SHUNT_R;
    return motor_current;
}

/*
- hall current transducer 이용 전류 측정
- 3.3V 전원 공급한 transducer의 출력 전압값을 전류로 환산
- map 함수 이용
- configuration
    입력 : ANALOG_RANGE (3.3V)
    출력 : CURRENT_SENSOR_VALUE (200A)
*/
float TorqueVectoringSystem::ReadCurrentSensor(float current_sensor_value)
{
    float current_sensor_output_V = current_sensor_value * ANALOG_RANGE;

    // map 함수 형식 이용, -3.3V ~ 3.3V의 입력을 -200A ~ 200A로 scaling
    return (current_sensor_output_V - ((-1.)*ANALOG_RANGE)) * (CURRENT_SENSOR_RANGE * 2)
            / (ANALOG_RANGE * 2) + (-1.)*CURRENT_SENSOR_RANGE;
}

/*
- 캔 통신 인터페이스 모듈(MCP2515)로 측정한 전류를 플레밍의 왼손 법칙을 이용하여 토크로 계산하는 함수.
- input
    f_motor_current_A(4개): float, current(A)
- output
    f_measured_torque_Nm(4개): float, torque(N*m)
- configuration
    KT(모터 토크 상수) = 17 / 148
*/
float TorqueVectoringSystem::CvtCurrent2Torque(float f_motor_current_A)
{
    return KT * f_motor_current_A;

}

/*
- PID 제어기로 계산한 torque를 이용해 output_throttle 생성하는 함수.
- input
    f_torque_(4방향)_Nm: 각각 4개, float, torque(N*m)
- output
    f_output_throttle_(4방향): 각각 4개, float, voltage(V)
- configuration
    MAX_TORQUE = 17Nm
*/
float TorqueVectoringSystem::Torque2Throttle(float f_torque_Nm)
{
    float f_output_throttle = f_torque_Nm * (3.3 / MAX_TORQUE);
    return f_output_throttle;
}

/*
- PID 제어기로 계산한 torque와 캔 통신으로 측정한 전류를 이용해 계산한 torque를 입력으로 받아 PID 제어기로 throttle을 계산하는 함수.
- PID 제어기를 위한 error = 입력값(f_torque_(4방향)_Nm) - 측정값(f_output_throttle_(4방향))
- f_PID_throttle_(4방향) = KP_FOR_THROTTLE * error (각각 4개)
- input
    f_torque_(4방향)_Nm: 각각 4개, float, torque(N*m), PID 제어기로 계산한 토크.
    f_output_throttle_(4방향): 각각 4개, float, torque(N*m), 캔 통신으로 측정한 전류를 이용해 계산한 토크.
- output
    f_PID_throttle_(4방향): 각각 4개, float, voltage(V)
- configuration
    KP_FOR_THROTTLE
*/
float TorqueVectoringSystem::PIDforThrottle(float f_torque_Nm, float f_measured_torque_Nm)
{
    float error = f_torque_Nm - f_measured_torque_Nm;
    float f_PID_throttle = KP_FOR_THROTTLE * error * (3.3 / MAX_TORQUE);
    return f_PID_throttle;
}

/*
- PWM 함수의 입력 범위가 올바르도록 Saturation하는 함수.
- input
    f_output_throttle_(4방향): 각각 4개, float, torque(N*m)
    f_PID_throttle_(4방향): 각각 4개, float, voltage(V)
- output
    f_PWM_input: 각각 4개, float, PWM 출력
- configuration
    LOWER_BOUND = 0
    UPPER_BOUND = 1
*/
float TorqueVectoringSystem::SumFFandPID(float f_output_throttle, float f_PID_throttle)
{
    float sumFF = (f_output_throttle + f_PID_throttle) / 3.3;

    if (sumFF >= UPPER_BOUND)
        return UPPER_BOUND;
    else if (sumFF <= LOWER_BOUND)
        return LOWER_BOUND;
    else
        return sumFF;
}


void TorqueVectoringSystem::process_accel(
    PinName FL_HALL_PIN, PinName FR_HALL_PIN, PinName RL_HALL_PIN, PinName RR_HALL_PIN, 
    PinName HANDLE_SENSOR_PIN, PinName MPU_SDA, PinName MPU_SCL, PinName PEDAL_SENSOR_PIN,
    PinName FL_CURRENT_SENSOR_PIN, PinName FR_CURRENT_SENSOR_PIN, PinName RL_CURRENT_SENSOR_PIN, PinName RR_CURRENT_SENSOR_PIN, 
    PinName FL_OUTPUT_THROTTLE_PIN, PinName FR_OUTPUT_THROTTLE_PIN, PinName RL_OUTPUT_THROTTLE_PIN, PinName RR_OUTPUT_THROTTLE_PIN)
{
    
    Serial pc(USBTX, USBRX, 115200);

    HallSensor FL_Hall_A(FL_HALL_PIN);
    HallSensor FR_Hall_A(FR_HALL_PIN);
    HallSensor RL_Hall_A(RL_HALL_PIN);
    HallSensor RR_Hall_A(RR_HALL_PIN);

    AnalogIn Handle_Sensor(HANDLE_SENSOR_PIN);

    AnalogIn FL_Current_OUT(FL_CURRENT_SENSOR_PIN);
    AnalogIn FR_Current_OUT(FR_CURRENT_SENSOR_PIN);
    AnalogIn RL_Current_OUT(RL_CURRENT_SENSOR_PIN);
    AnalogIn RR_Current_OUT(RR_CURRENT_SENSOR_PIN);
    

    MPU6050 mpu(MPU_SDA, MPU_SCL);                  // (sda, scl)
    
    AnalogIn Pedal_Sensor(PEDAL_SENSOR_PIN);
    
    PwmOut FL_OUTPUT_Throttle(FL_OUTPUT_THROTTLE_PIN);
    PwmOut FR_OUTPUT_Throttle(FR_OUTPUT_THROTTLE_PIN);
    PwmOut RL_OUTPUT_Throttle(RL_OUTPUT_THROTTLE_PIN);
    PwmOut RR_OUTPUT_Throttle(RR_OUTPUT_THROTTLE_PIN);
    
    
    mpu.start();

    pc.printf("mpu6050 started!\r\n");


    
    FL_OUTPUT_Throttle.period_us(PWM_PERIOD_US);
    FR_OUTPUT_Throttle.period_us(PWM_PERIOD_US);
    RL_OUTPUT_Throttle.period_us(PWM_PERIOD_US);
    RR_OUTPUT_Throttle.period_us(PWM_PERIOD_US);


    while(1) {
        pc.printf("entered WHILE : \r\n");

        f_motor_RPM_FL = FL_Hall_A.getRPM();
        f_motor_RPM_FR = FR_Hall_A.getRPM();
        f_motor_RPM_RL = RL_Hall_A.getRPM();
        f_motor_RPM_RR = RR_Hall_A.getRPM();
        
        pc.printf("FL RPM : %f, FR RPM : %f, RL RPM : %f, RR RPM : %f\r\n", f_motor_RPM_FL, f_motor_RPM_FR, f_motor_RPM_RL, f_motor_RPM_RR);



        f_vel_FL_ms = CvtRPM2Vel(f_motor_RPM_FL);
        f_vel_FR_ms = CvtRPM2Vel(f_motor_RPM_FR);
        f_vel_RL_ms = CvtRPM2Vel(f_motor_RPM_RL);
        f_vel_RR_ms = CvtRPM2Vel(f_motor_RPM_RR);
        
        pc.printf("FL vel : %f, FR vel : %f, RL vel : %f, RR vel : %f\r\n", f_vel_FL_ms, f_vel_FR_ms, f_vel_RL_ms, f_vel_RR_ms);



        f_vehicle_vel_ms = CalAvgVel(f_vel_RR_ms, f_vel_RL_ms);

        pc.printf("Car velocity : %f \r\n", f_vehicle_vel_ms);



    
        // f_steering_sensor_value 받기!
        pc.printf("Handle sensor value : %f\r\n", Handle_Sensor.read());

        f_wheel_angle_deg = CalHandlingVolt2WheelSteeringAngle(Handle_Sensor.read());

        pc.printf("wheel angle : %f\r\n",f_wheel_angle_deg);
        


        f_yawrate_input_deg = CalInputYawRate(f_vehicle_vel_ms, f_wheel_angle_deg);

        pc.printf("target yaw rate : %f \t\t", f_yawrate_input_deg);

        
        ///////////////////////////////////////////
        // update sensor data
        // move present value to previous value
        // and update new data to present value
        //////////////////////////////////////////
        mpu.read(&gx, &gy, &gz, &ax, &ay, &az);
        f_yawrate_meas_degs = gz;
        f_yaw_rate_meas_filtered_degs = IMUFilter(f_yawrate_meas_degs);

        pc.printf("measured yaw rate : %f \r\n", f_yaw_rate_meas_filtered_degs);
    

        //f_pedal_sensor_value 받기!
        f_pedal_sensor_value = Pedal_Sensor.read();

        pc.printf("pedal sensor value : %f\r\n", f_pedal_sensor_value);


        WheelSteeringAngle2Torque(f_wheel_angle_deg, f_pedal_sensor_value,
            f_wheel_torque_FL_Nm, f_wheel_torque_FR_Nm,
            f_wheel_torque_RL_Nm, f_wheel_torque_RR_Nm);

        pc.printf("feedforward torque : \r\n");
        pc.printf("FL : %f, FR : %f, RL : %f, RR : %f\r\n", f_wheel_torque_FL_Nm, f_wheel_torque_FR_Nm, f_wheel_torque_RL_Nm, f_wheel_torque_RR_Nm);
        
        PIDYawRate2Torque(f_yawrate_input_deg, f_yaw_rate_meas_filtered_degs,
            f_PID_yaw_rate2torque_FL_Nm, f_PID_yaw_rate2torque_FR_Nm,
            f_PID_yaw_rate2torque_RL_Nm, f_PID_yaw_rate2torque_RR_Nm);
            
        pc.printf("P controlled yaw rate output \r\n");
        pc.printf("FL : %f, FR : %f, RL : %f, RR : %f\r\n", f_PID_yaw_rate2torque_FL_Nm, f_PID_yaw_rate2torque_FR_Nm, f_PID_yaw_rate2torque_RL_Nm, f_PID_yaw_rate2torque_RR_Nm);

        /*
       
        f_motor_current_FL_A = OpAmp2Current(FL_Opamp_OUT.read());
        f_motor_current_FR_A = OpAmp2Current(FR_Opamp_OUT.read());
        f_motor_current_RL_A = OpAmp2Current(RL_Opamp_OUT.read());
        f_motor_current_RR_A = OpAmp2Current(RR_Opamp_OUT.read());

        pc.printf("current value \r\n");
        pc.printf("FL : %f, FR : %f, RL : %f, RR : %f\r\n", f_motor_current_FL_A, f_motor_current_FR_A, f_motor_current_RL_A, f_motor_current_RR_A);
        */

        //f_motor_current 받기!
        f_motor_current_FL_A = ReadCurrentSensor(FL_Current_OUT.read());
        f_motor_current_FR_A = ReadCurrentSensor(FR_Current_OUT.read());
        f_motor_current_RL_A = ReadCurrentSensor(RL_Current_OUT.read());
        f_motor_current_RR_A = ReadCurrentSensor(RR_Current_OUT.read());


        pc.printf("current value \r\n");
        pc.printf("FL : %f, FR : %f, RL : %f, RR : %f\r\n", f_motor_current_FL_A, f_motor_current_FR_A, f_motor_current_RL_A, f_motor_current_RR_A);
        


        
        f_measured_torque_FL_Nm = CvtCurrent2Torque(f_motor_current_FL_A);
        f_measured_torque_FR_Nm = CvtCurrent2Torque(f_motor_current_FR_A);
        f_measured_torque_RL_Nm = CvtCurrent2Torque(f_motor_current_RL_A);
        f_measured_torque_RR_Nm = CvtCurrent2Torque(f_motor_current_RR_A);

        pc.printf("measured torque \r\n");
        pc.printf("FL : %f, FR : %f, RL : %f, RR : %f\r\n", f_measured_torque_FL_Nm, f_measured_torque_FR_Nm, f_measured_torque_RL_Nm, f_measured_torque_RR_Nm);
        
        
    

        f_torque_FL_Nm = f_wheel_torque_FL_Nm + f_PID_yaw_rate2torque_FL_Nm;
        f_torque_FR_Nm = f_wheel_torque_FR_Nm + f_PID_yaw_rate2torque_FR_Nm;
        f_torque_RL_Nm = f_wheel_torque_RL_Nm + f_PID_yaw_rate2torque_RL_Nm;
        f_torque_RR_Nm = f_wheel_torque_RR_Nm + f_PID_yaw_rate2torque_RR_Nm;

        pc.printf("actual generating torque\r\n");
        pc.printf("FL : %f, FR : %f, RL : %f, RR : %f\r\n", f_torque_FL_Nm, f_torque_FR_Nm, f_torque_RL_Nm, f_torque_RR_Nm);
    
    

    
        f_output_throttle_FL = Torque2Throttle(f_torque_FL_Nm);
        f_output_throttle_FR = Torque2Throttle(f_torque_FR_Nm);
        f_output_throttle_RL = Torque2Throttle(f_torque_RL_Nm);
        f_output_throttle_RR = Torque2Throttle(f_torque_RR_Nm);

        pc.printf("feedforward output throttle signal(voltage)\r\n");
        pc.printf("FL : %f, FR : %f, RL : %f, RR : %f\r\n", f_output_throttle_FL, f_output_throttle_FR, f_output_throttle_RL, f_output_throttle_RR);

    
    

        f_PID_throttle_FL = PIDforThrottle(f_torque_FL_Nm, f_measured_torque_FL_Nm);
        f_PID_throttle_FR = PIDforThrottle(f_torque_FR_Nm, f_measured_torque_FR_Nm);
        f_PID_throttle_RL = PIDforThrottle(f_torque_RL_Nm, f_measured_torque_RL_Nm);
        f_PID_throttle_RR = PIDforThrottle(f_torque_RR_Nm, f_measured_torque_RR_Nm);

        pc.printf("feedback output throttle signal(voltage)\r\n");
        pc.printf("FL : %f, FR : %f, RL : %f, RR : %f\r\n", f_PID_throttle_FL, f_PID_throttle_FR, f_PID_throttle_RL, f_PID_throttle_RR);

    

    
        f_PWM_input_FL = SumFFandPID(f_output_throttle_FL, f_PID_throttle_FL);
        f_PWM_input_FR = SumFFandPID(f_output_throttle_FR, f_PID_throttle_FR);
        f_PWM_input_RL = SumFFandPID(f_output_throttle_RL, f_PID_throttle_RL);
        f_PWM_input_RR = SumFFandPID(f_output_throttle_RR, f_PID_throttle_RR);

        pc.printf("actual output throttle signal(voltage)\r\n");
        pc.printf("FL : %f, FR : %f, RL : %f, RR : %f\r\n", f_PWM_input_FL, f_PWM_input_FR, f_PWM_input_RL, f_PWM_input_RR);



    
        FL_OUTPUT_Throttle = f_PWM_input_FL;            // OUTPUT from mbed, input from controller
        FL_OUTPUT_Throttle = f_PWM_input_FR;
        FL_OUTPUT_Throttle = f_PWM_input_RL;
        FL_OUTPUT_Throttle = f_PWM_input_RR;

    }
}