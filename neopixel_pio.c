#include <stdio.h>       // Para funções padrão de entrada e saída
#include <string.h>      // Para manipulação de strings (memset)
#include <setjmp.h>     // Para usar longjmp e setjmp
#include <time.h>        // Para funções relacionadas ao tempo
#include "pico/stdlib.h" // Para funções básicas do Raspberry Pi Pico
#include "hardware/i2c.h" // Para comunicação I2C
#include "hardware/gpio.h" // Para manipulação de GPIOs
#include "hardware/adc.h"  // Para uso do ADC no joystick
#include "ws2818b.pio.h"  // Para uso dos LEDs WS2812B
#include "inc/ssd1306.h"  // Para uso do display OLED
#include "hardware/pwm.h" // para controlar PWM
#include "pico/cyw43_arch.h" 
#include "lwip/tcp.h"

#define I2C_PORT i2c1 // Define a porta I2C a ser utilizada
#define SDA_PIN 14    // Define o pino SDA
#define SCL_PIN 15   // Define o pino SCL
#define JOY_X 27   // Define o pino do eixo X do joystick
#define JOY_Y 26  // Define o pino do eixo Y do joystick
#define JOY_SW 22 // Define o pino do botão do joystick
#define BTN_A 5  // Define o pino do botão A
#define BTN_B 6  // Define o pino do botão B
#define MIC_PIN 28 // Define o pino do microfone

#define LED_COUNT 25 // Define a quantidade de LEDs
#define LED_PIN 7   // Define o pino do LED

#define DEBOUNCE_DELAY 50 // Tempo de debouncing em ms
uint8_t ssd[ssd1306_buffer_length]; // Buffer para o display OLED

volatile bool buttonAPressed = false; // Variável para armazenar o estado do botão A
volatile bool buttonBPressed = false; // Variável para armazenar o estado do botão B

// Posição do jogador (desvioPerfeito)
int posX = 0;
int posY = 4;

jmp_buf resetPoint; // Buffer para armazenar o ponto de retorno
int buttonBClickCount = 0; // Contador de cliques do botão B
uint32_t lastButtonBPressTime = 0; // Tempo do último clique do botão B

#define JOY_CENTER 2048 // Define valor central do analógico
#define JOY_THRESHOLD 500 // Sensibilidade do joystick

#define WIFI_SSID "LACERDA 2.4G"
#define WIFI_PASS "7223480CBD"
//WI-FI


// Função de interrupção para os botões A e B
void buttonISR(uint gpio, uint32_t events) {
    static uint32_t lastPressTimeA = 0;
    static uint32_t lastPressTimeB = 0;
    uint32_t currentTime = to_ms_since_boot(get_absolute_time());

    if (gpio == BTN_A) {
        if ((currentTime - lastPressTimeA) > DEBOUNCE_DELAY && gpio_get(BTN_A) == 0) {
            // Espera o botão ser solto
            while (gpio_get(BTN_A) == 0);
            printf("botão pressionado");
            buttonAPressed = true;
            lastPressTimeA = currentTime;
        }
    }

    if (gpio == BTN_B) {
        if ((currentTime - lastPressTimeB) > DEBOUNCE_DELAY && gpio_get(BTN_B) == 0) {
            // Espera o botão ser solto
            while (gpio_get(BTN_B) == 0);
            printf("botão pressionado");
            buttonBPressed = true;
            lastPressTimeB = currentTime;
        }
    }
}

//FORMULARIO

void ConfigQuestionario();

void remover_quebra_linha(char *str) {
    str[strcspn(str, "\r\n")] = '\0'; // Remove \r ou \n do final
}

int obter_resposta(const char *pergunta) {
    char entrada[10];
    int resposta;

    do {
        printf("%s\n(0) Nenhum impacto  (5) Moderado  (10) Impacto total\n", pergunta);
        
        if (fgets(entrada, sizeof(entrada), stdin) == NULL) {
            printf("Erro na leitura. Tente novamente.\n");
            continue;
        }

        remover_quebra_linha(entrada);
        resposta = atoi(entrada); // Converte string para inteiro

        if (resposta < 0 || resposta > 10) {
            printf("Entrada inválida! Digite um número entre 0 e 10.\n");
            resposta = -1; // Força repetição do loop
        }

    } while (resposta < 0 || resposta > 10);
    
    return resposta;
}

