/**
 * ****************************(C) COPYRIGHT 2023 Blue Bear****************************
 * @file       auto_task.cpp
 * @brief      
 * 
 * @note       
 * @history:
 *   Version   Date            Author          Modification    Email
 *   V1.0.0    Mar-03-2023     Tianyi Zhang    1. start        tz137@duke.edu/shadow_rogue@qq.com
 * 
 * @verbatim
 * ==============================================================================
 * 
 * ==============================================================================
 * @endverbatim
 * ****************************(C) COPYRIGHT 2023 Blue Bear****************************
 */
#include "dku/auto/auto_task.hpp"
#include "dku/functional_task.hpp"
#include "dku/sensor_task.hpp"
#include "pros/rtos.hpp"
#include <cstdint>
#include <Math.h>


auto_control_t auto_control;
void auto_init(auto_control_t* init)
{

    init->chassis_voltage = get_chassis_voltage_point();
    init->functional_status = get_functional_device_status();
    init->sensor_data = get_sensor_data_point();
    pros::Mutex init_mutex;
    init_mutex.take();
    {
        // printf("here2");
        init->functional_status->flywheel = E_FLYWHEEL_STATUS_OFF;
        init->functional_status->intake_motor = E_FUNCTIONAL_MOTOR_STATUS_OFF;
        // init->functional_status->roller_motor = E_FUNCTIONAL_MOTOR_STATUS_BACKWARD;
    }
    init_mutex.give();
    // printf("here3");
    // init->current_pos.current_x = 
    // init->current_pos.current_y = 
    // init->current_pos.current_dir = 
    //
}

/**
 * @brief           let robot turn to specific point
 * @param[in]       target_x: aimed position x
 * @param[in]       target_y: aimed position y
 * @param[in,out]   turn: find current position,give chassis voltage
 * @retval         
 */
void turn_to(double target_x, double target_y, auto_control_t* turn)
{
    std::int32_t analog_right_x = 0; //simulate the joystick
    // double rad = atan2f((target_x - turn->current_pos.current_x), (target_y - turn->current_pos.current_y));
    double rad = atan2f((target_x - turn->sensor_data->gps_front_data.gps_pos.x), (target_y - turn->sensor_data->gps_front_data.gps_pos.y));
    // double rad = atan2f((target_x - turn->sensor_data->gps_front_data.gps_pointer->get_status().x), (target_y - turn->sensor_data->gps_front_data.gps_pointer->get_status().y));
    double angle = rad*(180/PI);
    pros::lcd::print(0, "heading: ", turn->sensor_data->gps_front_data.gps_pos.yaw); 
    // printf("heading: %lf\n", turn->sensor_data->gps_front_data.gps_pos.yaw);
    // while (abs((int)(turn->current_pos.current_dir - angle))>=1) {
    pros::Mutex turn_mutex;
    std::int32_t angle_difference = abs((int)(turn->sensor_data->gps_front_data.gps_pos.yaw - angle));
    // std::int32_t angle_difference = abs((int)(turn->sensor_data->gps_front_data.gps_pointer->get_status().yaw - angle));
    if (angle_difference > 180) {
        angle_difference = 360 - angle_difference;
    }
    std::uint32_t now_1 = pros::millis();
    while (angle_difference>=3) {

        analog_right_x = CHASSIS_MOVE_SPEED;
        turn_mutex.take();
        {
            turn->chassis_voltage[0] = analog_right_x;
            turn->chassis_voltage[1] = analog_right_x;
            turn->chassis_voltage[2] = -analog_right_x;
            turn->chassis_voltage[3] = -analog_right_x;
        }
        turn_mutex.give();
        pros::lcd::print(2, "ANGLE: %lf", angle); 
        printf("angle %5.5lf, yaw %5.5lf, yaw-angle %5d\n", angle, turn->sensor_data->gps_front_data.gps_pos.yaw, angle_difference);
        // printf("angle %5.5lf, yaw %5.5lf, yaw-angle %5d\n", angle, turn->sensor_data->gps_front_data.gps_pointer->get_status().yaw, angle_difference);
        rad = atan2f((target_x - turn->sensor_data->gps_front_data.gps_pos.x), (target_y - turn->sensor_data->gps_front_data.gps_pos.y));
        angle = rad*(180/PI);
        angle_difference = abs((int)(turn->sensor_data->gps_front_data.gps_pos.yaw - angle));
        // angle_difference = abs((int)(turn->sensor_data->gps_front_data.gps_pointer->get_status().yaw - angle));
        if (angle_difference > 180) {
            angle_difference = 360 - angle_difference;
        }
        pros::Task::delay_until(&now_1, AUTO_TASK_TIME_MS);
    }
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    turn_mutex.take();
    {
        turn->chassis_voltage[0] = 0;
        turn->chassis_voltage[1] = 0;
        turn->chassis_voltage[2] = 0;
        turn->chassis_voltage[3] = 0;
    }
    turn_mutex.give();
}

