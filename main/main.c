#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"
#include "hc06.h"

#define BUTTON_POWER_PIN    14
#define BUTTON_DEFEND_PIN   15
#define BUTTON_ATTACK_PIN   16
#define BUTTON_THROW_PIN    17
#define ANALOG_MIRING_PIN   26
#define ANALOG_WALK_PIN     27
#define ANALOG_JUMP_PIN     28
#define LED_BT_CONNECTION   21

// Funções para inicializar os botões, LED e ADC
void init_buttons_led_adc() {
    gpio_init(BUTTON_POWER_PIN);
    gpio_init(BUTTON_DEFEND_PIN);
    gpio_init(BUTTON_ATTACK_PIN);
    gpio_init(BUTTON_THROW_PIN);
    gpio_init(LED_BT_CONNECTION);

    gpio_set_dir(BUTTON_POWER_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_DEFEND_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_ATTACK_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_THROW_PIN, GPIO_IN);
    gpio_set_dir(LED_BT_CONNECTION, GPIO_OUT);

    adc_init();
    adc_gpio_init(ANALOG_MIRING_PIN);
    adc_gpio_init(ANALOG_WALK_PIN);
    adc_gpio_init(ANALOG_JUMP_PIN);
}

// Função para ler o estado dos botões digitais
void read_digital_buttons(bool *power, bool *defend, bool *attack, bool *throw) {
    *power = !gpio_get(BUTTON_POWER_PIN);
    *defend = !gpio_get(BUTTON_DEFEND_PIN);
    *attack = !gpio_get(BUTTON_ATTACK_PIN);
    *throw = !gpio_get(BUTTON_THROW_PIN);
}

// Função para ler os valores dos controles analógicos
void read_analog_controls(uint16_t *miring, uint16_t *walk, uint16_t *jump) {
    *miring = adc_read();
    *walk = adc_read();
    *jump = adc_read();
}

// Função para controlar o LED de conexão Bluetooth
void set_bt_connection_led(bool connected) {
    gpio_put(LED_BT_CONNECTION, connected ? 1 : 0);
}

// Função para enviar comandos via Bluetooth
void send_command(const char *command) {
    uart_puts(HC06_UART_ID, command);
    uart_puts(HC06_UART_ID, "\n"); // Adiciona uma nova linha após o comando
}

// Tarefa para lidar com a comunicação via Bluetooth
void hc06_task(void *p) {
    uart_init(HC06_UART_ID, HC06_BAUD_RATE);
    gpio_set_function(HC06_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC06_RX_PIN, GPIO_FUNC_UART);
    hc06_init("nome_do_seu_dispositivo", "senha");

    while (true) {
        // Verifica se há dados recebidos no UART (Bluetooth)
        while (uart_is_readable(HC06_UART_ID)) {
            char received_char = uart_getc(HC06_UART_ID);
            // Processa os dados recebidos, se necessário
        }

        // Atualiza o estado do LED de conexão Bluetooth
        set_bt_connection_led(hc06_check_connection());

        // Pequeno delay para evitar leituras repetidas muito rápidas
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Função principal
int main() {
    stdio_init_all();

    printf("Iniciando controle do jogo\n");

    // Inicializa o módulo Bluetooth HC-06 em uma tarefa separada
    xTaskCreate(hc06_task, "Bluetooth_Task", 2048, NULL, 1, NULL);

    // Inicializa os botões digitais, LED e controles analógicos
    init_buttons_led_adc();

    bool power_pressed, defend_pressed, attack_pressed, throw_pressed;
    uint16_t miring_value, walk_value, jump_value;

    while (true) {
        // Lê o estado dos botões digitais
        read_digital_buttons(&power_pressed, &defend_pressed, &attack_pressed, &throw_pressed);

        // Lê os valores dos controles analógicos
        read_analog_controls(&miring_value, &walk_value, &jump_value);

        // Envio dos comandos correspondentes via Bluetooth
        if (power_pressed) {
            send_command("POWER");
        }
        if (defend_pressed) {
            send_command("DEFEND");
        }
        if (attack_pressed) {
            send_command("ATTACK");
        }
        if (throw_pressed) {
            send_command("THROW");
        }
        // Aqui você pode adaptar os comandos com base nos valores dos controles analógicos
        // Exemplo: controlar a mira, a velocidade de movimento, a altura do pulo, etc.

        // Pequeno delay para evitar leituras repetidas muito rápidas
        sleep_ms(50);
    }

    return 0;
}
