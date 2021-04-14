/*
    ophyra_hcsr04.c

    C usermod to use an external HCSR04 sensor with the Ophyra board, manufactured by
    Intesc Electronica y Embebidos, located in Puebla, Pue. Mexico.

    This file includes the definition of a class with some functions, that allow to operate the sensor.
    With these functions it is possible to get the length between the sensor and an obstacle, in centimeters
    and meters.

    To build the Micropython firmware for the Ophyra board including this C usermod, use:

        make BOARD=OPHYRA USER_C_MODULES=../../../modules CFLAGS_EXTRA=-DMODULE_OPHYRA_HCSR04_ENABLED=1 all

        In USER_C_MODULES the path to the "modules" folder is specified. Inside of it, there is the folder
        "ophyra_hcsr04", which contains the ophyra_hcsr04.c and micropython.mk files.

    Written by: Carlos D. Hernández.
    Last modification: 25/03/2021.

*/

#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"          //To make use of the function mp_hal_pin_config
#include "extmod/machine_pulse.h"
#include "py/mperrno.h"

/*
    Definition of the pins to use
*/
const pin_obj_t *pin_trigger;
const pin_obj_t *pin_echo;

typedef struct _hcsr04_class_obj_t{
    mp_obj_base_t base;
    uint16_t echo_timeout;

} hcsr04_class_obj_t;

const mp_obj_type_t hcsr04_class_type;

//Print function
STATIC void hcsr04_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    (void)kind;
    mp_print_str(print, "hcsr04_class()");
}

/*
    make_new: Class constructor. This function is invoked when the MicroPython user writes:
        HCSR04()
    This function will receive three arguments:
        -> pin_trigger
        -> pin_echo
        -> echo_timeout
    In this way, these objects will be used to transmit the ultrasonic signals using the sensor.
*/
STATIC mp_obj_t hcsr04_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 3, 3, false);
    hcsr04_class_obj_t *self = m_new_obj(hcsr04_class_obj_t);
    self->base.type = &hcsr04_class_type;
    mp_obj_t source_trigger=args[0];
    mp_obj_t source_echo=args[1];
    mp_obj_t source_echo_timeout=args[2];
    pin_trigger=pin_find(source_trigger);
    pin_echo=pin_find(source_echo);
    self->echo_timeout=mp_obj_get_int(source_echo_timeout);
    mp_hal_pin_config(pin_trigger, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0);
    mp_hal_pin_write(pin_trigger, 0);
    mp_hal_pin_config(pin_echo, MP_HAL_PIN_MODE_INPUT, MP_HAL_PIN_PULL_NONE, 0);

    
    return MP_OBJ_FROM_PTR(self);
}

/*
    send_pulse_and_wait is an internal function that emits the ultrasonic signals. Some time in microseconds is defined to adjust the
    trigger pulse time according to the operation of the sensor. The machine_time_pulse_us() allows us to obtain the echo pulse time.
    This last function, according to the documentation, can return 1 or 2 if the sensor catch an "out of range",
    that is why the exception OSError 110 is thrown when we get that values.
*/
STATIC mp_obj_t send_pulse_and_wait(mp_obj_t self_in) {
    hcsr04_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_hal_pin_write(pin_trigger,0);
    mp_hal_delay_us(5);
    mp_hal_pin_write(pin_trigger,1);
    mp_hal_delay_us(10);
    mp_hal_pin_write(pin_trigger,0);
    unsigned int pulse_time=machine_time_pulse_us(pin_echo,1,self->echo_timeout);
    if(pulse_time == 1 || pulse_time == 2)
    {
        mp_raise_OSError(MP_ETIMEDOUT);
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Out of range.\n"));
    }
    return mp_obj_new_int(pulse_time);

}

/*
    distance_mm()
    This function allows to calculate the distance between the sensor and an obstacle, in mm; using the time in us.
    According to the datasheet, 0.34320 mm/us -> then we can say that 1 mm is equal to 2.91 us, 2mm is equal to
    5.82us, so we avoid dividing by 2.
*/

STATIC mp_obj_t distance_mm(mp_obj_t self_in) {
    hcsr04_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int pulse_time=mp_obj_get_int(send_pulse_and_wait(self));
    int mm =pulse_time*100/582;
    return mp_obj_new_int(mm);
}

/*
    distance_cm()
    This function allows us to calculate the distance between the sensor and an obstacle, in cm; using the time in us.
    According to the datasheet, 0.034320 cm/us -> then we can say that 1 mm is equal to 29.1 us
    We must divide by 2 because the pulse travels the distance 2 times.
*/

STATIC mp_obj_t distance_cm(mp_obj_t self_in) {
    hcsr04_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int pulse_time=mp_obj_get_int(send_pulse_and_wait(self));
    float cms=(pulse_time/2)/29.1;
    return mp_obj_new_float(cms);
};

//We associate the functions above with their corresponding Micropython function object.
MP_DEFINE_CONST_FUN_OBJ_1(distance_mm_obj, distance_mm);
MP_DEFINE_CONST_FUN_OBJ_1(distance_cm_obj, distance_cm);

/*
    Here, we associate the "function object" of Micropython with a specific string. This string is the one
    that will be used in the Micropython programming. For example:
        HCSR04.distance_mm()
*/
STATIC const mp_rom_map_elem_t hcsr04_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance_mm), MP_ROM_PTR(&distance_mm_obj) },
    { MP_ROM_QSTR(MP_QSTR_distance_cm), MP_ROM_PTR(&distance_cm_obj) },
    //Name of the Micropython function     //Associated function object
};
                                
STATIC MP_DEFINE_CONST_DICT(hcsr04_class_locals_dict, hcsr04_class_locals_dict_table);

const mp_obj_type_t hcsr04_class_type = {
    { &mp_type_type },
    .name = MP_QSTR_ophyra_hcsr04,
    .print = hcsr04_class_print,
    .make_new = hcsr04_class_make_new,
    .locals_dict = (mp_obj_dict_t*)&hcsr04_class_locals_dict,
};

STATIC const mp_rom_map_elem_t ophyra_hcsr04_globals_table[] = {
                                                    //Name of this C usermod file
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ophyra_hcsr04) },
            //Name of the class        //Name of the associated "type"
    { MP_ROM_QSTR(MP_QSTR_HCSR04), MP_ROM_PTR(&hcsr04_class_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ophyra_hcsr04_globals, ophyra_hcsr04_globals_table);

// Define module object.
const mp_obj_module_t ophyra_hcsr04_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ophyra_hcsr04_globals,
};

// We need to register the module to make it available for Python.
//                          nameOfTheFile, nameOfTheFile_user_cmodule, MODULE_IDENTIFIER_ENABLED
MP_REGISTER_MODULE(MP_QSTR_ophyra_hcsr04, ophyra_hcsr04_user_cmodule, MODULE_OPHYRA_HCSR04_ENABLED);

