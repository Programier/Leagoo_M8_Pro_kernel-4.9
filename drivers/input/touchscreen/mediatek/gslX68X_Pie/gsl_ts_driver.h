#ifndef __GSL_TS_DRIVER_H__
#define __GSL_TS_DRIVER_H__
/*********************************/
#define TPD_HAVE_BUTTON		//virtual key
#define GSL_ALG_ID		//id suanfa
#define GSL_DEBUG			//debug
#define TPD_PROC_DEBUG		//adb debug
#define GSL_TIMER				//dingshiqi
#define GSL_GESTURE

#define GSL9XX_VDDIO_1800		1
#define TPD_DEBUG_TIME	0x20130424
struct gsl_touch_info
{
	int x[10];
	int y[10];
	int id[10];
	int finger_num;
};

struct gsl_ts_data {
	struct i2c_client *client;
	struct workqueue_struct *wq;
	struct work_struct work;
	unsigned int irq;
	//struct early_suspend pm;
};

/*button*/
#define TPD_KEY_COUNT           3
#define TPD_KEYS                {KEY_MENU,KEY_HOMEPAGE,KEY_BACK}
#ifdef  TPD_KEY_REVERSE
#define TPD_KEYS_DIM            {{400,900,120,80},{240,900,120,80},{80,900,120,80}}
#else
	#if defined(DROI_PRO_FQ5CW_ZGW5) || defined(DROI_PRO_F6_YT) || defined(DROI_PRO_PF5T_ZXHL) || defined(DROI_PRO_F6_NYX) || defined(DROI_PRO_PF5_ZXHL)
		#define TPD_KEYS_DIM            {{120,1380,120,80},{360,1380,120,80},{600,1380,120,80}}
	#else
        //#define TPD_KEYS_DIM            {{80,900,120,80},{240,900,120,80},{400,900,120,80}}
        #define TPD_KEYS_DIM            {{100,1330,120,80},{400,1330,120,80},{600,1330,120,80}}
    #endif
#endif



#ifdef GSL_ALG_ID
extern unsigned int gsl_mask_tiaoping(void);
extern unsigned int gsl_version_id(void);
extern void gsl_alg_id_main(struct gsl_touch_info *cinfo);
extern void gsl_DataInit(int *ret);
#endif

/* Fixme mem Alig */
#include "gsl_fw_leagoo3.h"
static unsigned char gsl_cfg_index = 0;

struct fw_config_type
{
	const struct fw_data *fw;
	unsigned int fw_size;
	unsigned int *data_id;
	unsigned int data_size;
};
static struct fw_config_type gsl_cfg_table[9] = {
/*0*/{GSLX680_FW,(sizeof(GSLX680_FW)/sizeof(struct fw_data)),
	gsl_config_data_id,(sizeof(gsl_config_data_id)/4)},
};

#endif