void display_message(const char *line1, const char *line2);
void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b);
void npWrite();
void npClear();

void questionario() {
    int respostas[10];
    const char *perguntas[] = {
        "De 0 a 10, quanto seus pensamentos estão controlando sua vida?",
        "De 0 a 10, quanto seus pensamentos atrapalham suas atividades diárias?",
        "De 0 a 10, quanto tempo do seu dia é consumido por preocupações?",
        "De 0 a 10, o quanto você sente que está no controle dos seus pensamentos?",
        "De 0 a 10, o quanto suas preocupações impedem você de aproveitar momentos de lazer?",
        "De 0 a 10, o quanto você sente que sua mente está constantemente ocupada com pensamentos repetitivos?",
        "De 0 a 10, o quanto seus pensamentos afetam sua concentração no trabalho ou estudo?",
        "De 0 a 10, o quanto seus pensamentos dificultam sua capacidade de tomar decisões?",
        "De 0 a 10, o quanto você sente que seus pensamentos afetam seu bem-estar emocional?",
        "De 0 a 10, o quanto você gostaria de ter mais controle sobre seus pensamentos?"
    };

    printf("\n--- Questionário sobre o Impacto dos Pensamentos na Vida Cotidiana ---\n");
    printf("Responda com base na última semana.\n\n");

    for (int i = 0; i < 10; i++) {
        respostas[i] = obter_resposta(perguntas[i]);
    }

    printf("\n--- Respostas Registradas ---\n");
    for (int i = 0; i < 10; i++) {
        printf("%s: %d\n", perguntas[i], respostas[i]);
    }

    display_message("Respostas", "Recebidas");
    sleep_ms(2000);
    display_message("Obrigado", "e melhoras");
    sleep_ms(2000);
    uint8_t brightness = 255;
    uint8_t colorR = 0, colorG = brightness, colorB = brightness; // Ciano
    uint indices[] = {16, 18, 14, 6, 7, 8, 10};
    for (uint i = 0; i < sizeof(indices) / sizeof(indices[0]); i++) {
        npSetLED(indices[i], colorR, colorG, colorB);
    }
    npWrite();
    sleep_ms(5000);
    npClear();
    npWrite();
    display_message("   ", "   ");
    longjmp(resetPoint, 1);
}


// MATRIZ DE LED 5x5

struct pixel_t {
    uint8_t G, R, B;
};

typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;
npLED_t leds[LED_COUNT];
PIO np_pio;
uint sm;

// Inicialização do PIO e configuração do LED
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
      np_pio = pio1;
      sm = pio_claim_unused_sm(np_pio, true);
    }
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
    for (uint i = 0; i < LED_COUNT; ++i) {
      leds[i].R = 0;
      leds[i].G = 0;
      leds[i].B = 0;
    }
}

// Definir a cor de um LED específica
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

// Escrever as cores dos LEDs
void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
      pio_sm_put_blocking(np_pio, sm, leds[i].G);
      pio_sm_put_blocking(np_pio, sm, leds[i].R);
      pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

// Limpar os LEDs
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        npSetLED(i, 0, 0, 0);
   }
}

// Percorre os LEDs de trás para frente (do último para o primeiro)
void cycleLEDs() {
    for (int i = LED_COUNT - 1; i >= 0; --i) {
      npClear();
      npSetLED(i, 0, 255, 0);
      npWrite();
      sleep_ms(100);
    }
    npClear();
    npWrite(); 
}


// PAINEL OLED

// Área de renderização
struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

