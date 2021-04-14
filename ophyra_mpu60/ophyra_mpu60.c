/*
    ophyra_mpu60.c

    C usermod to use the MPU6050 sensor included in the Ophyra board, manufactured by
    Intesc Electronica y Embebidos, located in Puebla, Pue. Mexico.

    This file includes the functions that read from the sensor registers the values of acceleration in the
    X, Y, and Z axis; the values of the gyroscope in the X, Y, and Z axis; and the temperature value in Celsius
    degrees. The "i2c_writeto" and "i2c_readfrom" functions are used for the communication with the sensor using
    the I2C protocol.

    To build the Micropython firmware for the Ophyra board including this C usermod, use:
        make BOARD=OPHYRA USER_C_MODULES=../../../modules CFLAGS_EXTRA=-DMODULE_OPHYRA_MPU60_ENABLED=1 all

        In USER_C_MODULES the path to the "modules" folder is specified. Inside of it, there is the folder
        "ophyra_mpu60", which contains the ophyra_mpu60.c and micropython.mk files.

    Written by: Jonatan Salinas y Carlos Daniel.
    Last modification: 21/03/2021.

*/

#include "py/runtime.h"
#include "py/obj.h"
#include "ports/stm32/mphalport.h"        
#include "i2c.h"

#define MPU6050_OPHYRA_ADDRESS      (104)
#define I2C_TIMEOUT_MS              (50)
        //Definition of the necessary sensor registers:
#define MPU60_WHO_AM_I_REG          (117)
#define MPU60_SMPLRT_DIV_REG        (25)
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
    float g;
    float sen;     
} mpu60_class_obj_t;

const mp_obj_type_t mpu60_class_type;

STATIC mpu60_class_obj_t mi_mpu60_obj;
/*
    This function prints the information that the struct mpu60_class_obj_t contains in certain moment.
    It is invoked when the MicroPython user writes "print(obj)", for example:
        SAG = MPU6050()
        print(SAG)        
*/
STATIC void mpu60_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    (void)kind;
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_print_str(print, "OPHYRA_MPU6050_SENSOR_OBJ\n");
    mp_print_str(print, "(g: ");
    mp_obj_print_helper(print, mp_obj_new_float(self->g), PRINT_REPR);  
    mp_print_str(print, " sen: ");
    mp_obj_print_helper(print, mp_obj_new_float(self->sen), PRINT_REPR);  
    mp_print_str(print, ")");         

}

/*
    Function that initializes the port 1 for the IC2 communication with the sensor.
    The value of the WHO_AM_I register is read, to verify the presence of the sensor. If its presence is not verified,
    an error ocurrs.
*/
STATIC void mpu60_start(void){

    i2c_init(I2C1, MICROPY_HW_I2C1_SCL, MICROPY_HW_I2C1_SDA, 400000, I2C_TIMEOUT_MS);

    uint8_t data[2] = { MPU60_WHO_AM_I_REG };
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data, 1, false);
    i2c_readfrom(I2C1, MPU6050_OPHYRA_ADDRESS, data, 1, true);
    if (data[0] != 0x68) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("MPU6050 not found.\n"));
    }

}

/*
    Function that is invoked when a new MPU6050() object is created in MicroPython, example:
        SAG = MPU6050()
*/
STATIC mp_obj_t mpu60_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    mi_mpu60_obj.base.type = &mpu60_class_type;
   
    mpu60_start();

    return MP_OBJ_FROM_PTR(&mi_mpu60_obj);
}
/*
    Function that is invoked when the MicroPython user writes something like this:
        SAG.init(8,1000)
    This function is for adjusting the accelerometer and gyroscope range.
*/
STATIC mp_obj_t init_function(mp_obj_t self_in, mp_obj_t range_accel_config_obj, mp_obj_t range_gyr_config_obj) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int miRangoAccel = mp_obj_get_int(range_accel_config_obj);
    int miRangoGyr = mp_obj_get_int(range_gyr_config_obj);

    int env_accel_config;
    int env_gyr_config;

    if(miRangoAccel == 2){
        env_accel_config = 0;
        self->g = 16384;
    }
    else if(miRangoAccel == 4){
        env_accel_config = 8;
        self->g = 8192;        
    }
    else if(miRangoAccel == 8){
        env_accel_config = 16;
        self->g = 4096;        
    }    
    else if(miRangoAccel == 16){
        env_accel_config = 24;
        self->g = 2048;        
    }
    else{
        mp_raise_ValueError(MP_ERROR_TEXT("Ingresaste un valor de rango equivocado para el acelerometro."));
    }    

    if(miRangoGyr == 250){
        env_gyr_config = 0;
        self->sen = 131;          
    }
    else if(miRangoGyr == 500){
        env_gyr_config = 8;
        self->sen = 65.5;          
    }
    else if(miRangoGyr == 1000){
        env_gyr_config = 16;
        self->sen = 32.8;          
    }
    else if(miRangoGyr == 2000){
        env_gyr_config = 24;
        self->sen = 16.4;         
    }
    else{
        mp_raise_ValueError(MP_ERROR_TEXT("Ingresaste un valor de rango equivocado para el giroscopio."));
    }

    //Wake up the sensor
    uint8_t data0[2] = {POWER_MANAG_REG, 0};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data0, 2, true);
    //Configuration of the Data output rate or Sample Rate
    uint8_t data1[2] = {MPU60_SMPLRT_DIV_REG, (uint8_t)(7)};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data1, 2, true);
    //Configuration of the accelerometer range
    uint8_t data2[2] = {ACCEL_CONFIG_REG, (uint8_t)(env_accel_config)};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data2, 2, true);
    //Configuration of the gyroscope range
    uint8_t data3[2] = {GYR_CONFIG_REG, (uint8_t)(env_gyr_config)};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data3, 2, true);

    return mp_obj_new_float(1);
}
/*
    Function that reads the two corresponding registers of certain accelerometer or gyroscope axis.
    This function returns the acceleration value (G) or the gyroscope value (°/seg) in that axis.
*/
STATIC mp_obj_t read_axis(int axis, float g_o_sin){
    uint8_t myAxis[1] = {(uint8_t)axis};
    uint8_t lectura_bytes[2];
                                       
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, myAxis, 1, false);       
    i2c_readfrom(I2C1, MPU6050_OPHYRA_ADDRESS, lectura_bytes, 2, true);

    int16_t miValorRes = (int16_t)(lectura_bytes[0] << 8 | lectura_bytes[1]);

    if(miValorRes > 32767){
        miValorRes = (65536 - miValorRes)*-1;
    }

    float resultado = (float)(miValorRes/(float)g_o_sin);

    return mp_obj_new_float(resultado);
}

