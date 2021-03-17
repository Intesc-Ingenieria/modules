
#include "py/runtime.h"
#include "py/obj.h"
#include "ports/stm32/mphalport.h"        
#include "i2c.h"

#define MPU6050_OPHYRA_ADDRESS      (104)
#define I2C_TIMEOUT_MS              (50)

#define POWER_MANAG_REG             (107)
#define GYR_CONFIG_REG              (27)
#define ACCEL_CONFIG_REG            (28)
#define ACCEL_REG_X                 (59)
#define ACCEL_REG_Y                 (61)
#define ACCEL_REG_Z                 (63)
#define TEMP_REG                    (65)
#define GYR_REG_X                   (67)
#define GYR_REG_Y                   (69)
#define GYR_REG_Z                   (71)

typedef struct _mpu60_class_obj_t{
    mp_obj_base_t base;
    int16_t env_gyr;
    int16_t env_accel;
    int16_t tem;
    float g;
    float sen;

    //INICIALIZAR AQUI EL I2C O EN EL CONSTRUCTOR?
} mpu60_class_obj_t;

const mp_obj_type_t mpu60_class_type;
        //PONER OBJETO GLOBAL??? (como en accel.c?)

STATIC void mpu60_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    (void)kind;
    mp_print_str(print, "mpu60_class()");
}

STATIC mp_obj_t mpu60_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    //Verificar numero de argumentos?
    mpu60_class_obj_t *self = m_new_obj(mpu60_class_obj_t);
    self->base.type = &mpu60_class_type;

    self->env_gyr = 0;
    self->env_accel = 0;
    self->tem = 340;
    self->g = 0;
    self->sen = 0;    

    i2c_init(I2C1, MICROPY_HW_I2C1_SCL, MICROPY_HW_I2C1_SDA, 400000, I2C_TIMEOUT_MS);

    return MP_OBJ_FROM_PTR(self);
}
                            //Agregar el self_in??
STATIC mp_obj_t init_function(mp_obj_t self_in, mp_obj_t range_accel_config_obj, mp_obj_t range_gyr_config_obj) {
    mpu60_class_obj_t * miMpu_ptr = MP_OBJ_TO_PTR(self_in);
    //mpu60_class_type->

    int miRangoAccel = mp_obj_get_int(range_accel_config_obj);
    int miRangoGyr = mp_obj_get_int(range_gyr_config_obj);

    if(miRangoAccel == 2){
        miMpu_ptr->env_accel = 0;
        miMpu_ptr->g = 16384;
    }
    else if(miRangoAccel == 4){
        miMpu_ptr->env_accel = 8;
        miMpu_ptr->g = 8192;        
    }
    else if(miRangoAccel == 8){
        miMpu_ptr->env_accel = 16;
        miMpu_ptr->g = 4096;        
    }    
    else{
        miMpu_ptr->env_accel = 24;
        miMpu_ptr->g = 2048;        
    }    

    if(miRangoGyr == 250){
        miMpu_ptr->env_gyr = 0;
        miMpu_ptr->sen = 131;          
    }
    else if(miRangoGyr == 500){
        miMpu_ptr->env_gyr = 8;
        miMpu_ptr->sen = 65.5;          
    }
    else if(miRangoGyr == 1000){
        miMpu_ptr->env_gyr = 16;
        miMpu_ptr->sen = 32.8;          
    }
    else{
        miMpu_ptr->env_gyr = 24;
        miMpu_ptr->sen = 16.4;         
    }

    //Verificar estas lineas de write
        //cual I2C, Address mpu,        dato, mem, ????
    uint8_t data1[2] = {POWER_MANAG_REG, 0};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data1, 2, true);

    uint8_t data2[2] = {ACCEL_CONFIG_REG, (uint8_t)(miMpu_ptr->env_accel)};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data2, 2, true);
    
    uint8_t data3[2] = {GYR_CONFIG_REG, (uint8_t)(miMpu_ptr->env_gyr)};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data3, 2, true);

    return mp_obj_new_float(1);
}

STATIC mp_obj_t read_axis(int axis, float g_o_sin){
    uint8_t myAxis[1] = {axis};
    uint8_t lectura_bytes[2];
                          
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, myAxis, 1, false);    
                                            //Parece que leo 2 bytes de informacion por eje.    
    i2c_readfrom(I2C1, MPU6050_OPHYRA_ADDRESS, lectura_bytes, 2, true);

    uint16_t miValorRes = lectura_bytes[0] << 8;
    uint16_t valTemp = (uint16_t)lectura_bytes[1];
    miValorRes = miValorRes + valTemp;

    if(miValorRes > 32767){
        miValorRes = (65536 - miValorRes)*-1;
        return mp_obj_new_float(miValorRes/g_o_sin);
    }
    else{
        return mp_obj_new_float(miValorRes/g_o_sin);
    }     
}

STATIC mp_obj_t get_accelerationX(mp_obj_t self_in) {
    mpu60_class_obj_t * miMpu_ptr = MP_OBJ_TO_PTR(self_in);
    return read_axis(ACCEL_REG_X, miMpu_ptr->g);
}

STATIC mp_obj_t get_accelerationY(mp_obj_t self_in) {
    mpu60_class_obj_t * miMpu_ptr = MP_OBJ_TO_PTR(self_in);
    return read_axis(ACCEL_REG_Y, miMpu_ptr->g);
}

STATIC mp_obj_t get_accelerationZ(mp_obj_t self_in) {
    mpu60_class_obj_t * miMpu_ptr = MP_OBJ_TO_PTR(self_in);
    return read_axis(ACCEL_REG_Z, miMpu_ptr->g);
}

