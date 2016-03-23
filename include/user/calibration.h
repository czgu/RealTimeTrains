#ifndef _CALIBRATION_H_
#define _CALIBRATION_H_

/**
 *  cal 0 <train_id> <train_speed> <sensor module> <sensor id>
 */
void calibrate_stop();

/**
 *  cal 1 <train_id> <min speed> <max speed>
 */
void calibrate_velocity();

/**
 *  cal 2 <train_id> <train speed> <0> <0> <delay time>
 */
void calibrate_acceleration_move();

/**
 *  cal 3 <train_id> <train speed 0> <train speed 1>
 */
void calibrate_acceleration_delta();

/**
 *  cal 4 <train_id> <min speed> <max speed>
 */
void calibrate_stop_time();

/**
 *  cal 6 <train_id>
 */
void calibrate_find_train();

#endif
