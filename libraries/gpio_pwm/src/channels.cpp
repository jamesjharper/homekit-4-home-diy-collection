#include "channels.h"

ledc_channel_config_t *PwmChannel::channel_list[LEDC_CHANNEL_MAX][LEDC_SPEED_MODE_MAX]={};
ledc_timer_config_t *ChannelTimerConfig::timer_list[LEDC_TIMER_MAX][LEDC_SPEED_MODE_MAX]={};