// Copyright 2018-2019 trans-semi inc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef AT_COM_H
#define AT_COM_H
#include "stddef.h"
#include "dce.h"

#define BUILD_UINT8(loByte, hiByte ) \
    ((unsigned char)(((loByte) & 0x0F) + (((hiByte) & 0x0F) << 4)))
unsigned char write_buffer[4110];
int TG_strtohex(const char * data,int data_len);



void  target_dce_transmit(const char* data, size_t size);

void  target_dce_request_process_command_line(dce_t* dce);

void  target_dce_reset(void);

int target_dce_get_state(void);

void* target_dce_get(void);

void  target_dce_init_factory_defaults(void);

void target_dec_switch_input_state(state_t state);
#endif
