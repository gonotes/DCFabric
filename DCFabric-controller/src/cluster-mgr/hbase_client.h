/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
 * Controller is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, , see <http://www.gnu.org/licenses/>.
 */

/******************************************************************************
*                                                                             *
*   File Name   : hbase_client.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-20           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef HBASE_CLIENT_H_
#define HBASE_CLIENT_H_

#include "common.h"

#define TABLE_STRING_LEN 256
#define TOPO_VER "topo_version"
#define MASTER_ID "master_id"
#define ELECTION_GENERATION_ID "election_generation_id"

//topology
#define TOPO_TABLE "T_TOPOLOGY"
#define TOPO_TABLE_CF "CF_TOPO"

//common
#define COMMON_TABLE "T_COMMON"
#define COMMON_TABLE_CF "CF_COMM"

//flow
#define DYNAMIC_FLOW_TABLE "T_FLOW"
#define DYNAMIC_FLOW_TABLE_CF "CF_FLOW"


extern INT1 g_hbase_ip[];
extern INT1 g_hbase_port[];

void hbase_client_add_record(INT1 *table_name, INT1 *cloumn_family, INT1 *row, INT4 num, INT1 *column[], INT1 *value[]);
void persist_topology();
void query_topology();
void delete_record();
void persist_value(const INT1* key, const INT1* value);
void query_value(const INT1* key, INT1* value);

INT4 init_hbase_client();
void fini_hbase_client();

#endif /* HBASE_CLIENT_H_ */
