#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/bootrom.h"
#include "hardware/pwm.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lib/ssd1306.h" 
#include "lib/ws2812.pio.h"
#include "lib/matrix.h"

// ***** MACROS *****
#define UART_ID uart0
#define UART_TX 0
#define UART_RX 1
#define BAUD_RATE 115200
#define BUZZER_PIN 21 // pino do buzzer
#define IS_RGBW false
#define BUZZER_FREQ_HIGH 2000 // Frequência alta para estado negativo
#define BUTTON_A_PIN 5 // Pino btn A
#define BUTTON_B_PIN 6 // Pino btn B
#define LED_PIN 12 // led
#define WIFI_SSID "G3" // local onde vc declara o nome da rede(protocolo http)
#define WIFI_PASS "marciaeglaube" // senha da rede

// Faixas ideais dos sensores
#define PH_MIN 5.5
#define PH_MAX 6.5
#define COND_MIN 1.2
#define COND_MAX 2.5
#define TEMP_MIN 20.0
#define TEMP_MAX 25.0
#define NIVEL_IDEAL 1.0

// Definições do I2C para o display OLED
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

// Variáveis globais para armazenar as leituras
float ph = 0.0, cond = 0.0, temp = 0.0, nivel = 0.0;
bool status_ok = true;

// Função para inicializar o buzzer como PWM
void configurar_buzzer() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, (float)clock_get_hz(clk_sys) / (BUZZER_FREQ_HIGH * 4096.0));
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

// Função para tocar o buzzer
void tocar_buzzer(int duracao_ms) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_gpio_level(BUZZER_PIN, 2048); // 50% duty cycle
    sleep_ms(duracao_ms);
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

// Função para atualizar matriz de LEDs e determinar status
bool atualizar_matriz(PIO pio, int sm, float ph, float cond, float temp, float nivel) {
    uint32_t cor = 0xFF0000; // Verde (valores corretos)
    bool status_ok = true;

    if (ph < PH_MIN || ph > PH_MAX || cond < COND_MIN || cond > COND_MAX || temp < TEMP_MIN || temp > TEMP_MAX || nivel < NIVEL_IDEAL) {
        cor = 0x00FF00; // Vermelho (fora da faixa ideal)
        status_ok = false;
        tocar_buzzer(1500);
    }

    for (int i = 0; i < 49; i++) {
        pio_sm_put_blocking(pio, sm, cor << 8);
    }

    return status_ok;
}

// função simulação de leituras alternadas
void simular_leitura_alternada(float *ph, float *cond, float *temp, float *nivel, bool *alternar) {
    if (*alternar) {
        // Valores dentro das faixas ideais (Tudo Ok)
        *ph = 6.0; // pH ideal
        *cond = 1.8; // Condutividade ideal
        *temp = 22.0; // Temperatura ideal
        *nivel = 1.0; // Nível ideal
    } else {
        // Valores fora das faixas ideais (Fora do Padrão)
        *ph = 4.5; // pH baixo
        *cond = 0.8; // Condutividade baixa
        *temp = 30.0; // Temperatura alta
        *nivel = 0.0; // Nível baixo
    }
    *alternar = !(*alternar); // Alterna o estado para a próxima leitura
}

// Função de callback para processar requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        // Cliente fechou a conexão
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Processa a requisição HTTP
    char *request = (char *)p->payload;

    if (strstr(request, "GET /led/on")) {
        gpio_put(LED_PIN, 1);  // Liga o LED
    } else if (strstr(request, "GET /led/off")) {
        gpio_put(LED_PIN, 0);  // Desliga o LED
    }

    // Cria a resposta HTTP com os valores atuais
    char response[512];
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
        "<!DOCTYPE html><html><body>"
        "<h1>Monitoramento</h1>"
        "<p>pH: %.1f</p>"
        "<p>Condutividade: %.1f</p>"
        "<p>Temperatura: %.1f°C</p>"
        "<p>Nível: %.0f</p>"
        "<p>STATUS: %s</p>"
        "<p><a href=\"/led/on\">Ligar LED</a></p>"
        "<p><a href=\"/led/off\">Desligar LED</a></p>"
        "</body></html>\r\n",
        ph, cond, temp, nivel, status_ok ? "Tudo Ok" : "Fora do Padrão");

    // Envia a resposta HTTP
    tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);

    // Libera o buffer recebido
    pbuf_free(p);

    return ERR_OK;
}

// Callback de conexão: associa o http_callback à conexão
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);  // Associa o callback HTTP
    return ERR_OK;
}

// Função de setup do servidor TCP
static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }

    // Liga o servidor na porta 80
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);  // Coloca o PCB em modo de escuta
    tcp_accept(pcb, connection_callback);  // Associa o callback de conexão

    printf("Servidor HTTP rodando na porta 80...\n");
}

int main() {
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_RX, GPIO_FUNC_UART);

    configurar_buzzer();

    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    // Inicialização do I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000); // Inicializa o I2C com frequência de 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicialização do display OLED
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, 0x3C, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Inicialização do Wi-Fi
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return 1;
    } else {
        printf("Connected.\n");
        // Exibe o endereço IP no console
        uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("Endereço IP %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }

    printf("Wi-Fi conectado!\n");
    printf("Para monitorar e controlar o LED, acesse o Endereço IP no navegador\n");

    // Configura o LED como saída
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Inicia o servidor HTTP
    start_http_server();

    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    bool running = false;
    bool alternar = true; 

    while (true) {
        if (!gpio_get(BUTTON_A_PIN)) {
            running = true;
            sleep_ms(200);
        }

        if (!gpio_get(BUTTON_B_PIN)) {
            running = false;
            sleep_ms(200);
        }

        if (running) {
            simular_leitura_alternada(&ph, &cond, &temp, &nivel, &alternar);
            status_ok = atualizar_matriz(pio, sm, ph, cond, temp, nivel);
            printf("pH: %.1f, Condutividade: %.1f, Temperatura: %.1f°C, Nível: %.0f, STATUS: %s\n", 
                   ph, cond, temp, nivel, status_ok ? "Tudo Ok" : "Fora do Padrão");

            // Exibe o status no display OLED
            char status_str[32];
            snprintf(status_str, sizeof(status_str), "STATUS"); // Texto fixo "STATUS"
            ssd1306_fill(&ssd, false); // Limpa o display

            // Escreve "STATUS" na parte superior (y = 0)
            ssd1306_draw_string(&ssd, status_str, 0, 0);

            // Escreve "TUDO OK" ou "FORA DO PADRÃO" no meio do display (y = 24)
            if (status_ok) {
                ssd1306_draw_string(&ssd, "TUDO OK", 0, 24);
            } else {
                ssd1306_draw_string(&ssd, "FORA DO PADRAO", 0, 24);
            }

            ssd1306_send_data(&ssd); // Envia os dados para o display

            sleep_ms(5000); // Espera de 5 segundos entre as leituras
        } else {
            sleep_ms(100);
        }

        cyw43_arch_poll();  // Necessário para manter o Wi-Fi ativo
    }

    cyw43_arch_deinit();  // Desliga o Wi-Fi (não será chamado, pois o loop é infinito)
    return 0;
}