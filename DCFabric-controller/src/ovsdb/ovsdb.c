/******************************************************************************
 *                                                                             *
 *   File Name   : ovsdb.c           *
 *   Author      : greenet Administrator           *
 *   Create Date : 2015-3-20           *
 *   Version     : 1.0           *
 *   Function    : .           *
 *                                                                             *
 ******************************************************************************/

#include "ovsdb.h"
#include "openflow-common.h"

struct sockaddr_in g_ovsdb_addr;
INT4 g_ovsdb_sockfd;
fd_set g_ovsdb_recvmask;
pthread_t g_ovsdb_recv_tid;

UINT1 g_ovsdb_of_version = OFP13_VERSION;           //openstack ovs��openflow�汾
UINT1 g_tunnel_type = NETWORK_TYPE_VXLAN;           //openstack�������� gre/vxlan

UINT1 g_ovsdb_clients[OVSDB_MAX_CONNECTION] = { 0 };
UINT1 g_ovsdb_clients_bak[OVSDB_MAX_CONNECTION] = { 0 };    //clients�ı���  ����ɾ��˿����ӵĽ�����
UINT4 g_ovsdb_clients_ip[OVSDB_MAX_CONNECTION] = { 0 };

ovsdb_server_t g_ovsdb_nodes[OVSDB_MAX_CONNECTION];
UINT4 g_ovsdb_port = OVSDB_SERVER_PORT;    //ovsdb�ӿڣ�Ĭ�϶˿�6640
UINT4 g_ovsdb_turnel_on = 1;

INT4 ovsdb_connect_quit()
{
    INT4 index;
    if (g_ovsdb_sockfd > 0)
    {
        close(g_ovsdb_sockfd);
        g_ovsdb_sockfd = 0;
    }

    for (index = 0; index < OVSDB_MAX_CONNECTION; index++)
    {
        if (g_ovsdb_clients[index])
        {
            FD_CLR(g_ovsdb_clients[index], &g_ovsdb_recvmask);
            close(g_ovsdb_clients[index]);
            g_ovsdb_clients[index] = 0;
        }
    }
    return GN_OK;
}

static inline BOOL is_socket_dead(INT4 recv_res)
{
    if ((recv_res == 0) || ((recv_res < 0) && (errno != EAGAIN)))
    {
        return TRUE;
    }
    return FALSE;
}

//{"id":"1b5f9ab0-4abc-40c3-97bc-7ea66e663923","method":"monitor","params":["Open_vSwitch",null,{"Bridge":{},"Port":{},"Interface":{},"Controller":{},"Manager":{},"Mirror":{},"NetFlow":{},"Open_vSwitch":{},"QoS":{},"Queue":{},"sFlow":{},"SSL":{},"Flow_Sample_Collector_Set":{},"Flow_Table":{},"IPFIX":{}}]}
void search_from_ovsdb_table_all(INT4 conn_fd)
{
    INT1 *text;
    json_t *array, *obj, *key, *value, *table_obj;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string(SEARCH_ALL_TABLE_ID);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("method");
    value = json_new_string("monitor");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("params");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(obj, key);

    value = json_new_string("Open_vSwitch");
    json_insert_child(array, value);

    value = json_new_string("null");
    json_insert_child(array, value);

    value = json_new_object();
    json_insert_child(array, value);

    //Open_vSwitch
    key = json_new_string("Open_vSwitch");
    table_obj = json_new_object();
    json_insert_child(key, table_obj);
    json_insert_child(value, key);

    //bridge
    key = json_new_string("Bridge");
    table_obj = json_new_object();
    json_insert_child(key, table_obj);
    json_insert_child(value, key);

    //Controller
    key = json_new_string("Controller");
    table_obj = json_new_object();
    json_insert_child(key, table_obj);
    json_insert_child(value, key);

    /*
     //Port
     key  = json_new_string("Port");
     table_obj   = json_new_object();
     json_insert_child(key, table_obj);
     json_insert_child(value, key);

     //Interface
     key  = json_new_string("Interface");
     table_obj   = json_new_object();
     json_insert_child(key, table_obj);
     json_insert_child(value, key);


     //Manager
     key  = json_new_string("Manager");
     table_obj   = json_new_object();
     json_insert_child(key, table_obj);
     json_insert_child(value, key);
     */
    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    send(conn_fd, text, strlen(text), 0);
}

