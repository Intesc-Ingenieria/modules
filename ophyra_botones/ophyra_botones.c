/*
    ophyra_botones.c

    C usermod to use the four general-purpose buttons on the Ophyra board, manufactured by
    Intesc Electronica y Embebidos, located in Puebla, Pue. Mexico.

    This file includes the definition of a class with four functions. Each of them returns the state of the
    corresponding pin, in which there is the specified button.    

    To build the Micropython firmware for the Ophyra board including this C usermod, use:
        make BOARD=OPHYRA USER_C_MODULES=../../../modules CFLAGS_EXTRA=-DMODULE_OPHYRA_BOTONES_ENABLED=1 all

        In USER_C_MODULES the path to the "modules" folder is specified. Inside of it, there is the folder
        "ophyra_botones", which contains the ophyra_botones.c and micropython.mk files.

    Written by: Jonatan Salinas.
    Last modification: 04/03/2021.

*/

#include "py/runtime.h"
#include "py/obj.h"
#include "ports/stm32/mphalport.h"          //To use the function mp_hal_pin_config

typedef struct _buttons_class_obj_t{
    mp_obj_base_t base;
} buttons_class_obj_t;

const mp_obj_type_t buttons_class_type;

//Print function
STATIC void buttons_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    (void)kind;
    mp_print_str(print, "buttons_class()");
}

/*
    make_new: Class constructor. This function is invoked when the Micropython user writes:
        sw()
*/
STATIC mp_obj_t buttons_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    buttons_class_obj_t *self = m_new_obj(buttons_class_obj_t);
    self->base.type = &buttons_class_type;

    //Configuration of the four pins, at which the buttons of the Ophyra board are attached, as INPUT and PULL_UP
    mp_hal_pin_config(pin_C2, MP_HAL_PIN_MODE_INPUT, MP_HAL_PIN_PULL_UP, 0);
    mp_hal_pin_config(pin_D5, MP_HAL_PIN_MODE_INPUT, MP_HAL_PIN_PULL_UP, 0);
    mp_hal_pin_config(pin_D4, MP_HAL_PIN_MODE_INPUT, MP_HAL_PIN_PULL_UP, 0);
    mp_hal_pin_config(pin_D3, MP_HAL_PIN_MODE_INPUT, MP_HAL_PIN_PULL_UP, 0);

    return MP_OBJ_FROM_PTR(self);
}

/*
    These four functions return the current state of a specific pin, in order to know if the button has been
    pressed or not. Returns 0 if the button is pressed, and 1 if not.
*/
STATIC mp_obj_t button0_pressed(mp_obj_t self_in) {
    return mp_obj_new_int(mp_hal_pin_read(pin_C2));
}

STATIC mp_obj_t button1_pressed(mp_obj_t self_in) {
    return mp_obj_new_int(mp_hal_pin_read(pin_D5));
}

STATIC mp_obj_t button2_pressed(mp_obj_t self_in) {
    return mp_obj_new_int(mp_hal_pin_read(pin_D4));
}

STATIC mp_obj_t button3_pressed(mp_obj_t self_in) {
    return mp_obj_new_int(mp_hal_pin_read(pin_D3));
};

//We associate the functions above with their corresponding Micropython function object.
MP_DEFINE_CONST_FUN_OBJ_1(button0_pressed_obj, button0_pressed);
MP_DEFINE_CONST_FUN_OBJ_1(button1_pressed_obj, button1_pressed);
MP_DEFINE_CONST_FUN_OBJ_1(button2_pressed_obj, button2_pressed);
MP_DEFINE_CONST_FUN_OBJ_1(button3_pressed_obj, button3_pressed);

/*
    Here, we associate the "function object" of Micropython with a specific string. This string is the one
    that will be used in the Micropython programming. For example, if we write:
        sw().sw1()
    Internally, in some way the function object "button0_pressed_obj" is called, which in fact is associated with
    the button0_pressed function, which return the value of the pin at which the button 1 is connected.
*/
STATIC const mp_rom_map_elem_t buttons_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_sw1), MP_ROM_PTR(&button0_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_sw2), MP_ROM_PTR(&button1_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_sw3), MP_ROM_PTR(&button2_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_sw4), MP_ROM_PTR(&button3_pressed_obj) },
    //Name of the function that         //Associated function object.
    //is going to be invoked in
    //Micropython     
};
                                
STATIC MP_DEFINE_CONST_DICT(buttons_class_locals_dict, buttons_class_locals_dict_table);

const mp_obj_type_t buttons_class_type = {
    { &mp_type_type },
    .name = MP_QSTR_ophyra_botones,
    .print = buttons_class_print,
    .make_new = buttons_class_make_new,
    .locals_dict = (mp_obj_dict_t*)&buttons_class_locals_dict,
};

STATIC const mp_rom_map_elem_t ophyra_botones_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ophyra_botones) },
                                                    //Name of this C usermode file
    { MP_ROM_QSTR(MP_QSTR_sw), MP_ROM_PTR(&buttons_class_type) },
    //sw: Name of the class         //Name of the associated "type".
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ophyra_botones_globals, ophyra_botones_globals_table);

// Define module object.
const mp_obj_module_t ophyra_botones_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ophyra_botones_globals,
};

// We need to register the module to make it available for Python.
//                          nameOfTheFile, nameOfTheFile_user_cmodule, MODULE_IDENTIFIER_ENABLED
MP_REGISTER_MODULE(MP_QSTR_ophyra_botones, ophyra_botones_user_cmodule, MODULE_OPHYRA_BOTONES_ENABLED);

