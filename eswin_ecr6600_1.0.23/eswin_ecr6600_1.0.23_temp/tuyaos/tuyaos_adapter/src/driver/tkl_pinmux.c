#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "chip_pinmux.h"
#include "tkl_pinmux.h"
#include "tkl_pwm.h"
#include "hal_pwm.h"
#include "oshal.h"
#include "gpio.h"

extern int pwm_set_gpio_used(unsigned int pin);
extern int pwm_find_channel(unsigned int pin, unsigned int *ch_num, unsigned int *used_gpio);
extern int adc_find_port_ch(UINT32_T adc_gpio_num, UINT32_T *adc_num, UINT32_T *adc_ch); 

OPERATE_RET tkl_io_pinmux_config(TUYA_PIN_NAME_E pin, TUYA_PIN_FUNC_E pin_func)
{
    int ret = OPRT_OK;

    if (pin_func >= TUYA_PWM0 && pin_func <= TUYA_PWM5) {
        ret = pwm_set_gpio_used(pin);
    }

    // PIN_FUNC_SET(pin, pin_func);
    return ret;
}

INT32_T tkl_io_pin_to_func(UINT32_T pin, TUYA_PIN_TYPE_E pin_type)
{
    unsigned int channel_num = 0;
    unsigned int used_gpio = 0;
    unsigned int adc_num = 0;
    unsigned int adc_ch = 0;
    int cfg_ret = 0;

    switch(pin_type) {
        case TUYA_IO_TYPE_PWM:
            cfg_ret = pwm_find_channel(pin, &channel_num, &used_gpio);
            if (cfg_ret != OPRT_OK) {
                os_printf(LM_APP, LL_ERR, "pwm_find_channel() err, cfg->pin is %d \r\n", pin);
                return OPRT_OS_ADAPTER_INVALID_PARM;
            }
            break;
        case TUYA_IO_TYPE_ADC:
            cfg_ret = adc_find_port_ch(pin, &adc_num, &adc_ch);
            if (cfg_ret != OPRT_OK) {
                os_printf(LM_APP, LL_ERR, "adc_find_port_ch() err, cfg->pin is %d \r\n", pin);
                return OPRT_OS_ADAPTER_INVALID_PARM;
            }
            adc_ch = adc_ch;
            adc_num = adc_num << 8;
            channel_num = adc_ch + adc_num;
            break;
        default:
            return OPRT_NOT_SUPPORTED;
    }

    return channel_num;
}