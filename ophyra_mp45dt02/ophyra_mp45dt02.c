/*
    ophyra_mp45dt02.c
    C usermod to use the microphone MP45DT02 on the Ophyra board, manufactured by
    Intesc Electronica y Embebidos, located in Puebla, Pue. Mexico.
    Date: 14 december 2021 
    Written by: Dionicio Meza.
*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/misc.h"
#include "py/stream.h"
#include "py/objstr.h"
#include "modmachine.h"
#include "pin.h"
#include "dma.h"
#include "py/mphal.h"    

#define FREC_PDM 176000
#define FREC_PCM 44000
#define LONG_BUF 6 * 2
#define HALF_BUF (LONG_BUF/2)

typedef struct _non_blocking_descriptor_t {
    mp_buffer_info_t appbuf;
    uint32_t index;
    bool copy_in_progress;
} non_blocking_descriptor_t;

typedef struct _mp45dt02_obj_t {
    mp_obj_base_t base;
    mp_obj_t callback_for_non_blocking;
    uint16_t dma_buffer[LONG_BUF];
    non_blocking_descriptor_t non_blocking_descriptor;
    
    I2S_HandleTypeDef hi2s2;
    DMA_HandleTypeDef hdma_rx;
    const dma_descr_t *dma_descr_rx;
} mp45dt02_obj_t;

STATIC mp_obj_t mp45dt02_deinit(mp_obj_t self_in);

const mp_obj_type_t mp45dt02_type;
STATIC mp45dt02_obj_t *mp45dt02_obj;
 
void mp45dt02_init0() {
    
    mp45dt02_obj = NULL;
   
}
 
#define WINDOWS    64

uint16_t sincfilter[WINDOWS] = {0, 2, 9, 21, 39, 63, 94, 132, 179, 236, 302, 379, 467, 565, 674, 792,920, 1055, 1196, 1341, 1487, 1633, 1776, 1913, 2042, 2159, 2263, 2352, 2422, 2474, 2506, 2516,
	2516,2506, 2474, 2422, 2352, 2263, 2159, 2042, 1913, 1776, 1633, 1487, 1341, 1196, 1055, 920, 792,674, 565, 467, 379, 302, 236, 179, 132, 94, 63, 39, 21, 9, 2, 0};

#define PDM_REPEAT_LOOP_16(X) X X X X X X X X X X X X X X X X

#define M 103
float h[M] = {



  0.005383878543905767,//50-1000,1500 44k  103
  0.0008745285494715447,
  0.0008482121881024766,
  0.0007460661434193918,
  0.0005591943982935655,
  0.0002791528160262167,
  -0.00009918091362182439,
  -0.0005811661980721277,
  -0.0011671893566906496,
  -0.0018565212849891023,
  -0.002645092214276294,
  -0.0035243971082051908,
  -0.004482296433960019,
  -0.005504049776296296,
  -0.00657090631514472,
  -0.007660384600351277,
  -0.008746718621093214,
  -0.009800854443203912,
  -0.010792801755240751,
  -0.011690626248753846,
  -0.012460044110719715,
  -0.013066138954722997,
  -0.013478050976535164,
  -0.013661419705534182,
  -0.01358814972261887,
  -0.013230659567661602,
  -0.012569445996477512,
  -0.01157485826919379,
  -0.01025972395735936,
  -0.008591627155855458,
  -0.0065797395189081615,
  -0.004235785042586487,
  -0.0015738584167381312,
  0.0013871360655679007,
  0.0046189335455996275,
  0.008086638890839394,
  0.011747688496100916,
  0.015554266016657506,
  0.019454870341107575,
  0.023394135061082386,
  0.02731338070322758,
  0.031152907654284957,
  0.03485296501818983,
  0.03835421967664724,
  0.04159966314102669,
  0.04453367705215179,
  0.04710590599599399,
  0.049273780397548254,
  0.05100223257635541,
  0.052255648506699295,
  0.05301808055521172,
  0.05327320160592689,
  0.05301808055521172,
  0.052255648506699295,
  0.05100223257635541,
  0.049273780397548254,
  0.04710590599599399,
  0.04453367705215179,
  0.04159966314102669,
  0.03835421967664724,
  0.03485296501818983,
  0.031152907654284957,
  0.02731338070322758,
  0.023394135061082386,
  0.019454870341107575,
  0.015554266016657506,
  0.011747688496100916,
  0.008086638890839394,
  0.0046189335455996275,
  0.0013871360655679007,
  -0.0015738584167381312,
  -0.004235785042586487,
  -0.0065797395189081615,
  -0.008591627155855458,
  -0.01025972395735936,
  -0.01157485826919379,
  -0.012569445996477512,
  -0.013230659567661602,
  -0.01358814972261887,
  -0.013661419705534182,
  -0.013478050976535164,
  -0.013066138954722997,
  -0.012460044110719715,
  -0.011690626248753846,
  -0.010792801755240751,
  -0.009800854443203912,
  -0.008746718621093214,
  -0.007660384600351277,
  -0.00657090631514472,
  -0.005504049776296296,
  -0.004482296433960019,
  -0.0035243971082051908,
  -0.002645092214276294,
  -0.0018565212849891023,
  -0.0011671893566906496,
  -0.0005811661980721277,
  -0.00009918091362182439,
  0.0002791528160262167,
  0.0005591943982935655,
  0.0007460661434193918,
  0.0008482121881024766,
  0.0008745285494715447,
  0.005383878543905767

};


float x_n[M+1]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};



float filtro(int in){
	float y=0;

	x_n[0] = in;
	y = h[0] * x_n[0];
	for(int j=1;j<M;j++){
		y += h[j] * x_n[j];
	};

	for (int i=M;i>0;i--){
		x_n[i]=x_n[i-1];
	};
	return y;
}


void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s2) {
    uint32_t errorCode = HAL_I2S_GetError(hi2s2);
    printf("I2S Error = %ld\n", errorCode);
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s2) {
    
    mp45dt02_obj_t *self;
    self = mp45dt02_obj;   
    
    if (self->non_blocking_descriptor.copy_in_progress) {
        uint16_t runningsum = 0;
		uint16_t *sinc_ptr = sincfilter;
		for (uint8_t i=HALF_BUF; i <= HALF_BUF+3 ; i++) {
			PDM_REPEAT_LOOP_16({
				if (self->dma_buffer[i] & 0x1) {
					runningsum += *sinc_ptr;
				}
				sinc_ptr++;
				self->dma_buffer[i] >>= 1;
			})
		}
		if (self->non_blocking_descriptor.index*2 >= self->non_blocking_descriptor.appbuf.len){
            self->non_blocking_descriptor.copy_in_progress = false;
            mp_sched_schedule(self->callback_for_non_blocking, MP_OBJ_FROM_PTR(self));
            
		}
	    ((uint16_t *)self->non_blocking_descriptor.appbuf.buf)[self->non_blocking_descriptor.index] = (uint16_t)filtro(runningsum);
	    self->non_blocking_descriptor.index++;
    }
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s2) {
    mp45dt02_obj_t *self;
    self = mp45dt02_obj;
    
    
    

    if (self->non_blocking_descriptor.copy_in_progress)  {
        uint16_t runningsum = 0;
        uint16_t *sinc_ptr = sincfilter;
	    for (uint8_t i=0; i <= 3 ; i++) {
		    PDM_REPEAT_LOOP_16({
			if (self->dma_buffer[i] & 0x1) {
				runningsum += *sinc_ptr;
			}
			sinc_ptr++;
			self->dma_buffer[i] >>= 1;
		    })
	    }
	    if (self->non_blocking_descriptor.index*2 >= self->non_blocking_descriptor.appbuf.len){
		    self->non_blocking_descriptor.copy_in_progress = false;
            mp_sched_schedule(self->callback_for_non_blocking, MP_OBJ_FROM_PTR(self));
            
	    }
	    ((uint16_t *)self->non_blocking_descriptor.appbuf.buf)[self->non_blocking_descriptor.index] = (uint16_t)filtro(runningsum);
	    self->non_blocking_descriptor.index++;
    
    }
    
}

STATIC void mp45dt02_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    
    (void)kind;
    mp_print_str(print, "mp45dt02_class()");
}

STATIC bool i2s_init(mp45dt02_obj_t *self) {

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
    PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;

    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStructure.Pull = GPIO_NOPULL;

    
    self->hi2s2.Instance = SPI2;
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    self->dma_descr_rx = &dma_I2S_2_RX;
    
    
    GPIO_InitStructure.Pin = GPIO_PIN_3;
    GPIO_InitStructure.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
    

    
    GPIO_InitStructure.Pin = GPIO_PIN_12|GPIO_PIN_13;
    GPIO_InitStructure.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

    if (HAL_I2S_Init(&self->hi2s2) == HAL_OK) {
        
        dma_invalidate_channel(self->dma_descr_rx);
        dma_init(&self->hdma_rx, self->dma_descr_rx, DMA_PERIPH_TO_MEMORY, &self->hi2s2);
        self->hi2s2.hdmarx = &self->hdma_rx;
        

        return true;
    } else {
        return false;
    }

}

STATIC void mp45dt02_init_helper(mp45dt02_obj_t *self) {

    memset(&self->hi2s2, 0, sizeof(self->hi2s2));

    self->callback_for_non_blocking = MP_OBJ_NULL;
    self->non_blocking_descriptor.copy_in_progress = false;

    I2S_InitTypeDef *init = &self->hi2s2.Init;
    init->Mode = I2S_MODE_MASTER_RX;
    init->Standard = I2S_STANDARD_PHILIPS;
    init->DataFormat = I2S_DATAFORMAT_16B;
    init->MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    init->AudioFreq = FREC_PDM;
    init->CPOL = I2S_CPOL_LOW;
    init->ClockSource = I2S_CLOCK_PLL;
    init->FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;

    if (!i2s_init(self)) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("I2S init failed"));
    }
    
    HAL_StatusTypeDef status;
    status = HAL_I2S_Receive_DMA(&self->hi2s2,&self->dma_buffer[0], HALF_BUF);
    

    if (status != HAL_OK) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("DMA init failed"));
    }
}



STATIC mp_obj_t mp45dt02_make_new(const mp_obj_type_t *type, size_t n_pos_args, size_t n_kw_args, const mp_obj_t *args) {
    mp_arg_check_num(n_pos_args, n_kw_args, 0, 0, false);
    
    mp45dt02_obj_t *self;

    if (mp45dt02_obj == NULL) {
        self = m_new_obj(mp45dt02_obj_t);
        mp45dt02_obj = self;
        self->base.type = &mp45dt02_type;
    } else {
        self = mp45dt02_obj;
        mp45dt02_deinit(MP_OBJ_FROM_PTR(self));
    }

    mp45dt02_init_helper(self);
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t mp45dt02_init(size_t n_pos_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp45dt02_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp45dt02_deinit(MP_OBJ_FROM_PTR(self));
    mp45dt02_init_helper(self);
    self->non_blocking_descriptor.copy_in_progress = true;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp45dt02_init_obj, 1, mp45dt02_init);

STATIC mp_obj_t mp45dt02_deinit(mp_obj_t self_in) {

    mp45dt02_obj_t *self = MP_OBJ_TO_PTR(self_in);

    dma_deinit(self->dma_descr_rx);
    HAL_I2S_DeInit(&self->hi2s2);

    __HAL_RCC_SPI2_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_3);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12|GPIO_PIN_13);


    __SPI2_FORCE_RESET();
    __SPI2_RELEASE_RESET();
    __SPI2_CLK_DISABLE();
    


    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp45dt02_deinit_obj, mp45dt02_deinit);

STATIC mp_obj_t mp45dt02_irq(mp_obj_t self_in, mp_obj_t handler) {
    mp45dt02_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (handler != mp_const_none && !mp_obj_is_callable(handler)) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid callback"));
    }
    
    self->callback_for_non_blocking = handler;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp45dt02_irq_obj, mp45dt02_irq);


STATIC mp_uint_t mp45dt02_stream_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    mp45dt02_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (size == 0) {
        return 0;
    }

    self->non_blocking_descriptor.appbuf.buf = (void *)buf_in;
    self->non_blocking_descriptor.appbuf.len = size;
    self->non_blocking_descriptor.index = 0;
    self->non_blocking_descriptor.copy_in_progress = true;
    return size;
}

STATIC const mp_rom_map_elem_t mp45dt02_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init),            MP_ROM_PTR(&mp45dt02_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto),        MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),          MP_ROM_PTR(&mp45dt02_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq),             MP_ROM_PTR(&mp45dt02_irq_obj) },

};
MP_DEFINE_CONST_DICT(mp45dt02_locals_dict, mp45dt02_locals_dict_table);



STATIC const mp_stream_p_t i2s_stream_p = {
    .read = mp45dt02_stream_read,
    .is_text = false,
}; 

const mp_obj_type_t mp45dt02_type = {
    { &mp_type_type },
    .name = MP_QSTR_ophyra_mp45dt02,
    .print = mp45dt02_print,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &i2s_stream_p,
    .make_new = mp45dt02_make_new,
    .locals_dict = (mp_obj_dict_t *)&mp45dt02_locals_dict,
};

STATIC const mp_rom_map_elem_t ophyra_mp45dt02_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ophyra_mp45dt02) },
    { MP_ROM_QSTR(MP_QSTR_MP45DT02), MP_ROM_PTR(&mp45dt02_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ophyra_mp45dt02_globals, ophyra_mp45dt02_globals_table);

const mp_obj_module_t ophyra_mp45dt02_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ophyra_mp45dt02_globals,
};

MP_REGISTER_MODULE(MP_QSTR_ophyra_mp45dt02, ophyra_mp45dt02_user_cmodule, MODULE_OPHYRA_MP45DT02_ENABLED);