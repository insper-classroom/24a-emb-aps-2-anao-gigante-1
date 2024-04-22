#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"
#include "hc06.h"


#define ANALOG_1_X 28
#define ADC_1_X 2
#define ANALOG_1_Y 27
#define ADC_1_Y 1

// #define BUTTON_POWER_PIN 14
// #define BUTTON_DEFEND_PIN 15
// #define BUTTON_ATTACK_PIN 16
// #define BUTTON_THROW_PIN 17
#define LED_BT_CONNECTION 15

// #define ANALOG_MIRING_PIN 26
// #define ANALOG_WALK_PIN 27
// #define ANALOG_JUMP_PIN 28

QueueHandle_t xQueueAdc;
#define WINDOW_SIZE 5

typedef struct adc {
    int axis;
    int val;
} adc_t;

typedef struct {
    int window[WINDOW_SIZE];
    int window_index;
} MovingAverage;

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF ;

    uart_putc_raw(uart0, data.axis); 
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb); 
    uart_putc_raw(uart0, -1); 
}

// Função para calcular a média móvel
int moving_average(MovingAverage *ma, int new_value) {
    // Adiciona o novo valor ao vetor de janela
    ma->window[ma->window_index] = new_value;

    // Atualiza o índice da janela circularmente
    ma->window_index = (ma->window_index + 1) % WINDOW_SIZE;

    // Calcula a média dos valores na janela
    int sum = 0;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        sum += ma->window[i];
    }
    return sum / WINDOW_SIZE;
}

void x_task(void *p) {
    adc_init();
    adc_gpio_init(ANALOG_1_X); 
    MovingAverage ma = {0};   
    while (1) {
        adc_select_input(ADC_1_X);
        int x = adc_read();

        struct adc x_base = {0,x};
        xQueueSend(xQueueAdc, &x_base, portMAX_DELAY);

        moving_average(&ma, x);
        printf("X: %d\n", x);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p) {
    adc_init();
    adc_gpio_init(ANALOG_1_Y);
    MovingAverage ma = {0};

    while (1) {
        adc_select_input(ADC_1_Y);
        int y = adc_read();

        struct adc y_base = {1,y};
        xQueueSend(xQueueAdc, &y_base, portMAX_DELAY);

        moving_average(&ma, y);
        printf("Y: %d\n", y);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void uart_task(void *p) {
    adc_t data;

    while (1) {
        xQueueReceive(xQueueAdc, &data, portMAX_DELAY);

        data.val = (data.val-2047)/8;
        int zone_limit = 165;
        if (data.val <=zone_limit && data.val >= -1*(zone_limit)) {
            data.val = 0;
        }
        printf("Axis: %d, Value: %d\n", data.axis, data.val);
        write_package(data);
    }
}






// Funções para inicializar os botões, LED e ADC
void init_buttons_led_adc() {
    // gpio_init(BUTTON_POWER_PIN);
    // gpio_init(BUTTON_DEFEND_PIN);
    // gpio_init(BUTTON_ATTACK_PIN);
    // gpio_init(BUTTON_THROW_PIN);
    gpio_init(LED_BT_CONNECTION);

    // gpio_set_dir(BUTTON_POWER_PIN, GPIO_IN);
    // gpio_set_dir(BUTTON_DEFEND_PIN, GPIO_IN);
    // gpio_set_dir(BUTTON_ATTACK_PIN, GPIO_IN);
    // gpio_set_dir(BUTTON_THROW_PIN, GPIO_IN);
    // gpio_set_dir(LED_BT_CONNECTION, GPIO_OUT);

    adc_init();
    // adc_gpio_init(ANALOG_MIRING_PIN);
    // adc_gpio_init(ANALOG_WALK_PIN);
    // adc_gpio_init(ANALOG_JUMP_PIN);
    adc_gpio_init(ANALOG_1_X);
    adc_gpio_init(ANALOG_1_Y);
}

// Função para ler o estado dos botões digitais
// void read_digital_buttons(bool *power, bool *defend, bool *attack, bool *throw) {
//     *power = !gpio_get(BUTTON_POWER_PIN);
//     *defend = !gpio_get(BUTTON_DEFEND_PIN);
//     *attack = !gpio_get(BUTTON_ATTACK_PIN);
//     *throw = !gpio_get(BUTTON_THROW_PIN);
// }


// Função para controlar o LED de conexão Bluetooth
void set_bt_connection_led(bool connected) {
    gpio_put(LED_BT_CONNECTION, connected ? 1 : 0);
}

// Função para ler os valores dos controles analógicos
void read_analog_walk(uint16_t *walk, uint16_t *jump) {
    *walk = adc_read();
    *jump = adc_read();
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
    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(x_task, "x_task", 256, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 256, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    vTaskStartScheduler();

    while (true);




    // stdio_init_all();
    // printf("Iniciando controle do jogo\n");
    // // Inicializa o módulo Bluetooth HC-06 em uma tarefa separada
    // xTaskCreate(hc06_task, "Bluetooth_Task", 2048, NULL, 1, NULL);
    // // Inicializa os botões digitais, LED e controles analógicos
    // init_buttons_led_adc();
    // uint16_t walk_value, jump_value;

    // while (true) {
    //     // Lê os valores dos controles analógicos
    //     read_analog_controls(&walk_value, &jump_value);

    //     // Envio dos comandos correspondentes via Bluetooth
    //         // if (power_pressed) {
    //         //     send_command("POWER");
    //         // }
    //         // if (defend_pressed) {
    //         //     send_command("DEFEND");
    //         // }
    //         // if (attack_pressed) {
    //         //     send_command("ATTACK");
    //         // }
    //         // if (throw_pressed) {
    //         //     send_command("THROW");
    //         // }

    //     // // Aqui você pode adaptar os comandos com base nos valores dos controles analógicos
    //     // Exemplo: controlar a mira, a velocidade de movimento, a altura do pulo, etc.


    //     // Pequeno delay para evitar leituras repetidas muito rápidas
    //     sleep_ms(50);
    // }

    return 0;
}
