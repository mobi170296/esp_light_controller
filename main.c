#include <osapi.h>
#include <mem.h>
#include <user_interface.h>
#include <espconn.h>

#define BAUDRATE 115200
#define SSID "LIGHT CONTROLLER"
#define PASSWORD "Mg01669334569@"

#define RELAY_PIN 12
#define RELAY_ON 0
#define RELAY_OFF 1

os_timer_t timer;
struct espconn tcp_connection;
int value = 0;

struct HTTPRequestHeader
{
    char method[8];
    char url[128];
    char version[10];
};

bool HTTP_IsValidMethod(const char *method)
{
    return !(strcmp(method, "GET") && strcmp(method, "POST"));
}

bool HTTP_GetHTTPRequest(struct HTTPRequestHeader *header, const char *request)
{
    const char *pstart = request;
    char *pend;

    os_memset(header, 0, sizeof *header);

    if ((pend = strstr(pstart, " ")) && (pend - pstart < sizeof header->method))
    {
        os_memcpy(header->method, pstart, pend - pstart);
    }
    else
    {
        return false;
    }
    
    pstart = pend + 1;

    if ((pend = strstr(pstart, " ")) && (pend - pstart < sizeof header->url))
    {
        os_memcpy(header->url, pstart, pend - pstart);
    }
    else
    {
        return false;
    }

    pstart = pend + 1;
    
    if ((pend = strstr(pstart, "\r\n")) && (pend - pstart < sizeof header->version))
    {
        os_memcpy(header->version, pstart, pend - pstart);
    }
    else
    {
        return false;
    }
    
    return true;
}

