#include "hal_adc.h"
#include "psm_system.h"

#define CONFIG_WIFI_VOLTAGE_THRESHOLD_VALUE 3300000 //3.3v
#define CONFIG_WIFI_SLEEP_AND_CHARGE_TIME   102400  //us

extern void txl_store_unsent_tx(void);
extern void txl_restore_unsent_tx(void);
extern unsigned int system_irq_save_psm(void);
extern void system_irq_restore_psm(unsigned int psw);

static void (*wifi_ps_callback)(void) = NULL;

int wifi_ps_enter(void)
{
    txl_store_unsent_tx();

    return 0;
}

int wifi_ps_exit(void)
{
    txl_restore_unsent_tx();

    return 0;
}

void wifi_ps_set_callback(void (*callback)(void))
{
    wifi_ps_callback = callback;
}

void wifi_ps_execute_callback(void)
{
    if (wifi_ps_callback)
        wifi_ps_callback();
}

static void single_firewire_callback(void)
{
    int voltage;
    unsigned int psw;

    while (1)
    {
        voltage = hal_adc_get_single(DRV_ADC_A_INPUT_VREF_BUFFER, DRV_ADC_B_INPUT_GPIO_0, DRV_ADC_EXPECT_MAX_VOLT3);
           if (voltage >= CONFIG_WIFI_VOLTAGE_THRESHOLD_VALUE)
        {
            break;
        }
        else
        {
            psw = system_irq_save_psm();
            psm_light_sleep(CONFIG_WIFI_SLEEP_AND_CHARGE_TIME);
            system_irq_restore_psm(psw);
        }
    }
}

void wifi_ps_init(void)
{
    wifi_ps_set_callback(single_firewire_callback);
}