// Função para exibir uma mensagem no display
void display_message(const char *line1, const char *line2) {
    memset(ssd, 0, ssd1306_buffer_length); // Limpa o buffer do display

    // Calcula a posição horizontal para centralizar o texto
    int x1 = (ssd1306_width - strlen(line1) * 6) / 2; 
    int x2 = (ssd1306_width - strlen(line2) * 6) / 2; 

    int font_height = 8; // Altura da fonte em pixels
    int line_spacing = 12; // Espaçamento adicional entre as linhas (aumente ou diminua conforme necessário)
    int total_height = 2 * font_height + line_spacing; // Altura total das duas linhas com espaçamento
    int y1 = (ssd1306_height - total_height) / 2; // Posição vertical da primeira linha
    int y2 = y1 + font_height + line_spacing; // Posição vertical da segunda linha (com espaçamento adicional)

    // Desenha as strings no buffer
    ssd1306_draw_string(ssd, x1, y1, line1); 
    ssd1306_draw_string(ssd, x2, y2, line2); 

    render_on_display(ssd, &frame_area); // Renderiza o buffer no display
}


void ControleSerialQuestionario() {
    display_message("Conecte o USB", "B para continuar");
    buttonBPressed = false;
    sleep_ms(100); 

    uint8_t brightness = 255;
    uint8_t colorR = 0, colorG = brightness, colorB = brightness; // Ciano

    uint indices[] = {22, 17, 18, 10, 16, 14, 12, 7, 2};
    for (uint i = 0; i < sizeof(indices) / sizeof(indices[0]); i++) {
        npSetLED(indices[i], colorR, colorG, colorB);
    }

    npWrite();
    while (1) {
        if (buttonBPressed) {
            buttonBPressed = false; 

            for (uint i = 0; i < sizeof(indices) / sizeof(indices[0]); i++) {
                npSetLED(indices[i], 0, 0, 0);
            }
            npWrite();

            display_message("Iniciar", "Monitor Serial");

            questionario();
            
            return; 
        }
    }
}




// REFLEXO ZEN:

//Função para ativar RGB central da placa
void setRGBColor(uint8_t r, uint8_t g, uint8_t b) {
    gpio_put(13, r); 
    gpio_put(11, g); 
    gpio_put(12, b); 
}

// Função principal do jogo
void reflexoZenGame() {
    buttonBPressed = false;
    uint8_t colors[3][3] = {
        {1, 0, 0}, 
        {0, 1, 0}, 
        {0, 0, 1}  
    };
    int currentColor = 0;
    int fixedColor = 0;
    int speed = 1000; 

    sleep_ms(300); 

    bool pauseColorChange = false; 
    npSetLED(12, colors[fixedColor][0] * 255, colors[fixedColor][1] * 255, colors[fixedColor][2] * 255);
    npWrite();
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &buttonISR);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &buttonISR);
    while (1) {
        if (buttonAPressed) {
            buttonAPressed = false; 
            int previousColor = (currentColor - 1 + 3) % 3; 
            bool isMatch = (colors[previousColor][0] == leds[12].R / 255 &&
                           colors[previousColor][1] == leds[12].G / 255 &&
                           colors[previousColor][2] == leds[12].B / 255);
            npClear();
            for (int i = 0; i < LED_COUNT; i++) {
                if (isMatch) {
                    npSetLED(i, 0, 255, 0);
                } else {
                    npSetLED(i, 255, 0, 0); 
                }
            }
            npWrite();
            sleep_ms(2000); 
            npClear();
            npWrite();
            sleep_ms(500); 

            if (isMatch) {
                speed = (speed - 100 > 200) ? (speed - 100) : 200; 
            } else {
                speed = 1000;
            }

            fixedColor = (fixedColor + 1) % 3; 
            npSetLED(12, colors[fixedColor][0] * 255, colors[fixedColor][1] * 255, colors[fixedColor][2] * 255);
            npWrite();
            currentColor = previousColor; 
            pauseColorChange = true; 
            sleep_ms(200); 
        }

        setRGBColor(colors[currentColor][0], colors[currentColor][1], colors[currentColor][2]);
        display_message("Reflexo Zen", "B para continuar");

        if (!pauseColorChange) {
            currentColor = (currentColor + 1) % 3; 
        } else {
            pauseColorChange = false; 
        }


        if (buttonBPressed) {
            buttonBPressed = false;
            setRGBColor(0, 0, 0); 
            ConfigQuestionario();
            return;
        }

        sleep_ms(speed);
    }
}