/*
    Function that is invoked when the MicroPython user writes something like this:
        x=SAG.accX()
    Returns the acceleration value of the sensor in the X axis.
*/                                
STATIC mp_obj_t get_accelerationX(mp_obj_t self_in) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return read_axis(ACCEL_REG_X, self->g);
}

/*
    Function that is invoked when the MicroPython user writes something like this:
        y=SAG.accY()
    Returns the acceleration value of the sensor in the Y axis.
*/   
STATIC mp_obj_t get_accelerationY(mp_obj_t self_in) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return read_axis(ACCEL_REG_Y, self->g);
}

/*
    Function that is invoked when the MicroPython user writes something like this:
        z=SAG.accZ()
    Returns the acceleration value of the sensor in the Z axis.
*/ 
STATIC mp_obj_t get_accelerationZ(mp_obj_t self_in) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return read_axis(ACCEL_REG_Z, self->g);
}

/*
    Function that reads the temperature registers of the sensor. The temperature value in Celcius degrees is
    calculated and returned. This function is invoked when writing in MicroPython something like this:
        tmp=SAG.temp()
*/ 
STATIC mp_obj_t get_temperature(mp_obj_t self_in) {
    uint8_t registro_temp[1] = {(uint8_t)TEMP_REG};
    uint8_t lectura_temperatura[2];

    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, registro_temp, 1, false);    
    i2c_readfrom(I2C1, MPU6050_OPHYRA_ADDRESS, lectura_temperatura, 2, true);

    int16_t miTempLeida = (int16_t)(lectura_temperatura[0] << 8 | lectura_temperatura[1]);

    float miTemperatura_calculada = (float)(miTempLeida/(float)340 + (float)36.53);

    return mp_obj_new_float(miTemperatura_calculada);
}

/*
    Function that is invoked when the MicroPython user writes something like this:
        gx=SAG.gyrX()
    Returns the gyroscope value of the sensor in the X axis.
*/
STATIC mp_obj_t get_gyroscopeX(mp_obj_t self_in) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return read_axis(GYR_REG_X, self->sen);
}

/*
    Function that is invoked when the MicroPython user writes something like this:
        gy=SAG.gyrY()
    Returns the gyroscope value of the sensor in the Y axis.
*/
STATIC mp_obj_t get_gyroscopeY(mp_obj_t self_in) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return read_axis(GYR_REG_Y, self->sen);
}

/*
    Function that is invoked when the MicroPython user writes something like this:
        gz=SAG.gyrZ()
    Returns the gyroscope value of the sensor in the Z axis.
*/
STATIC mp_obj_t get_gyroscopeZ(mp_obj_t self_in) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return read_axis(GYR_REG_Z, self->sen);
}

/*
    Function that allows to write a byte of information to a specific register of the sensor.
*/
STATIC mp_obj_t write_function(mp_obj_t self_in, mp_obj_t num_obj, mp_obj_t address_obj) {
    int numero_a_escribir = mp_obj_get_int(num_obj);
    int direccion_a_escribir = mp_obj_get_int(address_obj);

    uint8_t data[2] = {(uint8_t)direccion_a_escribir, (uint8_t)numero_a_escribir};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data, 2, true);

    return mp_obj_new_int(0);
}

/*
    Function that allows to read a byte of information from a specific register of the sensor.
    It returns the read value of the register.
*/
STATIC mp_obj_t read_function(mp_obj_t self_in, mp_obj_t address_obj) {
    int direccion_a_leer = mp_obj_get_int(address_obj);

    uint8_t registro_a_leer[1] = {(uint8_t)direccion_a_leer};

    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, registro_a_leer, 1, false);    
    i2c_readfrom(I2C1, MPU6050_OPHYRA_ADDRESS, registro_a_leer, 1, true);    

    return mp_obj_new_int(registro_a_leer[0]);
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
                                                    //Name of this C usermod file
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ophyra_mpu60) },
            //Name of the class        //Name of the associated "type"
    { MP_ROM_QSTR(MP_QSTR_MPU6050), MP_ROM_PTR(&mpu60_class_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ophyra_mpu60_globals, ophyra_mpu60_globals_table);

// Define module object.
const mp_obj_module_t ophyra_mpu60_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ophyra_mpu60_globals,
};

// We need to register the module to make it available for Python.
//                          nameOfTheFile, nameOfTheFile_user_cmodule, MODULE_IDENTIFIER_ENABLED
MP_REGISTER_MODULE(MP_QSTR_ophyra_mpu60, ophyra_mpu60_user_cmodule, MODULE_OPHYRA_MPU60_ENABLED);

