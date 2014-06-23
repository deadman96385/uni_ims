// 
// Spreadtrum Auto Tester
//
// anli   2012-12-03
//
#ifndef _FM_20121203_H__
#define _FM_20121203_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_fm {
//-----------------------------------------------------------------------------
    
int fmOpen( void );

int fmPlay( uint freq );

int fmStop( void );

int fmClose( void );

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _FM_20121203_H__