//DESVIO PERFEITO

//Mapeamento da matriz de led
const int matriz[5][5] = {
    {24, 23, 22, 21, 20}, // LINHA 1
    {15, 16, 17, 18, 19}, // LINHA 2
    {14, 13, 12, 11, 10}, // LINHA 3
    {5,  6,  7,  8,  9},  // LINHA 4
    {4,  3,  2,  1,  0}   // LINHA 5
};

// Obstáculos (no máximo 3 simultâneos)
typedef struct {
    int x, y;
    bool ativo;
} Obstacle;

Obstacle obstacles[3];

// Inicializa obstáculos
void initObstacles() {
    for (int i = 0; i < 3; i++) {
        obstacles[i].ativo = false;
    }
}

// Cria um novo obstáculo aleatório no topo
void spawnObstacle() {
    for (int i = 0; i < 3; i++) {
        if (!obstacles[i].ativo) {
            obstacles[i].x = rand() % 5; // Posição aleatória na linha de cima
            obstacles[i].y = 0;
            obstacles[i].ativo = true;
            break;
        }
    }
}

// Move obstáculos para baixo
void updateObstacles() {
    for (int i = 0; i < 3; i++) {
        if (obstacles[i].ativo) {
            obstacles[i].y++; 
            if (obstacles[i].y > 4) {
                obstacles[i].ativo = false; 
            }
        }
    }
}

// Verifica colisão entre jogador e obstáculos
bool checkCollision() {
    for (int i = 0; i < 3; i++) {
        if (obstacles[i].ativo && posX == obstacles[i].x && posY == obstacles[i].y) {
            return true;
        }
    }
    return false;
}

//Função para indicar a colisão
void blinkCollisionLEDs() {
    int collision_leds[] = {20, 18, 12, 6, 4, 24, 16, 8, 0};
    int num_leds = 9;
    
    // Pisca os LEDs em verde
    for (int i = 0; i < num_leds; i++) {
        npSetLED(collision_leds[i], 255, 0, 0);
    }
    npWrite();
    sleep_ms(1000); // Mantém aceso por 1 segundo
    npClear();
    npWrite();
}

// Função principal do jogo
void desvioPerfeito() {
    sleep_ms(100); 
    printf("Entrando em desvioPerfeito()\n");
    display_message("Desvio Perfeito", "B para continuar");
    buttonBPressed = false;
    // Inicializa ADC do joystick
    adc_init();
    adc_gpio_init(JOY_X);
    adc_gpio_init(JOY_Y);

    printf("ADC inicializado com sucesso.\n");

    posX = 0;
    posY = 4;
    printf("Inicializando obstáculos...\n");
    initObstacles();
    printf("Obstáculos inicializados com sucesso.\n");

    uint32_t last_update = 0;
    uint32_t last_speed_increase = 0;
    uint32_t obstacle_speed = 300; // Tempo inicial de atualização
    const uint32_t SPEED_INCREASE_INTERVAL = 10000; // A cada 10 segundos

    while (1) {
        if (buttonBPressed) {
            buttonBPressed = false; // Reseta a flag do botão
            npClear();
            npWrite(); 

            ConfigQuestionario();  //Chama a próxima função
            return; // Sai da função desvioPerfeito()
        }
        adc_select_input(1);
        float joyX = (float)adc_read();
        adc_select_input(0);
        float joyY = (float)adc_read();

        printf("Valores do joystick: joyX = %.2f, joyY = %.2f\n", joyX, joyY);

        int moveX = 0;
        int moveY = 0;

        if (joyY > JOY_CENTER + 500) moveY = -1; 
        if (joyY < JOY_CENTER - 500) moveY = 1; 
        if (joyX > JOY_CENTER + 500) moveX = 1;  
        if (joyX < JOY_CENTER - 500) moveX = -1;

        posX += moveX;
        posY += moveY;

        if (posX < 0) posX = 0;
        if (posX > 4) posX = 4;
        if (posY < 0) posY = 0;
        if (posY > 4) posY = 4;

        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - last_speed_increase >= SPEED_INCREASE_INTERVAL) {
            obstacle_speed = (obstacle_speed > 100) ? obstacle_speed - 50 : 100;
            last_speed_increase = current_time;
            printf("Velocidade dos obstáculos aumentada: %d ms\n", obstacle_speed);
        }

        if (current_time - last_update >= obstacle_speed) {
            if (rand() % 20 == 0) {
                spawnObstacle();
                printf("Novo obstáculo gerado.\n");
            }
            updateObstacles();
            last_update = current_time;
        }

        // Verifica colisão
        if (checkCollision()) {
            printf("Colisão detectada!\n");
            blinkCollisionLEDs();
            posX = 0;
            posY = 4;
            initObstacles();
            obstacle_speed = 300;
            last_speed_increase = current_time;
            continue;
        }
        npClear();
        int ledIndex = matriz[posY][posX];
        npSetLED(matriz[posY][posX], 0, 255, 0);
        printf("LED atualizado: posX = %d, posY = %d, índice = %d\n", posX, posY, ledIndex);
        for (int i = 0; i < 3; i++) {
            if (obstacles[i].ativo) {
                npSetLED(matriz[obstacles[i].y][obstacles[i].x], 255, 0, 0);
            }
        }
        npWrite();
        sleep_ms(100); 
    }

    printf("Saindo de desvioPerfeito()\n");
}

