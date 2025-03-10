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

#ifndef __AT_DEF_H
#define __AT_DEF_H


#include "oshal.h"
#include "cli.h"

#define CONFIG_AT_COMMAND 1

#define malloc    os_malloc
#define free      os_free
#define calloc    os_calloc
#define zalloc    os_zalloc
#define realloc   os_realloc

#define SSL_CERT_PEM_LEN                    1674
#define SSL_CA_PEM_FLASH_ADDR               0x1F4000
#define SSL_CLIENT_CRT_PEM_FLASH_ADDR       0x1F5000
#define SSL_CLIENT_KEY_PEM_FLASH_ADDR       0x1F6000
#define SSL_CRT_PEM_START_STR              "-----BEGIN CERTIFICATE-----"
#define SSL_CRT_PEM_END_STR                "-----END CERTIFICATE-----"
#define SSL_KEY_PEM_START_STR              "-----BEGIN RSA PRIVATE KEY-----"
#define SSL_KEY_PEM_END_STR                "-----END RSA PRIVATE KEY-----"



#endif