/**
 * @brief           let robot turn a relative angle, unit is degree(not rad!)
 * @param[in]       target_angle: aimed position x
 * @param[in,out]   turn: find current position,give chassis voltage
 * @retval          null
 */
// TODO: use enum state value to handle the move and rotate state, ro achieve
// high level movement(along the circle/or...)
void turn_relative(double target_angle, auto_control_t* turn)
{
    std::int32_t direction_flag = -1;
    if (target_angle > 0 ) {
        direction_flag = 1; //turn right TODO: use enum
    }
    double angle = 0;
    std::int32_t analog_right_x = 0; //simulate the joystick
    analog_right_x = CHASSIS_MOVE_SPEED;
    pros::Mutex turn_mutex;
    double yaw_gyro = 0;
    std::uint32_t now_0 = pros::millis();
    printf("angle %lf, target: %lf\n", angle, target_angle);
    while (angle < target_angle*direction_flag) {
        turn_mutex.take();
        {
            turn->chassis_voltage[0] = analog_right_x*direction_flag;
            turn->chassis_voltage[1] = analog_right_x*direction_flag;
            turn->chassis_voltage[2] = -analog_right_x*direction_flag;
            turn->chassis_voltage[3] = -analog_right_x*direction_flag;
        }
        turn_mutex.give();
        yaw_gyro = turn->sensor_data->gps_front_data.gps_gyro.z;
        angle += yaw_gyro * (AUTO_TASK_TIME_MS/1000.0) * (double)direction_flag;
        printf("angle %lf\n", angle);
        // pros::lcd::print(2, "angle: %lf\n", angle);
        pros::Task::delay_until(&now_0, AUTO_TASK_TIME_MS);
    }
    turn_mutex.take();
    {
        turn->chassis_voltage[0] = 0;
        turn->chassis_voltage[1] = 0;
        turn->chassis_voltage[2] = 0;
        turn->chassis_voltage[3] = 0;
    }
    turn_mutex.give();
}


/**
 * @brief           let robot turn to specific point
 * @param[in]       direction: direction, -1 or +1
 * @param[in]       time: a period of time, unit:ms
 * @param[in,out]   turn: find current position,give chassis voltage
 * @retval          null
 */
void turn_time(double direction, double time, auto_control_t* turn)
{
    std::int32_t analog_right_x = 127;
    pros::Mutex turn_mutex;
    turn_mutex.take();
    {
        turn->chassis_voltage[0] = analog_right_x*direction;
        turn->chassis_voltage[1] = analog_right_x*direction;
        turn->chassis_voltage[2] = -analog_right_x*direction;
        turn->chassis_voltage[3] = -analog_right_x*direction;
    }
    turn_mutex.give();
    pros::delay(time);
    analog_right_x = 0;
    turn_mutex.take();
    {
        turn->chassis_voltage[0] = analog_right_x*direction;
        turn->chassis_voltage[1] = analog_right_x*direction;
        turn->chassis_voltage[2] = -analog_right_x*direction;
        turn->chassis_voltage[3] = -analog_right_x*direction;
    }
    turn_mutex.give();
}
/**
 * @brief           let robot move to specific point
 * @param[in]       target_x: aimed position x
 * @param[in]       target_y: aimed position y
 * @param[in,out]   move: find current position,give chassis voltage
 * @retval         
 */
