Monitoramento de Parâmetros com BitDogLab - Raspberry Pi Pico W

Este projeto implementa um sistema de monitoramento de sensores de horta hidropônica utilizando a placa Raspberry Pi Pico. O código simula a leitura de parâmetros essenciais, como pH, condutividade elétrica, temperatura e nível de água, e exibe os resultados através de uma matriz de LEDs RGB e de uma página web, utilizando protocolo HTTP.

📌 Funcionalidades Simulação de sensores: Alterna entre leituras "Tudo Ok" e "Fora do Padrão". Matriz de LEDs: Indica o status das medições (Verde = OK, Vermelho = Problema). Buzzer: Ativado quando os valores estão fora da faixa ideal. Botões de controle: Botão A: Inicia o monitoramento. Botão B: Pausa o monitoramento. Monitor Serial: Exibe os valores lidos e o status correspondente.

🚀 Como Usar Conecte a Raspberry Pi Pico ao computador via USB. Compile e carregue o código utilizando o ambiente de desenvolvimento (VS Code + Pico SDK ou Arduino IDE). Abra o Monitor Serial (115200 baud), e utilize o ip para acessar a pagina web local, para visualizar as leituras(Ou acompanhe pelo monitor serial mesmo). Pressione o botão A para iniciar o monitoramento. Segure o botão B durante um ciclo(5 segundos) para pausar a execução.

🛠️ Tecnologias Utilizadas C (Pico SDK) GPIO para controle de LEDs, botões e buzzer PWM para acionamento do buzzer PIO para comunicação com matriz de LEDs

📝 Estrutura do Código Simulação de sensores na função simular_leitura_alternada() Controle de LEDs na função atualizar_matriz() Acionamento do buzzer na função tocar_buzzer() Loop principal gerencia os estados e alterna as leituras

📄 Licença Este projeto é de código aberto e pode ser utilizado livremente.