int HTTP_BodyGetInt(const char *request, unsigned short length)
{
    int elength;
    char number[32];
    char *pstart = strstr(request, "\r\n\r\n");
    if (pstart)
    {
        elength = pstart - request;
        if (elength < length)
        {
            pstart += 4;
            os_memcpy(number, pstart, length - elength);
            number[sizeof number - 1] = '\0';
            if (!strcmp(number, "0"))
            {
                return 0;
            }
            else if (!strcmp(number, "1"))
            {
                return 1;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
}

void HTTP_ResponseControllerPage(struct espconn *connection)
{
    unsigned char *response = (unsigned char*)os_malloc(1024);
    unsigned char *body = (unsigned char*)os_malloc(1024);
    os_sprintf(body,
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>Light Controller</title>"
    "</head>"
    "<body>"
    "<div id=\"btn\" data-state=%d style=\"width: 50vh; height: 50vh; user-select: none; border-radius: 50%%; margin: 0px auto; background-color: %s; color: #fff; font-weight: bold; font-size: 20vh; text-align: center; line-height: 50vh; box-shadow: 0px 0px 50px 0px #999\">%s</div>"
    "</body>"
    "<script>"
    "waiting=false;"
    "btn=document.getElementById('btn');"
    "function setState(state){"
        "btn.style.backgroundColor=state?'#f00':'#0f0';"
        "btn.innerText=state?'OFF':'ON';"
        "btn.dataset.state=state?1:0;"
    "}"
    "btn.onclick=function(e){"
        "if(waiting){return};waiting=true;"
        "xhr = new XMLHttpRequest();"
        "xhr.open('POST', '/setstate');"
        "xhr.onreadystatechange=function(e){"
            "if(xhr.readyState==this.DONE){"
                "waiting=false;"
                "if(xhr.status=200&&parseInt(this.response)!=NaN){"
                    "setState(parseInt(this.responseText));"
                "}"
            "}"
        "};"
        "xhr.send(btn.dataset.state^1);"
    "}"
    "</script>"
    "</html>",
    GPIO_INPUT_GET(RELAY_PIN) == RELAY_ON ? 1 : 0,
    GPIO_INPUT_GET(RELAY_PIN) == RELAY_ON ? "#f00" : "#0f0",
    GPIO_INPUT_GET(RELAY_PIN) == RELAY_ON ? "OFF" : "ON"
    );

    os_sprintf(response, "HTTP/1.1 200 OK\r\nServer: ESP8266\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s", os_strlen(body), body);

    espconn_send(connection, response, os_strlen(response));

    os_free(response);
    os_free(body);
}

void HTTP_ResponseSetState(struct espconn *connection, int state)
{
    unsigned char *response = (unsigned char*)os_malloc(512);

    os_sprintf(response,
    "HTTP/1.1 200 OK\r\n"
    "Server: ESP8266\r\n"
    "Content-Length: 1\r\n"
    "\r\n"
    "%d",
    state
    );

    espconn_send(connection, response, os_strlen(response));

    os_free(response);
}

void app_init(void)
{
    // TODO...
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
}

void setup_wifi(void)
{
    struct softap_config wifi_config;
    struct ip_info ip_config;

    wifi_status_led_install(2, PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
    wifi_set_opmode_current(SOFTAP_MODE);

    os_strcpy(wifi_config.ssid, SSID);
    wifi_config.ssid_len = sizeof(SSID) - 1;
    wifi_config.ssid_hidden = 0;
    os_strcpy(wifi_config.password, PASSWORD);
    wifi_config.authmode = AUTH_WPA_WPA2_PSK;
    wifi_config.channel = 9;
    wifi_config.beacon_interval = 100;
    wifi_config.max_connection = 4;

    wifi_softap_set_config_current(&wifi_config);

    IP4_ADDR(&ip_config.ip, 9, 9, 9, 9);
    IP4_ADDR(&ip_config.gw, 9, 9, 9, 9);
    IP4_ADDR(&ip_config.netmask, 255, 255, 255, 0);

    wifi_set_ip_info(SOFTAP_IF, &ip_config);
}

void setup_dhcp(void)
{
    struct dhcps_lease lease;

    wifi_softap_dhcps_stop();

    lease.enable = 1;
    IP4_ADDR(&lease.start_ip, 9, 9, 9, 10);
    IP4_ADDR(&lease.end_ip, 9, 9, 9, 100);

    wifi_softap_set_dhcps_lease(&lease);

    wifi_softap_dhcps_start();
}

void disconnect_callback(void *arg)
{
    // os_printf("Disconnect callback\n");
}

void receive_callback(void *arg, char *pdata, unsigned short length)
{
    struct HTTPRequestHeader header;
    char *request = (char*)os_malloc(length + 1);

    os_memcpy(request, pdata, length);
    request[length] = '\0';

    if (HTTP_GetHTTPRequest(&header, request))
    {
        if (os_strcmp(header.url, "/setstate"))
        {
            // Send control page
            HTTP_ResponseControllerPage((struct espconn*)arg);
        }
        else
        {
            // Handle pin status
            switch (HTTP_BodyGetInt(request, length))
            {
                case 1:
                    GPIO_OUTPUT_SET(RELAY_PIN, RELAY_ON);
                break;
                case 0:
                    GPIO_OUTPUT_SET(RELAY_PIN, RELAY_OFF);
                break;
            }
            HTTP_ResponseSetState((struct espconn*)arg, GPIO_INPUT_GET(RELAY_PIN) == RELAY_ON ? 1 : 0);
        }
    }
    else
    {
        // os_printf("Cannot parse HTTPRequest\n");
    }

    os_free(request);

    espconn_disconnect((struct espconn*)arg);
}

void connect_callback(void *arg)
{
    // os_printf("Connect callback\n");
    struct espconn *connection = (struct espconn*)arg;
    espconn_regist_disconcb(connection, disconnect_callback);
    espconn_regist_recvcb(connection, receive_callback);
}

void setup_tcp_server(void)
{
    tcp_connection.type = ESPCONN_TCP;
    tcp_connection.state = ESPCONN_NONE;
    tcp_connection.proto.tcp = (esp_tcp*)os_zalloc(sizeof(esp_tcp));
    tcp_connection.proto.tcp->local_port = 80;
    espconn_regist_connectcb(&tcp_connection, connect_callback);

    if (espconn_accept(&tcp_connection))
    {
        // os_printf("Accept TCP Connection fail\n");
    }
    else
    {
        // os_printf("Accept TCP Connection success\n");
    }

    // if (espconn_regist_time(&tcp_connection, 180, 0))
    // {
    //     os_printf("Set TCP timeout fail\n");
    // }
    // else
    // {
    //     os_printf("Set TCP timeout success\n");
    // }
}

void user_init(void)
{
    system_init_done_cb(app_init);
    uart_div_modify(0, UART_CLK_FREQ / BAUDRATE);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
    GPIO_OUTPUT_SET(RELAY_PIN, RELAY_OFF);
    wifi_softap_dhcps_stop();
    setup_wifi();
    setup_dhcp();
    setup_tcp_server();
}