//{"id": "d0d14471-5a66-412c-bcae-0e594f5ee9af","method": "transact","params": ["Open_vSwitch",{"op": "mutate","table": "Bridge","where": [["_uuid","==",["uuid","6656bc46-915c-4eef-8e07-3c53cf916387"]]],"mutations": [["ports","insert",["named-uuid","new_port"]]]},{"op": "insert","table": "Port","row": {"name": "gre-10.8.1.212","interfaces": ["set",[["named-uuid","new_interface"]]]},"uuid-name": "new_port"},{"op": "insert","table": "Interface","row": {"name": "gre-10.8.1.212","options": ["map",[["local_ip","10.8.1.211"],["remote_ip","10.8.1.212"],["key","flow"]]],"type": "gre"},"uuid-name": "new_interface"}]}
void add_port_and_portoption(INT4 conn_fd, INT1 *_uuid_br, INT1 *port_name,
        INT1 *local_ip, INT1 *remote_ip, INT1 *option_key, INT1 *type)
{
    INT1 *text;
    json_t *root_array, *array, *obj, *key, *value, *db_obj, *_uuid_array,
            *uuid_array, *named_array, *row_obj;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string(ADD_INTERFACE_OPTION_ID);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("method");
    value = json_new_string("transact");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("params");
    root_array = json_new_array();
    json_insert_child(key, root_array);
    json_insert_child(obj, key);

    key = json_new_string("Open_vSwitch");
    json_insert_child(root_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("mutate");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    //where
    key = json_new_string("where");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("_uuid");
    json_insert_child(_uuid_array, key);

    key = json_new_string("==");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string(_uuid_br);
    json_insert_child(uuid_array, key);

    //mutations
    key = json_new_string("mutations");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("ports");
    json_insert_child(_uuid_array, key);

    key = json_new_string("insert");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("named-uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string("new_port");
    json_insert_child(uuid_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    //Port
    key = json_new_string("op");
    value = json_new_string("insert");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Port");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("name");
    value = json_new_string(port_name);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    //interfaces
    key = json_new_string("interfaces");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    named_array = json_new_array();
    json_insert_child(uuid_array, named_array);

    key = json_new_string("named-uuid");
    json_insert_child(named_array, key);

    key = json_new_string("new_interface");
    json_insert_child(named_array, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_port");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    //interface
    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("insert");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Interface");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("name");
    value = json_new_string(port_name);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    key = json_new_string("options");
    _uuid_array = json_new_array();
    json_insert_child(key, _uuid_array);
    json_insert_child(row_obj, key);

    key = json_new_string("map");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    named_array = json_new_array();
    json_insert_child(uuid_array, named_array);
    key = json_new_string("local_ip");
    json_insert_child(named_array, key);
    key = json_new_string(local_ip);
    json_insert_child(named_array, key);

    named_array = json_new_array();
    json_insert_child(uuid_array, named_array);
    key = json_new_string("remote_ip");
    json_insert_child(named_array, key);
    key = json_new_string(remote_ip);
    json_insert_child(named_array, key);

    named_array = json_new_array();
    json_insert_child(uuid_array, named_array);
    key = json_new_string("key");
    json_insert_child(named_array, key);
    key = json_new_string(option_key);
    json_insert_child(named_array, key);

    key = json_new_string("type");
    value = json_new_string(type);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_interface");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    send(conn_fd, text, strlen(text), 0);
}

//{"id":"d0d14471-5a66-412c-bcae-0e594f5ee9af","method":"transact","params":["Open_vSwitch",{"op":"mutate","table":"Bridge","where":[["_uuid","==",["uuid","6656bc46-915c-4eef-8e07-3c53cf916387"]]],"mutations":[["ports","insert",["named-uuid","new_port"]]]},{"op":"insert","table":"Port","row":{"name":"gre-10.8.1.212","interfaces":["set",[["named-uuid","new_interface"]]]},"uuid-name":"new_port"},{"op":"insert","table":"Interface","row":{"name":"gre-10.8.1.212"},"uuid-name":"new_interface"}]}
void add_port(INT4 conn_fd, INT1 *br_uuid, INT1 *port_name)
{
    INT1 *text;
    json_t *root_array, *array, *obj, *key, *value, *db_obj, *_uuid_array,
            *uuid_array, *named_array, *row_obj;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string(ADD_PORT_ID);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("method");
    value = json_new_string("transact");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("params");
    root_array = json_new_array();
    json_insert_child(key, root_array);
    json_insert_child(obj, key);

    key = json_new_string("Open_vSwitch");
    json_insert_child(root_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("mutate");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    //where
    key = json_new_string("where");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("_uuid");
    json_insert_child(_uuid_array, key);

    key = json_new_string("==");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string(br_uuid);
    json_insert_child(uuid_array, key);

    //mutations
    key = json_new_string("mutations");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("ports");
    json_insert_child(_uuid_array, key);

    key = json_new_string("insert");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("named-uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string("new_port");
    json_insert_child(uuid_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    //Port
    key = json_new_string("op");
    value = json_new_string("insert");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Port");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("name");
    value = json_new_string(port_name);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    //interfaces
    key = json_new_string("interfaces");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    named_array = json_new_array();
    json_insert_child(uuid_array, named_array);

    key = json_new_string("named-uuid");
    json_insert_child(named_array, key);

    key = json_new_string("new_interface");
    json_insert_child(named_array, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_port");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    //interface
    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("insert");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Interface");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("name");
    value = json_new_string(port_name);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_interface");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    send(conn_fd, text, strlen(text), 0);
}

//{"id":"315c15f7-8b78-41ff-880f-fc6211cb5b8e","method":"transact","params":["Open_vSwitch",{"op":"mutate","table":"Open_vSwitch","where":[["_uuid","==",["uuid","17074e89-2ac5-4bba-997a-1a5a3527cf56"]]],"mutations":[["bridges","insert",["named-uuid","new_bridge"]]]},{"op":"insert","table":"Bridge","row":{"name":"br-int","fail_mode":["set",["secure"]],"protocols":["set",["OpenFlow13"]]},"uuid-name":"new_bridge"}]}
void add_bridge(INT4 conn_fd, INT1 *br_name, INT1 *open_vswitch_uuid, INT1 *fail_mode, INT1 *of_proto)
{
    INT1 *text;
    json_t *root_array, *array, *obj, *key, *value, *db_obj, *_uuid_array, *uuid_array, *row_obj;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string(ADD_BRIDGE_ID);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("method");
    value = json_new_string("transact");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("params");
    root_array = json_new_array();
    json_insert_child(key, root_array);
    json_insert_child(obj, key);

    key = json_new_string("Open_vSwitch");
    json_insert_child(root_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("mutate");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Open_vSwitch");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    //where
    key = json_new_string("where");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("_uuid");
    json_insert_child(_uuid_array, key);

    key = json_new_string("==");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string(open_vswitch_uuid);
    json_insert_child(uuid_array, key);

    //mutations
    key = json_new_string("mutations");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("bridges");
    json_insert_child(_uuid_array, key);

    key = json_new_string("insert");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("named-uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string("new_bridge");
    json_insert_child(uuid_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    //Bridge
    key = json_new_string("op");
    value = json_new_string("insert");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("name");
    value = json_new_string(br_name);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    //fail_mode
    key = json_new_string("fail_mode");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    key = json_new_string(fail_mode);
    json_insert_child(uuid_array, key);

    key = json_new_string("protocols");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    key = json_new_string(of_proto);
    json_insert_child(uuid_array, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    send(conn_fd, text, strlen(text), 0);
}

void add_tunnel(ovsdb_server_t *compute_node, ovs_bridge_t *compute_bridge)
{
    INT4 index;
    INT4 index_br;
    INT1 control_tunnel_port_name[32];
    INT1 compute_tunnel_port_name[32];
    INT1 *remote_ip = NULL;
    INT1 *compute_ip = NULL;

    ovsdb_server_t *remote_node = NULL;
    if(g_ovsdb_turnel_on == 0){
    	return;
    }

    for (index = 0; index < OVSDB_MAX_CONNECTION; index++)
    {
        if (g_ovsdb_clients[index])
        {
            if (g_ovsdb_nodes[index].bridge[0].is_using)
            {
                remote_node = &g_ovsdb_nodes[index];
                remote_ip = strdup(inet_htoa(ntohl(remote_node->node_ip)));
                compute_ip = strdup(inet_htoa(ntohl(compute_node->node_ip)));

                if (g_tunnel_type == NETWORK_TYPE_GRE)
                {
                    sprintf(control_tunnel_port_name, "gre-%s", inet_htoa(ntohl(compute_node->node_ip)));
                    sprintf(compute_tunnel_port_name, "gre-%s", inet_htoa(ntohl(remote_node->node_ip)));
                }
                else if(g_tunnel_type == NETWORK_TYPE_VXLAN)
                {
                    sprintf(control_tunnel_port_name, "vxlan-%s", inet_htoa(ntohl(compute_node->node_ip)));
                    sprintf(compute_tunnel_port_name, "vxlan-%s", inet_htoa(ntohl(remote_node->node_ip)));
                }

                //control---->compute
                for (index_br = 0; index_br < NEUTRON_BRIDGE_MAX_NUM; index_br++)
                {
                    if (strncmp(remote_node->bridge[index_br].name, "br-int", 6) == 0)
                    {
                        if (g_tunnel_type == NETWORK_TYPE_GRE)
                        {
                            add_port_and_portoption(remote_node->node_fd, remote_node->bridge[index_br]._uuid,
                                    control_tunnel_port_name, remote_ip, compute_ip, "key", INTERFACE_TYPE_GRE);
                        }
                        else if (g_tunnel_type == NETWORK_TYPE_VXLAN)
                        {
                            add_port_and_portoption(remote_node->node_fd, remote_node->bridge[index_br]._uuid,
                                    control_tunnel_port_name, remote_ip, compute_ip, "key", INTERFACE_TYPE_VXLAN);
                        }
                        break;
                    }
                }

                //compute---->control
                if (g_tunnel_type == NETWORK_TYPE_GRE)
                {
                    add_port_and_portoption(compute_node->node_fd, compute_bridge->_uuid, compute_tunnel_port_name,
                            compute_ip, remote_ip, "key", INTERFACE_TYPE_GRE);
                }
                else if (g_tunnel_type == NETWORK_TYPE_VXLAN)
                {
                    add_port_and_portoption(compute_node->node_fd, compute_bridge->_uuid, compute_tunnel_port_name,
                            compute_ip, remote_ip, "key", INTERFACE_TYPE_VXLAN);
                }

                free(remote_ip);
                free(compute_ip);
            }
        }
    }
}

//{"id":"2e93096c-ab33-4abd-a244-bad919566fac","method":"transact","params":["Open_vSwitch",{"op":"mutate","table":"Bridge","where":[["_uuid","==",["uuid","612c720c-52a5-4231-bb51-ff94a0d38671"]]],"mutations":[["controller","insert",["named-uuid","new_controller"]]]},{"op":"insert","table":"Controller","row":{"target":"tcp:10.8.1.211:6633"},"uuid-name":"new_controller"}]}
void set_controller(INT4 conn_fd, INT1 *br_uuid, INT1 *controller_ip)
{
    INT1 *text;
    json_t *root_array, *array, *obj, *key, *value, *db_obj, *_uuid_array,
            *uuid_array, *row_obj;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string(SET_CONTROLLER_ID);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("method");
    value = json_new_string("transact");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("params");
    root_array = json_new_array();
    json_insert_child(key, root_array);
    json_insert_child(obj, key);

    key = json_new_string("Open_vSwitch");
    json_insert_child(root_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("mutate");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    //where
    key = json_new_string("where");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("_uuid");
    json_insert_child(_uuid_array, key);

    key = json_new_string("==");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string(br_uuid);
    json_insert_child(uuid_array, key);

    //mutations
    key = json_new_string("mutations");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("controller");
    json_insert_child(_uuid_array, key);

    key = json_new_string("insert");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("named-uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string("new_controller");
    json_insert_child(uuid_array, key);

    //
    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("insert");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Controller");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("target");
    value = json_new_string(controller_ip);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_controller");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    send(conn_fd, text, strlen(text), 0);

}

//{"id":"292174f5-5da9-4d7f-b9a9-1f0b787607c6","method":"transact","params":["Open_vSwitch",{"op":"update","table":"Bridge","where":[["_uuid","==",["uuid","6656bc46-915c-4eef-8e07-3c53cf916387"]]],"row":{"fail_mode":["set",["secure"]],"protocols":["set",["OpenFlow13"]]}}]}
void set_failmod_and_ofver(INT4 conn_fd, INT1 *_uuid_br, INT1 *fail_mode, INT1 *ofproto)
{
    INT1 *text;
    json_t *array, *obj, *key, *value, *db_obj, *_uuid_array, *uuid_array,
            *row_obj;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string(SET_FAILMODE_OF_ID);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("method");
    value = json_new_string("transact");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("params");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(obj, key);

    key = json_new_string("Open_vSwitch");
    json_insert_child(array, key);

    db_obj = json_new_object();
    json_insert_child(array, db_obj);

    key = json_new_string("op");
    value = json_new_string("update");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("where");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("_uuid");
    json_insert_child(_uuid_array, key);

    key = json_new_string("==");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string(_uuid_br);
    json_insert_child(uuid_array, key);

    //row
    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("fail_mode");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    key = json_new_string(fail_mode);
    json_insert_child(uuid_array, key);

    key = json_new_string("protocols");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    key = json_new_string(ofproto);
    json_insert_child(uuid_array, key);

    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    send(conn_fd, text, strlen(text), 0);
}

//{"id":"echo","result":[]}
void echo_reply_ovsdb(INT4 conn_fd)
{
    INT1 *text;
    json_t *array, *obj, *key, *value;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string("echo");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("result");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(obj, key);

    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    send(conn_fd, text, strlen(text), 0);
}

BOOL handle_controller_table(INT4 client_fd, INT4 seq, json_t *result)
{
    json_t *controller = NULL;

    //����Controller
    controller = json_find_first_label(result->child, "Controller");
    if (controller)
    {
        return FALSE;
    }

    return TRUE;
}

void handle_openvswitch_table(INT4 client_fd, INT4 seq, json_t *result)
{
    json_t *open_vswitch = NULL;
    json_t *open_vswitch_uuid = NULL;

    //Open_vSwitch
    open_vswitch = json_find_first_label(result->child, "Open_vSwitch");
    if (open_vswitch)
    {
        open_vswitch_uuid = open_vswitch->child->child;
        if (open_vswitch_uuid)
        {
            //open_vswitch._uuid
            memcpy(g_ovsdb_nodes[seq].open_vswitch._uuid, open_vswitch_uuid->text, strlen(open_vswitch_uuid->text));
            LOG_PROC("INFO", "Hand open_vswitch uuid: %s", g_ovsdb_nodes[seq].open_vswitch._uuid);
        }
    }
}

void handle_bridge_table(INT4 client_fd, INT4 seq, json_t *result, BOOL have_controller)
{
    json_t *br = NULL;
    json_t *br1_uuid = NULL;
    json_t *br2_uuid = NULL;
    json_t *br1_name = NULL;
    json_t *br2_name = NULL;
    INT1 controller_ip[128];

    //Bridge
    br = json_find_first_label(result->child, "Bridge");
    if (br)
    {
        br1_uuid = br->child->child;
        if (br1_uuid)
        {
            memcpy(g_ovsdb_nodes[seq].bridge[0]._uuid, br1_uuid->text,
                    strlen(br1_uuid->text));

            br1_name = br1_uuid->child->child->child->child->child;
            if (br1_name)
            {
                if (g_ovsdb_nodes[seq].bridge[0].is_using)
                {
                    return;
                }

                memcpy(g_ovsdb_nodes[seq].bridge[0].name, br1_name->text, strlen(br1_name->text));
                LOG_PROC("INFO", "Switch info: name[%s], uuid[%s]", g_ovsdb_nodes[seq].bridge[0].name, g_ovsdb_nodes[seq].bridge[0]._uuid);
                if (g_ovsdb_of_version == OFP10_VERSION)
                {
                    set_failmod_and_ofver(client_fd, g_ovsdb_nodes[seq].bridge[0]._uuid, FAIL_MODE_SECURE, OFP_10);
                }
                else if (g_ovsdb_of_version == OFP13_VERSION)
                {
                    set_failmod_and_ofver(client_fd, g_ovsdb_nodes[seq].bridge[0]._uuid, FAIL_MODE_SECURE, OFP_13);
                }

                if (have_controller)
                {
                    sprintf(controller_ip, "tcp:%s:%d", inet_htoa(g_controller_ip), g_controller_south_port);
                    set_controller(client_fd, g_ovsdb_nodes[seq].bridge[0]._uuid, controller_ip);
                }
                g_ovsdb_nodes[seq].bridge[0].is_using = TRUE;
            }

            br2_uuid = br1_uuid->next;
            if (br2_uuid)
            {
                memcpy(g_ovsdb_nodes[seq].bridge[1]._uuid, br2_uuid->text, strlen(br2_uuid->text));
                br2_name = br2_uuid->child->child->child->child->child;
                if (br2_name)
                {
                    if (g_ovsdb_nodes[seq].bridge[1].is_using)
                    {
                        return;
                    }

                    memcpy(g_ovsdb_nodes[seq].bridge[1].name, br2_name->text, strlen(br2_name->text));
                    LOG_PROC("INFO", "Switch info: name[%s], uuid[%s]", g_ovsdb_nodes[seq].bridge[0].name, g_ovsdb_nodes[seq].bridge[0]._uuid);


                    if (g_ovsdb_of_version == OFP10_VERSION)
                    {
                        set_failmod_and_ofver(client_fd, g_ovsdb_nodes[seq].bridge[1]._uuid, FAIL_MODE_SECURE, OFP_10);
                    }
                    else if (g_ovsdb_of_version == OFP13_VERSION)
                    {
                        set_failmod_and_ofver(client_fd, g_ovsdb_nodes[seq].bridge[1]._uuid, FAIL_MODE_SECURE, OFP_13);
                    }

                    if (have_controller)
                    {
                        sprintf(controller_ip, "tcp:%s:%d", inet_htoa(g_controller_ip), g_controller_south_port);
                        set_controller(client_fd, g_ovsdb_nodes[seq].bridge[1]._uuid, controller_ip);
                    }
                    g_ovsdb_nodes[seq].bridge[1].is_using = TRUE;
                }
            }
        }
    }
    else    //add br-int
    {
        if (g_ovsdb_of_version == OFP10_VERSION)
        {
            add_bridge(client_fd, INTERNAL_BR, g_ovsdb_nodes[seq].open_vswitch._uuid, FAIL_MODE_SECURE, OFP_10);
        }
        else if (g_ovsdb_of_version == OFP13_VERSION)
        {
            add_bridge(client_fd, INTERNAL_BR, g_ovsdb_nodes[seq].open_vswitch._uuid, FAIL_MODE_SECURE, OFP_13);
        }
    }
}

void proc_ovsdb_msg(INT1 *ovsdb_msg, INT4 client_fd, UINT4 client_ip, INT4 seq)
{
    json_t *id = NULL;
    json_t *result = NULL;
    json_t *method = NULL;
    json_t *params = NULL;
    json_t *br = NULL;
    json_t *root = NULL;
    INT1* test = NULL;
    INT4 parse_type = 0;
    INT1 controller_ip[128];
    BOOL have_controller = TRUE;

    parse_type = json_parse_document (&root, ovsdb_msg);
    if(parse_type != JSON_OK)
    {
        //printf("json_error type:   %d\n", parse_type);
        return;
    }

    if(NULL == root)
    {
        return;
    }

    json_tree_to_string(root, &test);
    method = json_find_first_label(root, "method");
    if (method)
    {
        if (strncmp(method->child->text, "update", 6) == 0)    //update
        {
            id = method->next;
            if (id)
            {
                if (id->child->type == JSON_NULL)
                {
                    params = id->next;
                    if (params)
                    {
                        br = json_find_first_label(params->child->child->next, "Bridge");
                        if (br)
                        {
                            if (g_ovsdb_nodes[seq].bridge[0].is_using)
                            {
                                return;
                            }

                            memcpy(g_ovsdb_nodes[seq].bridge[0].name, INTERNAL_BR, strlen(INTERNAL_BR));
                            memcpy(g_ovsdb_nodes[seq].bridge[0]._uuid, br->child->child->text, strlen(br->child->child->text));
                            LOG_PROC("INFO", "Switch info: name[%s], uuid[%s]", g_ovsdb_nodes[seq].bridge[0].name, g_ovsdb_nodes[seq].bridge[0]._uuid);
                            sprintf(controller_ip, "tcp:%s:%d", inet_htoa(g_controller_ip), g_controller_south_port);

                            //���ÿ�����
                            set_controller(client_fd, g_ovsdb_nodes[seq].bridge[0]._uuid, controller_ip);

                            //�½�port: br-int
                            add_port(client_fd, g_ovsdb_nodes[seq].bridge[0]._uuid, g_ovsdb_nodes[seq].bridge[0].name);
                            add_tunnel(&g_ovsdb_nodes[seq], &g_ovsdb_nodes[seq].bridge[0]);
                            g_ovsdb_nodes[seq].bridge[0].is_using = TRUE;
                        }
                    }
                }
            }
        }
    }

    id = json_find_first_label(root, "id");
    if (id && id->child && id->child->text)
    {
        if (strncmp(id->child->text, "echo", 4) == 0)                   //echo
        {
            echo_reply_ovsdb(client_fd);
        }
        else if (strncmp(id->child->text, SEARCH_ALL_TABLE_ID, 1) == 0) //bridge
        {
            result = id->next;
            if (result)
            {
                handle_openvswitch_table(client_fd, seq, result);
                have_controller = handle_controller_table(client_fd, seq, result);
                handle_bridge_table(client_fd, seq, result, have_controller);
            }
        }
        json_free_value(&id);
    }

    json_free_value(&root);
}

INT4 get_json(const INT1 *json_str)
{
    INT4 parentheses_cnt_1 = 0;
    INT4 parentheses_cnt_2 = 0;
    INT4 length = 0;

    if((NULL == json_str) || (json_str[0] == '\0'))
    {
        return 0;
    }

    for(; json_str[length] != '\0'; length++)
    {
        switch(json_str[length])
        {
            case '{':
                parentheses_cnt_1++;
                break;
            case '[':
                parentheses_cnt_2++;
                break;
            case '}':
                parentheses_cnt_1--;
                if((parentheses_cnt_1 == 0) && (parentheses_cnt_2 == 0))
                {
                    length++;
                    goto EXIT;
                }
                break;
            case ']':
                parentheses_cnt_2--;
                if((parentheses_cnt_1 == 0) && (parentheses_cnt_2 == 0))
                {
                    length++;
                    goto EXIT;
                }
                break;
            default:
                break;
        }

    }

    length = 0;
EXIT:
    return length;
}


void *ovsdb_recv_msg(void *para)
{
    INT4 index;
    INT4 ret = 0;
    INT4 conn_fd = 0;
    struct timeval wait;
    wait.tv_sec = 5;
    wait.tv_usec = 0;
    UINT4 clientip = 0;    //�����ֽ���
    UINT2 client_port = 0;
    INT1 *p_str = NULL;
    INT4 len = 0;
    INT4 offset = 0;
    INT1 ovsdb_buff[OVSDB_BUFF_LEN + 1] = { 0 };
    INT1 json_string[10240] = {0};

    BOOL recv_flag = FALSE;
    INT4 recv_len = 0;

    fd_set recvmask;
    FD_ZERO(&g_ovsdb_recvmask);
    FD_SET(g_ovsdb_sockfd, &g_ovsdb_recvmask);


    while (1)
    {
        wait.tv_sec = 5;
        wait.tv_usec = 0;
        memcpy(&recvmask, &g_ovsdb_recvmask, sizeof(fd_set));
        ret = select(FD_SETSIZE, &recvmask, NULL, NULL, &wait);
        if (ret <= 0)
            continue;

        if (FD_ISSET(g_ovsdb_sockfd, &recvmask))
        {
            memset(&g_ovsdb_addr, 0, sizeof(struct sockaddr_in));
            socklen_t size = sizeof(struct sockaddr);

            conn_fd = accept(g_ovsdb_sockfd, (struct sockaddr *) &g_ovsdb_addr, &size);    //��������ʵֻacceptһ��
            clientip = *(UINT4 *) &g_ovsdb_addr.sin_addr;    //�����ֽ���
            client_port = *(UINT2 *) &g_ovsdb_addr.sin_port;

            LOG_PROC("INFO", "New OVSDB connected[%s:%d]", inet_htoa(ntohl(clientip)),ntohs(client_port));
            search_from_ovsdb_table_all(conn_fd);

            for (index = 0; index < OVSDB_MAX_CONNECTION; index++)
            {
                if (g_ovsdb_clients[index] == 0)
                {
                    g_ovsdb_clients[index] = conn_fd;
                    g_ovsdb_clients_bak[index] = conn_fd;
                    g_ovsdb_clients_ip[index] = clientip;

                    g_ovsdb_nodes[index].bridge[0].is_using = FALSE;
                    g_ovsdb_nodes[index].bridge[1].is_using = FALSE;
                    g_ovsdb_nodes[index].bridge[2].is_using = FALSE;

                    g_ovsdb_nodes[index].node_ip = clientip;
                    g_ovsdb_nodes[index].node_fd = conn_fd;

                    FD_SET(conn_fd, &g_ovsdb_recvmask);

                    break;
                }
            }

            if (index >= OVSDB_MAX_CONNECTION)    // exceed max connections.
            {
                LOG_PROC("ERROR", "Max OVSDB connected limited [%d]", OVSDB_MAX_CONNECTION);
                close(conn_fd);
            }
        }

        for (index = 0; index < OVSDB_MAX_CONNECTION; index++)
        {
            recv_len = 0;
            recv_flag = FALSE;

            if (g_ovsdb_clients[index])
            {
                if (FD_ISSET(g_ovsdb_clients[index], &recvmask))
                {
                    //һ����ȡBUFF_LEN���ֽڴ浽��Ӧ������Ļ�����
                    do
                    {
                        recv_len += read(g_ovsdb_clients[index], ovsdb_buff + recv_len, OVSDB_BUFF_LEN - recv_len);
                        if(recv_len > 0)
                        {
                            ovsdb_buff[recv_len] = '\0';
                            p_str = ovsdb_buff;
                            offset = 0;
                            recv_flag = FALSE;

                            do
                            {
                               len = get_json(p_str);
                               memcpy(json_string, p_str, len);
                               json_string[len] = '\0';
                               proc_ovsdb_msg(json_string, g_ovsdb_clients[index], g_ovsdb_clients_ip[index], index);

                               offset += len;
                               p_str += len;
                            }while(len);

                            if(recv_len != offset)
                            {
                                memmove(ovsdb_buff, ovsdb_buff + offset, recv_len - offset );
                                recv_len = recv_len - offset;
                                recv_flag = TRUE;
                            }
                        }
                        else
                        {
                            if (is_socket_dead(recv_len))
                            {
                                if (g_ovsdb_clients[index])
                                {
                                    FD_CLR(g_ovsdb_clients[index], &g_ovsdb_recvmask);
                                    close(g_ovsdb_clients[index]);
                                    g_ovsdb_clients[index] = 0;
                                }

                                recv_flag = FALSE;
                            }
                        }
                    }while(recv_flag);
                }
            }
        }
    }
}

INT4 ovsdb_connect_init()
{
    struct timeval wait;

    wait.tv_sec = 1;
    wait.tv_usec = 0;
    char * value = NULL;
    INT4 tcp_serv_fd = 0;
    UINT4 ssize = sizeof(struct sockaddr_in);

    struct sockaddr_in tcpsaddr;
    memset(&tcpsaddr, 0, sizeof(struct sockaddr_in));
    tcpsaddr.sin_family = AF_INET;
    tcpsaddr.sin_port = htons(g_ovsdb_port);    //htons(SERV_PORT);
    tcpsaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    value = get_value(g_controller_configure, "[ovsdb_conf]", "ovsdb_tunnel_on");
    g_ovsdb_turnel_on = (NULL == value)?1:atoi(value);

    tcp_serv_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcp_serv_fd == -1)
    {
        LOG_PROC("ERROR", "Create OVSDB server failed");
        return GN_ERR;
    }

    setsockopt(tcp_serv_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &wait, sizeof(wait));
    setsockopt(tcp_serv_fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &wait, sizeof(wait));
    setsockopt(tcp_serv_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &wait, sizeof(wait));

    if (bind(tcp_serv_fd, (struct sockaddr *) &tcpsaddr, ssize) == -1)
    {
        LOG_PROC("ERROR", "Create OVSDB server failed");
        close(tcp_serv_fd);
    }

    if (listen(tcp_serv_fd, OVSDB_MAX_CONNECTION) == -1)
    {
        LOG_PROC("ERROR", "Create OVSDB server failed");
        close(tcp_serv_fd);
    }

    LOG_PROC("INFO", "Create OVSDB server succeed, listening at [%d]", g_ovsdb_port);
    return tcp_serv_fd;
}

INT4 init_ovsdb()
{
    //��ʼ�����ӹ���
    g_ovsdb_sockfd = ovsdb_connect_init();
    if (g_ovsdb_sockfd < 0)
    {
        return GN_ERR;
    }

    //recv�߳�
    if (pthread_create(&g_ovsdb_recv_tid, NULL, ovsdb_recv_msg, NULL))
    {
        ovsdb_connect_quit();
        return GN_ERR;
    }

    return GN_OK;
}