void move_to(double target_x, double target_y, auto_control_t* move)
{
    turn_to(target_x, target_y, move);
    std::int32_t analog_left_y = 0; //simulate the joystick
    // double distance = sqrt(pow((target_x - move->current_pos.current_x),2) + pow((target_y - move->current_pos.current_y),2));
    double distance = sqrt(pow((target_x - move->sensor_data->gps_front_data.gps_pos.x),2) + pow((target_y - move->sensor_data->gps_front_data.gps_pos.y),2));
    printf("distance %5.5lf\n", distance);

    pros::Mutex move_mutex;
    while (abs((int)(distance*100))>=10) {
        int analog_left_y = 50;
        move_mutex.take();
        {
            move->chassis_voltage[0] = analog_left_y;
            move->chassis_voltage[1] = analog_left_y;
            move->chassis_voltage[2] = analog_left_y;
            move->chassis_voltage[3] = analog_left_y;
        }
        move_mutex.give();
        double distance = sqrt(pow((target_x - move->sensor_data->gps_front_data.gps_pos.x),2) + pow((target_y - move->sensor_data->gps_front_data.gps_pos.y),2));
        printf("distance %5.5lf\n", distance);
    }
    printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
    move_mutex.take();
    {
        move->chassis_voltage[0] = 0;
        move->chassis_voltage[1] = 0;
        move->chassis_voltage[2] = 0;
        move->chassis_voltage[3] = 0;
    }
    move_mutex.give();
}

/**
 * @brief           let robot turn to specific point
 * @param[in]       direction: direction, -1 or +1
 * @param[in]       time: a period of time
 * @param[in,out]   turn: find current position,give chassis voltage
 * @retval          null
 */
void move_time(double direction, double time, auto_control_t* move)
{
    std::int32_t analog_left_y = CHASSIS_MOVE_SPEED;
    pros::Mutex turn_mutex;
    turn_mutex.take();
    {
        move->chassis_voltage[0] = analog_left_y*direction;
        move->chassis_voltage[1] = analog_left_y*direction;
        move->chassis_voltage[2] = analog_left_y*direction;
        move->chassis_voltage[3] = analog_left_y*direction;
    }
    turn_mutex.give();
    pros::delay(time);
    analog_left_y = 0;
    turn_mutex.take();
    {
        move->chassis_voltage[0] = analog_left_y*direction;
        move->chassis_voltage[1] = analog_left_y*direction;
        move->chassis_voltage[2] = analog_left_y*direction;
        move->chassis_voltage[3] = analog_left_y*direction;
    }
    turn_mutex.give();
}



/**
 * @brief           move a relative distance, unit is meter
 * @param[in]       target_distance: aimed distance
 * @param[in,out]   move: find current position,give chassis voltage
 * @param[in,out]   analog_left_y: the value to simulate joystic
 * @retval          null
 */
// TODO: use enum state value to handle the move and rotate state, ro achieve
// high level movement(along the circle/or...)
void move_vertical_relative_speed(double target_distance, auto_control_t* move, std::int32_t analog_left_y)
{
    std::int32_t direction_flag = -1;
    if (target_distance > 0 ) {
        direction_flag = 1; //move forward TODO: use enum
    }   
    double v0 = 0;
    double vt = 0;
    double x = 0;
    double a = 0;
    pros::Mutex turn_mutex;
    std::uint32_t now_2 = pros::millis();
    while (x < target_distance*direction_flag) {
        turn_mutex.take();
        {
            move->chassis_voltage[0] = analog_left_y*direction_flag;
            move->chassis_voltage[1] = analog_left_y*direction_flag;
            move->chassis_voltage[2] = analog_left_y*direction_flag;
            move->chassis_voltage[3] = analog_left_y*direction_flag;
        }
        turn_mutex.give();
        a = -move->sensor_data->gps_front_data.gps_acc.y;
        v0 = vt;
        vt = v0 + a*(AUTO_TASK_TIME_MS/100.0);
        x += (v0*(AUTO_TASK_TIME_MS/100.0) + 0.5*a*(AUTO_TASK_TIME_MS/100.0)*(AUTO_TASK_TIME_MS/100.0))*direction_flag;
        printf("distance %lf", x);
        pros::lcd::print(1, "distance: %lf", x);
        pros::lcd::print(2, "acc: %lf", move->sensor_data->gps_front_data.gps_acc.y);
        pros::Task::delay_until(&now_2, AUTO_TASK_TIME_MS);
    }
    turn_mutex.take();
    {
        move->chassis_voltage[0] = 0;
        move->chassis_voltage[1] = 0;
        move->chassis_voltage[2] = 0;
        move->chassis_voltage[3] = 0;
    }
    turn_mutex.give();
}

