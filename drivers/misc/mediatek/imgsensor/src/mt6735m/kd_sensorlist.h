/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

//s_add new sensor driver here
//export funtions


/* Main Camera*/
UINT32 GC8024MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc);
UINT32 S5K4H8_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc);
UINT32 GC5005MIPI_SUB_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc);
UINT32 GC5025_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc);
UINT32 SP5506_SUB_MIPISensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc);
//! Add Sensor Init function here
//! Note:
//! 1. Add by the resolution from ""large to small"", due to large sensor
//!    will be possible to be main sensor.
//!    This can avoid I2C error during searching sensor.
//! 2. This file should be the same as mediatek\custom\common\hal\imgsensor\src\sensorlist.cpp
ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT kdSensorList[MAX_NUM_OF_SUPPORT_SENSOR+1] =
{

/* GC */
#if defined(GC8024_MIPI_RAW)
    {GC8024MIPI_SENSOR_ID, SENSOR_DRVNAME_GC8024MIPI_RAW,GC8024MIPI_RAW_SensorInit},
#endif

#if defined(S5K4H8_MIPI_RAW)
    {S5K4H8_SENSOR_ID,SENSOR_DRVNAME_S5K4H8_MIPI_RAW,S5K4H8_MIPI_RAW_SensorInit},
#endif
#if defined(GC5005_SUB_MIPI_RAW)
    {GC5005MIPI_SUB_SENSOR_ID, SENSOR_DRVNAME_GC5005MIPI_SUB_RAW,GC5005MIPI_SUB_RAW_SensorInit},
#endif

#if defined(GC5025_MIPI_RAW)
    {GC5025_SENSOR_ID, SENSOR_DRVNAME_GC5025_MIPI_RAW,GC5025_MIPI_RAW_SensorInit},
#endif

#if defined(SP5506_SUB_MIPI_RAW)
    {SP5506_SUB_MIPI_SENSOR_ID,SENSOR_DRVNAME_SP5506_SUB_MIPI_RAW,SP5506_SUB_MIPISensorInit},
#endif


/*  ADD sensor driver before this line */
    {0,{0},NULL}, //end of list
};
//e_add new sensor driver here