//CICLO DE RESPIRAÇÃO

//Som Binaural para relaxamento
void playBinauralSound() {
    uint base_freq = 200; // Defina a frequência base desejada
    uint left_freq = base_freq;
    uint right_freq = base_freq + 4; // Diferença de 4 Hz para o batimento binaural

    gpio_set_function(10, GPIO_FUNC_PWM);
    gpio_set_function(21, GPIO_FUNC_PWM);

    uint slice_left = pwm_gpio_to_slice_num(10);
    uint slice_right = pwm_gpio_to_slice_num(21);

    uint clock_div = 125;
    uint wrap_left = 125000000 / (clock_div * left_freq);
    uint wrap_right = 125000000 / (clock_div * right_freq);

    pwm_set_wrap(slice_left, wrap_left);
    pwm_set_wrap(slice_right, wrap_right);

    pwm_set_chan_level(slice_left, PWM_CHAN_A, wrap_left * 2);  // 50% duty cycle
    pwm_set_chan_level(slice_right, PWM_CHAN_A, wrap_right * 2);

    pwm_set_enabled(slice_left, true);
    pwm_set_enabled(slice_right, true);
}

// Parar som Binaural
void stopBinauralSound() {
    uint slice_left = pwm_gpio_to_slice_num(10);
    uint slice_right = pwm_gpio_to_slice_num(21);
    
    // Reduzir duty cycle para 0 antes de desativar PWM
    pwm_set_chan_level(slice_left, PWM_CHAN_A, 0);
    pwm_set_chan_level(slice_right, PWM_CHAN_A, 0);
    
    pwm_set_enabled(slice_left, false);
    pwm_set_enabled(slice_right, false);
    
    // Resetar os pinos para garantir que o som pare completamente
    gpio_set_function(10, GPIO_FUNC_SIO);
    gpio_set_function(21, GPIO_FUNC_SIO);
    gpio_set_dir(10, GPIO_OUT);
    gpio_set_dir(21, GPIO_OUT);
    gpio_put(10, 0);
    gpio_put(21, 0);
}

// (preenchido com -1 para os LEDs não utilizados)
int numbers[8][13] = {
    {23, 22, 21, 18, 16, 13, 12, 11, 8, 1, 2, 3, 6}, // 8
    {23, 22, 21, 18, 11, 8, 1, -1, -1, -1, -1, -1, -1}, // 7 
    {23, 22, 21, 16, 13, 12, 11, 8, 6, 3, 2, 1, -1}, // 6
    {23, 22, 21, 16, 13, 12, 11, 8, 3, 2, 1, -1, -1}, // 5
    {23, 21, 18, 16, 13, 12, 11, 8, 1, -1, -1, -1, -1}, // 4
    {23, 22, 21, 18, 11, 12, 13, 8, 1, 2, 3, -1, -1}, // 3
    {23, 22, 21, 18, 11, 12, 13, 6, 3, 2, 1, -1, -1}, // 2
    {22, 16, 17, 12, 7, 2, -1, -1, -1, -1, -1, -1, -1}  // 1
};