void move_horizontal_relative(double target_distance, auto_control_t* move)
{
    std::int32_t direction_flag = -1;
    if (target_distance > 0 ) {
        direction_flag = 1; //move right TODO: use enum same as before
    }  
    double v0 = 0;
    double vt = 0;
    double x = 0;
    double a = 0;
    pros::Mutex turn_mutex;
    std::int32_t analog_left_y = CHASSIS_MOVE_SPEED;
    std::uint32_t now_2 = pros::millis();
    while (abs(x) < target_distance*direction_flag ) {
        turn_mutex.take();
        {
            move->chassis_voltage[0] = analog_left_y*direction_flag ;
            move->chassis_voltage[1] = -analog_left_y*direction_flag ;
            move->chassis_voltage[2] = -analog_left_y*direction_flag ;
            move->chassis_voltage[3] = analog_left_y*direction_flag ;
        }
        turn_mutex.give();
        a = -move->sensor_data->gps_front_data.gps_acc.x;
        v0 = vt;
        vt = v0 + a*(AUTO_TASK_TIME_MS/100.0);
        x += (v0*(AUTO_TASK_TIME_MS/100.0) + 0.5*a*(AUTO_TASK_TIME_MS/100.0)*(AUTO_TASK_TIME_MS/100.0))*direction_flag ;
        printf("distance %lf", x);
        pros::lcd::print(1, "distance: %lf", x);
        pros::lcd::print(2, "acc: %lf", move->sensor_data->gps_front_data.gps_acc.x);
        pros::Task::delay_until(&now_2, AUTO_TASK_TIME_MS);
    }
    turn_mutex.take();
    {
        move->chassis_voltage[0] = 0;
        move->chassis_voltage[1] = 0;
        move->chassis_voltage[2] = 0;
        move->chassis_voltage[3] = 0;
    }
    turn_mutex.give();
}

/**
  * @brief          auto task. auto task is not a while true loop
  * @param[in]      param: null
  * @retval         none
  */
/**
  * @brief          自动任务
  * @param[in]      param: 空
  * @retval         none
  */
void auto_task_fn(void* param)
{

}
/**
 * @brief           kick out 3 plates
 * @param[in,out]   kick: change the voltage of index
 * @retval          null
 */
void kick_out(auto_control_t* kick)
{
    pros::Mutex kick_mutex;
    kick_mutex.take();
    {
        kick->functional_status->flywheel = E_FLYWHEEL_STATUS_SPEED_HIGH;
    }
    kick_mutex.give();
    kick_mutex.take();
    {
        kick->functional_status->intake_motor = E_FUNCTIONAL_MOTOR_STATUS_OFF;
    }
    kick_mutex.give();
    pros::delay(2000);
    kick_mutex.take();
    {
        kick->functional_status->intake_motor = E_FUNCTIONAL_MOTOR_STATUS_BACKWARD;
    }
    kick_mutex.give();
    pros::delay(2000);
    kick_mutex.take();
    {
        kick->functional_status->index_motor = E_FUNCTIONAL_MOTOR_STATUS_FORWARD;
    }
    kick_mutex.give();
    pros::delay(5000);
    kick_mutex.take();
    {
        kick->functional_status->index_motor = E_FUNCTIONAL_MOTOR_STATUS_OFF;
    }
    kick_mutex.give();
    kick_mutex.take();
    {
        kick->functional_status->flywheel = E_FLYWHEEL_STATUS_OFF;
    }
    kick_mutex.give();
    kick_mutex.take();
    {
        kick->functional_status->intake_motor = E_FUNCTIONAL_MOTOR_STATUS_FORWARD;
    }
    kick_mutex.give();
}


/**
 * @brief           rotate the roller
 * @param[in]       time: the rotate time, unit: ms
 * @param[in,out]   rotate: the control pointer
 * @retval          null
 */
void rotate_roller(std::int32_t time , auto_control_t* rotate)
{
    pros::Mutex rotate_mutex;
    rotate_mutex.take();
    {
        rotate->functional_status->roller_motor = E_FUNCTIONAL_MOTOR_STATUS_BACKWARD;
    }
    rotate_mutex.give();
    pros::delay(time);
    rotate_mutex.take();
    {
        rotate->functional_status->roller_motor = E_FUNCTIONAL_MOTOR_STATUS_OFF;
    }
    rotate_mutex.give();
}

/**
 * @brief            get the current status struck(x,y,direction)
 * @param[out]       null
 * @return           current_status_t*
 * @retval           
 */

current_status_t* get_current_status_pointer(void)
{
    return &auto_control.current_pos;
}



