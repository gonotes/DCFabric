/*
 * DCFabric GPL Source Code
 * Copyright (C) 2015, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the DCFabric SDN Controller. DCFabric SDN
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

/*
 * openstack_host.h
 *
 *  Created on: Jun 16, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */

#ifndef INC_OPENSTACK_OPENSTACK_HOST_H_
#define INC_OPENSTACK_OPENSTACK_HOST_H_
#include "gnflush-types.h"
// tenant
#define OPENSTACK_TENANT_ID_LEN 48

// network
#define OPENSTACK_NETWORK_ID_LEN 48
#define OPENSTACK_NETWORK_NAME_LEN 48
#define OPENSTACK_NETWORK_MAX_NUM 128

// subnet
#define OPENSTACK_SUBNET_ID_LEN 48
#define OPENSTACK_SUBNET_NAME_LEN 48
#define OPENSTACK_SUBNET_MAX_NUM 1024

// port
#define OPENSTACK_PORT_ID_LEN 48
#define OPENSTACK_PORT_MAX_NUM 10240

// node
#define OPENSTACK_NODE_MAX_NUM (OPENSTACK_NETWORK_MAX_NUM+OPENSTACK_SUBNET_MAX_NUM+OPENSTACK_PORT_MAX_NUM)

////////////////////////////////////////////////////////////////////////


typedef struct _openstack_port{
	gn_switch_t* sw;
	UINT8 dpid;
	UINT4 port;
	UINT4 ip;
	UINT1 mac[6];
	UINT4 ofport_no;
	char tenant_id[OPENSTACK_TENANT_ID_LEN];
	char network_id[OPENSTACK_NETWORK_ID_LEN];
	char subnet_id[OPENSTACK_SUBNET_ID_LEN];
	char port_id[OPENSTACK_PORT_ID_LEN];
}openstack_port,* openstack_port_p;

typedef struct _openstack_subnet{
	char tenant_id[OPENSTACK_TENANT_ID_LEN];
	char network_id[OPENSTACK_NETWORK_ID_LEN];
	//char subnet_name[OPENSTACK_SUBNET_NAME_LEN];
	char subnet_id[OPENSTACK_SUBNET_ID_LEN];
	UINT4 gateway_ip;
	UINT4 start_ip;
	UINT4 end_ip;
	openstack_port_p gateway_port;
	openstack_port_p dhcp_port;
	UINT4 port_num;
}openstack_subnet,* openstack_subnet_p;

typedef struct _openstack_network{
	char tenant_id[OPENSTACK_TENANT_ID_LEN];
	char network_id[OPENSTACK_NETWORK_ID_LEN];
	UINT4 subnet_num;
	UINT1 shared;
}openstack_network,* openstack_network_p;

typedef struct _openstack_node{
	UINT1* data;
	struct _openstack_node* next;
}openstack_node,*openstack_node_p;

////////////////////////////////////////////////////////////////////////

void init_openstack_host();
void show_openstack_total();
void destory_openstack_host();

////////////////////////////////////////////////////////////////////////
openstack_network_p create_openstack_host_network(
		char* tenant_id,
		char* network_id,
		UINT1 shared);
void destory_openstack_host_network(openstack_network_p network);
void add_openstack_host_network(openstack_network_p network);
openstack_network_p find_openstack_host_network_by_network_id(char* network_id);
openstack_network_p remove_openstack_host_network_by_network_id(char* network_id);


////////////////////////////////////////////////////////////////////////
openstack_subnet_p create_openstack_host_subnet(
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		UINT4 gateway_ip,
		UINT4 start_ip,
		UINT4 end_ip);
void destory_openstack_host_subnet(openstack_subnet_p subnet);
void add_openstack_host_subnet(openstack_subnet_p subnet);
openstack_subnet_p find_openstack_host_subnet_by_subnet_id(char* subnet_id);
openstack_subnet_p remove_openstack_host_subnet_by_subnet_id(char* subnet_id);
openstack_subnet_p find_openstack_host_subnet_by_geteway_ip(UINT4 gateway_ip);

void delete_openstack_host_subnet_by_tenant_id(char* tenant_id);
void delete_openstack_host_subnet_by_network_id(char* network_id);
void delete_openstack_host_subnet_by_subnet_id(char* subnet_id);
////////////////////////////////////////////////////////////////////////
openstack_port_p create_openstack_host_port(
		gn_switch_t* sw,
		UINT4 port,
		UINT4 ip,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id);
void destory_openstack_host_port(openstack_port_p port);
void add_openstack_host_port(openstack_port_p port);
void set_openstack_host_port_portno(const UINT1 *mac, UINT4 ofport_no);
openstack_port_p find_openstack_host_port_by_port_id(char* port_id);
openstack_port_p remove_openstack_host_port_by_port_id(char* port_id);
openstack_port_p find_openstack_host_port_by_mac(UINT1* mac);
openstack_port_p remove_openstack_host_port_by_mac(UINT1* mac);
openstack_port_p find_openstack_host_port_by_ip_tenant(UINT4 ip,char* tenant_id);
openstack_port_p find_openstack_host_port_by_ip_network(UINT4 ip,char* network_id);
openstack_port_p find_openstack_host_port_by_ip_subnet(UINT4 ip,char* subnet_id);

UINT4 find_openstack_host_ports_by_subnet_id(char* subnet_id,openstack_port_p* host_list);

void delete_openstack_host_port_by_tenant_id(char* tenant_id);
void delete_openstack_host_port_by_network_id(char* network_id);
void delete_openstack_host_port_by_subnet_id(char* subnet_id);
void delete_openstack_host_port_by_port_id(char* port_id);
////////////////////////////////////////////////////////////////////////

openstack_node_p create_openstack_host_node(UINT1* data);
void destory_openstack_host_node(openstack_node_p node);
#endif /* INC_OPENSTACK_OPENSTACK_HOST_H_ */