// Função para exibir um número na matriz de LEDs
void displayNumber(int number) {
    playBinauralSound();
    npClear();
    int *leds = numbers[8 - number];
    int size = sizeof(numbers[8 - number]) / sizeof(numbers[8 - number][0]);
    for (int i = 0; i < size; i++) {
        if (leds[i] != -1) {  // Ignora valores -1
            npSetLED(leds[i], 255, 0, 0);
        }
    }
    npWrite();
    sleep_ms(1000); // Agora cada número dura 1s (totalizando 8s)
}

// Ler o nível de áudio de forma suavizada
uint16_t readSmoothedAudioLevel() {
    uint32_t sum = 0;
    const uint8_t samples = 20;
    for (int i = 0; i < samples; i++) {
        sum += adc_read();
        sleep_us(100);
    }
    return sum / samples;
}

// Função para detectar o nível de áudio da respiração
void detectAudioForDuration(uint16_t duration_ms) {
    stopBinauralSound(); 
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(2);

    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while (to_ms_since_boot(get_absolute_time()) - start_time < duration_ms) {
        uint16_t audio_level = readSmoothedAudioLevel();
        uint16_t threshold = 2100;  // Ajuste esse valor conforme necessário
        uint8_t brightness = (audio_level > threshold) ? (audio_level * 64 / 4096) : 0; // Ajuste o brilho conforme o nível de áudio
        int height = (audio_level > threshold) ? (audio_level * 8 / 4096) : 0; 
        npClear();
        if (audio_level > threshold) {
            for (int layer = 0; layer < height; layer++) {
                for (int i = layer * 5; i < (layer + 1) * 5; i++) {
                    uint8_t r = 0; 
                    uint8_t g = brightness; // Brilho baseado no nível de áudio
                    uint8_t b = 0; 
                    npSetLED(i, r, g, b);
                }
            }
            npWrite();
        } else {
            npClear(); 
            npWrite();
        }
        sleep_ms(20);
    }
    npClear();
    sleep_ms(20);
}

// Protótipo da função para evitar erro de declaração implícita
void breathActivityLoop();

// Função para controlar a decisão de repetir ou continuar
void controlBreathCycle() {
    display_message("Repetir: Clique A", "Continuar: Clique B");

    buttonAPressed = false;
    buttonBPressed = false;

    while (!buttonAPressed && !buttonBPressed) {
        printf("Aguardando botão A ou B...\n");
        sleep_ms(100);
    }

    if (buttonAPressed) {
        buttonAPressed = false; 
        breathActivityLoop();
    } else if (buttonBPressed) {
        srand(time(NULL));
        int randomValue = rand() % 2;
        buttonBPressed = false;
        printf("Valor gerado por rand() %% 2: %d\n", randomValue);
        if (randomValue) {
            reflexoZenGame();
            printf("teste1\n");
        } else {
            printf("Chamando desvioPerfeito()\n");
            desvioPerfeito();
            printf("Saindo do bloco else\n");
        }
    }
}

// Função de controle do tempo de inspiração e expiração
void breathActivityLoop() {
    int cycles = 5; // Definir o número de ciclos de respiração
    while (cycles > 0) {
        for (int i = 8; i >= 1; i--) {
            displayNumber(i);
            display_message("Inspire", "Click B para voltar");

            if (buttonBPressed) {
                buttonBPressed = false;
                longjmp(resetPoint, 1); // Volta para antes do loop principal
            }
            
        }
        display_message("Expire",  "Click B para voltar");
        uint32_t start_time = to_ms_since_boot(get_absolute_time());
        while (to_ms_since_boot(get_absolute_time()) - start_time < 8000) {
            detectAudioForDuration(50); // Captura áudio por 50ms
            if (buttonBPressed) {
                buttonBPressed = false;
                longjmp(resetPoint, 1); // Volta para antes do loop principal
            }
        }
        cycles--; // Reduzir o número de ciclos restantes
    }
}

