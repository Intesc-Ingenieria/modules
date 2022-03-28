/**
 * @file kairos_AK8975.c
 * @author Ana María Ruiz Fernández
 * @brief C usermod para usar el sensor MPU6050 incluido en la tarjeta Kairos,
 * elaborada por Intesc Electrónica y Embebidos, ubicados en Puebla, Pue. Mexico.
 * @version 0.1
 * @date 2022-03-24
 */


#include "py/runtime.h"
#include "py/obj.h"
#include "ports/stm32/mphalport.h"
#include "i2c.h"

#define I2C_TIMEOUT_MS              (50)

#define MPU6050_ADDRESS             (104)
#define MPU6050_WHO_AM_I            (117)
#define MPU6050_BYPASS_REG          (55)
#define MPU6050_FREQ_REG            (107)

#define AK8975_ADDRESS              (12)
#define AK8975_MODE_REG             (10)
#define AK8975_X_REG                (3)
#define AK8975_Y_REG                (5)


typedef struct _ak8975_class_obj 
{
    mp_obj_base_t base;
} ak8975_class_obj_t;

const mp_obj_type_t ak8975_class_type;

/**
 * @brief Imprime información sobre la clase.
 * 
 * @return STATIC 
 */
STATIC void ak8975_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)kind;
    mp_print_str(print, "ak8975_class()");
}

/**
 * @brief Creador para la clase AK8975
 * 
 * @param type 
 * @param n_args Debe ser 0, ya que esta clase no toma argumentos
 * @param n_kw 
 * @param args 
 * @return STATIC 
 */
STATIC mp_obj_t ak8975_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) 
{
    ak8975_class_obj_t *self = m_new_obj(ak8975_class_obj_t);

    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    self->base.type = &ak8975_class_type;
   
    return MP_OBJ_FROM_PTR(self);
}

/**
 * @brief Inicializador para el magnetometro. Se encarga de configurar el I2C.
 * 
 * @return STATIC 
 */
STATIC mp_obj_t init_func() 
{
    // inicialización del Puerto 1 para la comunicación I2C
    i2c_init(I2C1, MICROPY_HW_I2C1_SCL, MICROPY_HW_I2C1_SDA, 100000, I2C_TIMEOUT_MS);    
    
    // comprobación de la disponibilidad del MPU
    uint8_t _whoami[2] = { MPU6050_WHO_AM_I };
    i2c_writeto(I2C1, MPU6050_ADDRESS, _whoami, 1, false);
    i2c_readfrom(I2C1, MPU6050_ADDRESS, _whoami, 1, true);

    // se marca un error si el MPU no se encuentra
    if(_whoami[0] != 0x68)
    {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("No se encontró el MPU6050.\n"));
    }

    // activación del bypass
    uint8_t bypass[2] = { MPU6050_BYPASS_REG, 2 };
    i2c_writeto(I2C1, MPU6050_ADDRESS, bypass, 2, true);

    // despertar al sensor y configurar la frecuencia a 8MHz 
    uint8_t freq[2] = { MPU6050_FREQ_REG, 0 };
    i2c_writeto(I2C1, MPU6050_ADDRESS, freq, 2, true);

    return mp_const_none;
}


/**
 * @brief Obtiene la medición del magnetómetro para el eje dado.
 * 
 * @param axis Registro del eje a medir.
 * @return STATIC Floar que corresponde a la medida calculada para el eje dado.
 */
STATIC mp_obj_t read_axis(int axis)
{
    uint8_t this_axis[1] = { (uint8_t)axis };
    uint8_t data[2] = { AK8975_MODE_REG, 1 };

    // configurar el sensor al modo de operación Single Measurement
    i2c_writeto(I2C1, AK8975_ADDRESS, data, 2, true);

    // agregar un delay de 10 ms para permitir que el sensor capture los datos
    HAL_Delay(10);

    // leer los datos y almacenar en dos bytes
    i2c_writeto(I2C1, AK8975_ADDRESS, this_axis, 1, false);
    i2c_readfrom(I2C1, AK8975_ADDRESS, data, 2, true);

    // concatenar los bytes más y menos significativos
    int16_t measurement = (int16_t)(data[1] << 8 | data[0]);

    // dividir el eje en +180° y -180°
    if(measurement > +32768)
    {
        measurement -= 65536;
    }

    return mp_obj_new_float((float)(measurement));
}

/**
 * @brief Obtiene la medición del sensor para el eje x. Esta función se invoca
 * cuando el usuario escribe:
 *      x = AK8975.get_x()
 * 
 * @return STATIC. Float que corresponde a la medición en el eje x.
 */
STATIC mp_obj_t getX() 
{
    return read_axis(AK8975_X_REG);
}


/**
 * @brief Obtiene la medición del sensor para el eje y. Esta función se invoca
 * cuando el usuario escribe:
 *      y = AK8975.get_y()
 * 
 * @return STATIC. Float que corresponde a la medición en el eje y.
 */
STATIC mp_obj_t getY() 
{
    return read_axis(AK8975_Y_REG);
}

// objetos función asociados a las funciones de esta clase
MP_DEFINE_CONST_FUN_OBJ_0(init_func_obj, init_func);
MP_DEFINE_CONST_FUN_OBJ_0(getX_obj, getX);
MP_DEFINE_CONST_FUN_OBJ_0(getY_obj, getY);

// associación de los objetos función con sus funciones en python
STATIC const mp_rom_map_elem_t ak8975_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&init_func_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_x), MP_ROM_PTR(&getX_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_y), MP_ROM_PTR(&getY_obj) },
    // Nombre de la función que   // Nombre de la función en C 
    // se invocará en Micropython
};

STATIC MP_DEFINE_CONST_DICT(ak8975_class_locals_dict, ak8975_class_locals_dict_table);

const mp_obj_type_t ak8975_class_type = {
    { &mp_type_type },
    .name = MP_QSTR_kairos_AK8975,
    .print = ak8975_class_print,
    .make_new = ak8975_class_make_new,
    .locals_dict = (mp_obj_dict_t*)&ak8975_class_locals_dict,
};

STATIC const mp_rom_map_elem_t kairos_ak8975_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_kairos_AK8975) },
                                                // Nombre este archivo
    { MP_ROM_QSTR(MP_QSTR_AK8975), MP_ROM_PTR(&ak8975_class_type) },
    // Nombre de la clase en Micropython    // Nombre del tipo asociado
};

STATIC MP_DEFINE_CONST_DICT(mp_module_kairos_ak8975_globals, kairos_ak8975_globals_table);

const mp_obj_module_t kairos_AK8975_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_kairos_ak8975_globals,
};

// Registro del módulo para que esté disponible en MicroPython
//                    nombre del módulo, nombre del archivo en C, identificador del activador de modulo
MP_REGISTER_MODULE(MP_QSTR_kairos_AK8975, kairos_AK8975_user_cmodule, MODULE_KAIROS_AK8975_ENABLED);