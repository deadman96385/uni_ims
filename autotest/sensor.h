// 
// Spreadtrum Auto Tester
//
// anli   2013-01-23
//
#ifndef _SENSOR_20130123_H__
#define _SENSOR_20130123_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_snsr {
//-----------------------------------------------------------------------------
#ifndef SENSOR_TYPE_ACCELEROMETER

#define SENSOR_TYPE_ACCELEROMETER       1
#define SENSOR_TYPE_MAGNETIC_FIELD      2
#define SENSOR_TYPE_GYROSCOPE           4
#define SENSOR_TYPE_LIGHT               5
#define SENSOR_TYPE_PROXIMITY           8


#endif // SENSOR_TYPE_ACCELEROMETER

int sensorOpen( void );

int sensorActivate( int type );

int sensorClose( void );
//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _SENSOR_20130123_H__