//configuração Questionário

static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *request = (char *)p->payload;

    if (strstr(request, "GET /submit?")) {
        display_message("Respostas", "Recebidas");
        sleep_ms(2000);
        display_message("Obrigado", "e melhoras");
        sleep_ms(2000);
        uint8_t brightness = 255;
        uint8_t colorR = 0, colorG = brightness, colorB = brightness; // Ciano
        uint indices[] = {16, 18, 14, 6, 7, 8, 10};
        for (uint i = 0; i < sizeof(indices) / sizeof(indices[0]); i++) {
            npSetLED(indices[i], colorR, colorG, colorB);
        }
        npWrite();
        sleep_ms(5000);
        npClear(); 
        npWrite();
        display_message("   ", "   ");
        longjmp(resetPoint, 1);
    }

    const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nRecebido";
    tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
    pbuf_free(p);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);
    return ERR_OK;
}

static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        display_message("Erro", "Criar PCB");
        return;
    }

    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        display_message("Erro", "Bind Porta 80");
        return;
    }

    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    display_message("Servidor", "Rodando");
}

void ControleWifiQuestionario() {
    display_message("Pressione A", "Para ligar");

    while (true) {
        if (buttonAPressed) {
            buttonAPressed = false;
            break;
        }
        sleep_ms(100);
    }

    display_message("Conectando", "Wi-Fi...");
    if (cyw43_arch_init()) {
        display_message("Erro", "Wi-Fi Init");
        return;
    }

    cyw43_arch_enable_sta_mode();
    display_message("Wi-Fi", "Habilitado");

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        display_message("Falha", "Conectar Wi-Fi");
        return;
    }

    display_message("Wi-Fi", "Conectado!");
    start_http_server();

    while (true) {
        cyw43_arch_poll();
        sleep_ms(100);
    }
}

void ConfigQuestionario() {
    npClear(); 
    npWrite();
    display_message("agora um", "questionario");
    sleep_ms(5000);

    display_message("escolha dentre", "as opcoes");
    sleep_ms(5000);

    display_message("A Serial", "B Wifi");

    while (true) {
        if (buttonAPressed) {
            buttonAPressed = false;
            ControleSerialQuestionario();
            break;
        } else if (buttonBPressed) {
            buttonBPressed = false;
            ControleWifiQuestionario();
            break;
        }
        sleep_ms(100);
    }
}



int main() {
    stdio_init_all();
    i2c_init(I2C_PORT, ssd1306_i2c_clock * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    gpio_init(13);
    gpio_set_dir(13, GPIO_OUT);
    gpio_init(11);
    gpio_set_dir(11, GPIO_OUT);
    gpio_init(12);
    gpio_set_dir(12, GPIO_OUT);
    gpio_init(10);
    gpio_set_dir(10, GPIO_OUT);
    
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &buttonISR);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &buttonISR);

    gpio_init(JOY_Y);
    gpio_set_dir(JOY_Y, GPIO_IN);
    gpio_pull_up(JOY_Y);

    npInit(LED_PIN);
    npClear();
    
    ssd1306_init();
    calculate_render_area_buffer_length(&frame_area);
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
    if (setjmp(resetPoint) != 0) {
        stopBinauralSound();
        buttonAPressed = false;
        printf("Reiniciando o programa...\n");
    }
    cycleLEDs(); // Faz o ciclo de LEDs no início
    npClear();
    display_message("iniciar o tratamento", "clique no A");

    // Loop principal
    while (1) {
        if (buttonAPressed) { 
            buttonAPressed = false; 
            breathActivityLoop();
            controlBreathCycle(); 
        }

        if (buttonBPressed) { 
            buttonBPressed = false; 
            printf("Botão B pressionado!\n"); 
        }
        sleep_ms(100); // Pequeno atraso para evitar uso excessivo da CPU
    }

}