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

void  target_dce_transmit(const char* data, size_t size);

void  target_dce_request_process_command_line(dce_t* dce);

void  target_dce_reset(void);

void  target_dce_init_factory_defaults(void);

void target_dec_switch_input_state(state_t state);
#endif