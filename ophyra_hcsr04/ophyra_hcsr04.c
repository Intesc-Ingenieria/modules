/*
    ophyra_hcsr04.c

    Modulo de usuario en C para la utilizacion del sensor HCSR04 la tarjeta Ophyra, fabricada por
    Intesc Electronica y Embebidos.

    Este archivo incluye la definicion de una clase con 4 funciones, que sirven para hacer funcionar el
    sensor ultrasonico dando como resultado la longitud entre el sensor y un obstaculo que tenga frente
    en Centimetros y Metros.

    Para construir el firmware incluyendo este modulo, ejecutar en Cygwin:
        make BOARD=OPHYRA USER_C_MODULES=../../../modules CFLAGS_EXTRA=-DMODULE_OPHYRA_HCSR04_ENABLED=1 all

        En USER_C_MODULES se especifica el path hacia la localizacion de la carpeta modules, donde se encuentra
        el archivo ophyra_botones.c y micropython.mk

    Escrito por: Carlos D. Hernández.
    Ultima fecha de modificacion: 25/03/2021.

*/

#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"          //Para el uso de la funcion mp_hal_pin_config
#include "extmod/machine_pulse.h"


typedef struct _hcsr04_class_obj_t{
    mp_obj_base_t base;
    const pin_obj_t pin_trigger;
    const pin_obj_t pin_echo;
    uint16_t echo_timeout;

} hcsr04_class_obj_t;

const mp_obj_type_t hcsr04_class_type;
STATIC hcsr04_class_obj_t mi_hcsr04;
//Funcion print
STATIC void hcsr04_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    (void)kind;
    mp_print_str(print, "hcsr04_class()");
}

/*
    make_new: Constructor de la clase. Esta funcion se invoca cuando el usuario de Micropython escribe:
        HCSR04()
    Este objeto de tipo constructor recibira tres argumentos los cuales son:
        -> pin_trigger
        -> pin_echo
        -> echo_timeout
    de esta manera estos objetos seran utilizados para poder transmitir las señales ultrasonicas por medio del sensor
*/
STATIC mp_obj_t hcsr04_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 3, 3, false);
    mp_obj_t source_trigger=args[0];
    mp_obj_t source_echo=args[1];
    mp_obj_t source_echo_timeout=args[2];
    mi_hcsr04->*pin_trigger=pin_find(source_trigger);
    mi_hcsr04->*pin_echo=pin_find(source_echo);
    mi_hcsr04->echo=mp_obj_get_int(source_echo_timeout);
    mi_hcsr04->echo_timeout=mp_obj_get_int(echo_timeout);
    mp_hal_pin_config(mi_hcsr04->pin_trigger, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0);
    mp_hal_pin_write(mi_hcsr04->pin_trigger, 0);
    mp_hal_pin_config(mi_hcsr04->pin_echo, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0);

    
    return MP_OBJ_FROM_PTR(&mi_hcsr04);
}

/*
    send_pulse_and_wait es una funcion definida de uso interno en la cual se utiliza para emitir las señales ultrasonicas,
    de esta manera se definen tiempos de espera en microsegundos especificos de espera en los que el sensor puede emitir una señal
    y esta pueda ser capturada a traves de una funcion machine_time_pulse_us()
*/
STATIC mp_obj_t send_pulse_and_wait(mp_obj_t self_in) {
    hcsr04_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_hal_pin_write(self->pin_trigger,0);
    mp_hal_delay_us(5);
    mp_hal_pin_write(self->pin_trigger,1);
    mp_hal_delay_us(10);
    mp_hal_pin_write(self->pin_trigger,0);
    uint8_t pulse_time=machine_time_pulse_us(self->pin_echo,1,self->echo_timeout);
    if(pulse_time!=0)
        return mp_obj_new_int(pulse_time);
    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Out of range.\n"));

}

/*
    Funcion distance_mm() esta nos sirve para calcular a traves del tiempo em microsegundos que se tarda el sensor
    a mm de acuerdo con el datasheet 0.34320 mm/us -> entonces tenemos que 1 milimetro equivale a 2.91 us * 2 es igual
    5.82us asi evitamos dividir sobre 2.
*/

STATIC mp_obj_t distance_mm(mp_obj_t self_in) {
    hcsr04_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int pulse_time=mp_obj_get_int(send_pulse_and_wait(self));
    uint8_t mm =(uint8_t)pulse_time*100/582;
    return mp_obj_new_int(mm);
}

/*
    Funcion distance_cm() esta nos sirve para calcular a traves del tiempo em microsegundos que se tarda el sensor
    a cm de acuerdo con el datasheet 0.034320 cm/us -> entonces tenemos que 1 milimetro equivale a 29.1 us
    se tiene que dividir sobre 2 debido a que el pulso recorre la distancia 2 veces 
*/

STATIC mp_obj_t distance_cm(mp_obj_t self_in) {
    hcsr04_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int pulse_time=mp_obj_get_int(send_pulse_and_wait(self));
    float cms=((uint8_t)pulse_time/2)/29.1;
    return mp_obj_new_float(cms);
};

//Se asocian las funciones arriba escritas con su correspondiente objeto de funcion para Micropython.
MP_DEFINE_CONST_FUN_OBJ_1(distance_mm_obj, distance_mm);
MP_DEFINE_CONST_FUN_OBJ_1(distance_cm_obj, distance_cm);

/*
    Se asocia el objeto de funcion de Micropython con cierto string, que sera el que se utilice en la
    programacion en Micropython. Ej: Si se escribe:
        HCSR04.distance_mm()
    Internamente se llama al objeto de funcion distance_mm, que esta asociado con la funcion
    distance_mm, que retorna el valor de longitud entre el obstaculo y el sensor.
*/
STATIC const mp_rom_map_elem_t hcsr04_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance_mm), MP_ROM_PTR(&distance_mm_obj) },
    { MP_ROM_QSTR(MP_QSTR_distance_cm), MP_ROM_PTR(&distance_cm_obj) },
    //Nombre de la func. que se va a invocar en Python     //Pointer al objeto de la func. que se va a invocar.
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
                                                    //Nombre del archivo (User C module)
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ophyra_hcsr04) },
            //Nombre de la clase        //Nombre del "tipo" asociado.
    { MP_ROM_QSTR(MP_QSTR_HCSR04), MP_ROM_PTR(&hcsr04_class_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ophyra_hcsr04_globals, ophyra_hcsr04_globals_table);

// Define module object.
const mp_obj_module_t ophyra_hcsr04_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ophyra_hcsr04_globals,
};

// Registro del modulo para hacerlo disponible para Python.
//                      nombreArchivo, nombreArchivo_user_cmodule, MODULE_IDENTIFICADOR_ENABLED
MP_REGISTER_MODULE(MP_QSTR_ophyra_hcsr04, ophyra_hcsr04_user_cmodule, MODULE_OPHYRA_HCSR04_ENABLED);