STATIC mp_obj_t get_temperature(mp_obj_t self_in) {
    mpu60_class_obj_t * miMpu_ptr = MP_OBJ_TO_PTR(self_in);
    //uint o int???
    uint8_t registro_temp[1] = {(uint8_t)TEMP_REG};
    uint8_t lectura_temperatura[2];

    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, registro_temp, 1, false);    
    i2c_readfrom(I2C1, MPU6050_OPHYRA_ADDRESS, lectura_temperatura, 2, true);

    uint16_t miTempLeida = lectura_temperatura[0] << 8;
    uint16_t valTemp = (uint16_t)lectura_temperatura[1];
    miTempLeida = miTempLeida + valTemp;

    float miTemperatura_calculada = ((miTempLeida/(miMpu_ptr->tem)) + 36.53)/10;

    return mp_obj_new_float(miTemperatura_calculada);
}

STATIC mp_obj_t get_gyroscopeX(mp_obj_t self_in) {
    mpu60_class_obj_t * miMpu_ptr = MP_OBJ_TO_PTR(self_in);
    return read_axis(GYR_REG_X, miMpu_ptr->sen);
}

STATIC mp_obj_t get_gyroscopeY(mp_obj_t self_in) {
    mpu60_class_obj_t * miMpu_ptr = MP_OBJ_TO_PTR(self_in);
    return read_axis(GYR_REG_Y, miMpu_ptr->sen);
}

STATIC mp_obj_t get_gyroscopeZ(mp_obj_t self_in) {
    mpu60_class_obj_t * miMpu_ptr = MP_OBJ_TO_PTR(self_in);
    return read_axis(GYR_REG_Z, miMpu_ptr->sen);
}

STATIC mp_obj_t write_function(mp_obj_t self_in, mp_obj_t num_obj, mp_obj_t address_obj) {
    //uint8_t data3[1] = {GYR_CONFIG_REG, (uint8_t)(miMpu_ptr->env_gyr)};
    //i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data3, 2, true);

    //return mp_obj_new_int(mp_hal_pin_read(pin_D3));
    return 0;
}

STATIC mp_obj_t read_function(mp_obj_t self_in, mp_obj_t address_obj) {
    //return mp_obj_new_int(mp_hal_pin_read(pin_D3));
    return 0;
};

MP_DEFINE_CONST_FUN_OBJ_3(init_function_obj, init_function);
MP_DEFINE_CONST_FUN_OBJ_1(get_accelerationX_obj, get_accelerationX);
MP_DEFINE_CONST_FUN_OBJ_1(get_accelerationY_obj, get_accelerationY);
MP_DEFINE_CONST_FUN_OBJ_1(get_accelerationZ_obj, get_accelerationZ);
MP_DEFINE_CONST_FUN_OBJ_1(get_temperature_obj, get_temperature);
MP_DEFINE_CONST_FUN_OBJ_1(get_gyroscopeX_obj, get_gyroscopeX);
MP_DEFINE_CONST_FUN_OBJ_1(get_gyroscopeY_obj, get_gyroscopeY);
MP_DEFINE_CONST_FUN_OBJ_1(get_gyroscopeZ_obj, get_gyroscopeZ);
MP_DEFINE_CONST_FUN_OBJ_3(write_function_obj, write_function);
MP_DEFINE_CONST_FUN_OBJ_2(read_function_obj, read_function);

STATIC const mp_rom_map_elem_t mpu60_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&init_function_obj) },
    { MP_ROM_QSTR(MP_QSTR_accX), MP_ROM_PTR(&get_accelerationX_obj) },
    { MP_ROM_QSTR(MP_QSTR_accY), MP_ROM_PTR(&get_accelerationY_obj) },
    { MP_ROM_QSTR(MP_QSTR_accZ), MP_ROM_PTR(&get_accelerationZ_obj) },
    { MP_ROM_QSTR(MP_QSTR_temp), MP_ROM_PTR(&get_temperature_obj) },
    { MP_ROM_QSTR(MP_QSTR_gyrX), MP_ROM_PTR(&get_gyroscopeX_obj) },
    { MP_ROM_QSTR(MP_QSTR_gyrY), MP_ROM_PTR(&get_gyroscopeY_obj) },
    { MP_ROM_QSTR(MP_QSTR_gyrZ), MP_ROM_PTR(&get_gyroscopeZ_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&write_function_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&read_function_obj) },
};
                                
STATIC MP_DEFINE_CONST_DICT(mpu60_class_locals_dict, mpu60_class_locals_dict_table);

const mp_obj_type_t mpu60_class_type = {
    { &mp_type_type },
    .name = MP_QSTR_ophyra_mpu60,
    .print = mpu60_class_print,
    .make_new = mpu60_class_make_new,
    .locals_dict = (mp_obj_dict_t*)&mpu60_class_locals_dict,
};

STATIC const mp_rom_map_elem_t ophyra_mpu60_globals_table[] = {
                                                    //Nombre del archivo (User C module)
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ophyra_mpu60) },
            //Nombre de la clase        //Nombre del "tipo" asociado.
    { MP_ROM_QSTR(MP_QSTR_MPU6050), MP_ROM_PTR(&mpu60_class_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ophyra_botones_globals, ophyra_mpu60_globals_table);

// Define module object.
const mp_obj_module_t ophyra_mpu60_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ophyra_botones_globals,
};

// Registro del modulo para hacerlo disponible para Python.
//                      nombreArchivo, nombreArchivo_user_cmodule, MODULE_IDENTIFICADOR_ENABLED
MP_REGISTER_MODULE(MP_QSTR_ophyra_mpu60, ophyra_mpu60_user_cmodule, MODULE_OPHYRA_MPU60_ENABLED);

