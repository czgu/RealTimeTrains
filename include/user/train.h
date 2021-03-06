#ifndef _TRAIN_H_
#define _TRAIN_H_

#include <track_data.h>

extern float train_model_speed[15];
extern float train_model_stop_dist[15];

int train_cmd(char c1, char c2);

typedef struct TrainCalibrationProfile {
    float velocity[15];
    //float calibration_weight[15];
    float stop_distance[15];
} TrainCalibrationProfile;

#define TRACK_MAX_EDGES_BTW_SENSORS 5

typedef struct TrainCalibrationWork {
    // dynamic calibration fields
    int num_arcs_passed;
    track_edge* arcs_passed[TRACK_MAX_EDGES_BTW_SENSORS];
    int dist_from_last_sensor;       // running distance from last sensor
    float weight_factors;            // running weighted sum of track weight factors
} TrainCalibrationWork;

typedef struct TrainModelPosition {
    track_edge* arc;
    track_node* next_sensor;

    int sensor_triggered_time;

    track_node* stop_node;
    int stop_node_dist;

    float estimated_next_sensor_dist;

    float dist_travelled; // distance travelled
    int updated_time;

    // not really position-related, but necessary
    int stop_dist;          // instantaneous stopping distance
    int stop_time;          // time the train takes to stop
} TrainModelPosition;

void train_calibration_profile_init(TrainCalibrationProfile* profile, int id);
void train_calibration_work_init(TrainCalibrationWork* work);
void train_model_position_init(TrainModelPosition* position);

#define TRAIN_ID_MIN 58
#define TRAIN_ID_MAX 72

#define TRAIN_LOCATION_UPDATE_TIME_INTERVAL 10
#define TRAIN_SENSOR_HIT_TOLERANCE -500
#define TRAIN_LOOK_AHEAD_DIST 200

typedef struct TrainModel {
	short id;		// [1,80]
	short speed;	// [0,14]
	short previous_speed;

    int speed_updated_time; // used to calculate acceleration
    float velocity;
#define TRAIN_ACCELERATION 0.01     // we assume that acceleration is approx. constant
#define TRAIN_ACCELERATION_DELTA (TRAIN_ACCELERATION * TRAIN_LOCATION_UPDATE_TIME_INTERVAL)

#define TRAIN_DECELERATION (-0.015)     // we assume that acceleration is approx. constant
#define TRAIN_DECELERATION_DELTA (TRAIN_DECELERATION * TRAIN_LOCATION_UPDATE_TIME_INTERVAL)
    int accel_const;        // -1 for deceleration, 0 for no acceleration, 1 for acceleration
    int prev_accel;

#define TRAIN_MODEL_DIRECTION_FWD 0x1
#define TRAIN_MODEL_ACTIVE 0x2
#define TRAIN_MODEL_POSITION_KNOWN 0x4
#define TRAIN_MODEL_TRACKED 0x8
    short bitmap;

    TrainModelPosition position;
    TrainCalibrationProfile profile;
    TrainCalibrationWork calibration_work;
} TrainModel;

void train_model_init(TrainModel* train, int id);

// Initialization and helper
void train_model_init_location(TrainModel* train, int time, int switches, track_node* sensor_start);
void train_model_update(TrainModel* train, int time,  int switches);
void train_model_update_location(TrainModel* train, int time,  int switches);

// track helper
track_edge* track_next_arc(int switches, track_edge* current, float* dist);
track_node* track_next_sensor_node(int switches, track_edge* current, float* dist);
int track_ahead_contain_node(track_node* node, int switches, track_node* current, int lookahead_dist, int* dist_to_node);


void train_model_update_speed(TrainModel* train, int time, int switches, int speed);
void train_model_reverse_direction(TrainModel* train, int time, int switches);

void train_model_next_sensor_triggered(TrainModel* train, int time, int switches, int* error);

void train_set_speed(int location_server_tid, int train, int speed);

/*
 * Sends reverse command to train
 */
void train_reverse(int location_server_tid, int train);

typedef enum { STRAIGHT = 0, CURVED } sw_state;

#define SWITCHCODE 0x20

// Masks for setting switch
#define SMASK 0x1
#define CMASK 0x2



#define SWADDRBASEL 1
#define SWADDRBASEH 0x99

#define SWLENL 18
#define SWLENH 4
#define SWLEN (SWLENL + SWLENH)

typedef struct Track_Switch {
	char addr;
	sw_state status;
} Track_Switch;

void switch_init(Track_Switch*, char addr);
// opt = 1 to turn solenoid off after
// track_switch is in range[1, 22]
void track_set_switch(int location_server_tid, int track_switch, char curve, int soloff);
void track_soloff();

// switch_index is in range[1, 22]
inline void set_switch_normalized(int switch_index, char curve);

#define SNLEN 5
#define SNSIZE 2
#define SNREQUEST 0xc0
#define MULTI_SNREQUEST 0x80
typedef struct SensorData {
	char lo;
	char hi;
} SensorData;

void sensor_reset(SensorData*);
void sensor_request(unsigned int module_id);
void sensor_request_upto(unsigned int module_id);


#endif
