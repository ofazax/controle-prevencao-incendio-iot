import paho.mqtt.client as mqtt
import mysql.connector
import time
import os
from dotenv import load_dotenv

load_dotenv() # Carrega as variáveis do arquivo .env

# Definições dos tópicos
TOPICO_PUBLISH_TEMPERATURA = "topico_sensor_temperatura"
TOPICO_PUBLISH_TEMPERATURA_VALOR = "topico_sensor_temperatura_valor"
TOPICO_PUBLISH_UMIDADE = "topico_sensor_umidade"
TOPICO_PUBLISH_UMIDADE_VALOR = "topico_sensor_umidade_valor"
TOPICO_PUBLISH_CHUVA = "topico_sensor_chuva"
TOPICO_PUBLISH_FOGO = "topico_sensor_fogo"

# Configurações do Broker MQTT
BROKER_MQTT = "broker.emqx.io"
PORTA_MQTT = 1883

# Configurações do banco de dados MySQL 
DB_HOST = os.getenv("DB_HOST", "localhost")
DB_USER = os.getenv("DB_USER")
DB_PASSWORD = os.getenv("DB_PASSWORD")
DB_NAME = "sensores"

# Variáveis para armazenar os dados recebidos
temperatura = None
umidade = None
chuva = None
fogo = None

# Função callback para quando a mensagem for recebida
def on_message(client, userdata, message):
    global temperatura, umidade, chuva, fogo

    # Decodifica a mensagem recebida
    payload = message.payload.decode()
    topic = message.topic

    # Log para verificar se a mensagem está sendo recebida
    print(f"Mensagem recebida no tópico '{topic}': {payload}")

    # Armazena os dados dos tópicos
    if topic == TOPICO_PUBLISH_TEMPERATURA_VALOR:
        temperatura = float(payload)
    elif topic == TOPICO_PUBLISH_UMIDADE_VALOR:
        umidade = int(payload)
    elif topic == TOPICO_PUBLISH_CHUVA:
        chuva = payload
    elif topic == TOPICO_PUBLISH_FOGO:
        fogo = payload

    # Chama a função para inserir os dados no banco de dados sempre que uma nova mensagem é recebida
    inserir_dados_no_mysql()

# Função para configurar a conexão MQTT
def on_connect(client, userdata, flags, rc):
    print(f"Conectado ao broker MQTT com código {rc}")
    
    # Inscreve-se nos tópicos de interesse
    client.subscribe(TOPICO_PUBLISH_TEMPERATURA_VALOR)
    client.subscribe(TOPICO_PUBLISH_UMIDADE_VALOR)
    client.subscribe(TOPICO_PUBLISH_CHUVA)
    client.subscribe(TOPICO_PUBLISH_FOGO)

# Função para inserir os dados no banco de dados MySQL
def inserir_dados_no_mysql():
    if temperatura is not None and umidade is not None and chuva is not None and fogo is not None:
        try:
            # Conectando ao banco de dados MySQL
            db = mysql.connector.connect(
                host=DB_HOST,
                user=DB_USER,
                password=DB_PASSWORD,
                database=DB_NAME
            )
            cursor = db.cursor()

            # Inserindo os dados na tabela 'dados_sensores'
            query = """
                INSERT INTO dados_sensores (temperatura_valor, umidade_valor, chuva, fogo)
                VALUES (%s, %s, %s, %s)
            """
            values = (temperatura, umidade, chuva, fogo)
            cursor.execute(query, values)

            # Confirmando a inserção
            db.commit()
         
            # Fechando a conexão
            cursor.close()
            db.close()
        except mysql.connector.Error as err:
            print(f"Erro ao inserir dados no MySQL: {err}")

# Criação do cliente MQTT
client = mqtt.Client()  # Sem o ID especificado
client.on_connect = on_connect
client.on_message = on_message

# Conectar-se ao broker MQTT
client.connect(BROKER_MQTT, PORTA_MQTT, 60)

# Loop principal para processar mensagens de MQTT em tempo real
try:
    while True:
        client.loop()  # Processa as mensagens recebidas continuamente

        # Log para verificar o estado das variáveis
        print(f"Temperatura: {temperatura}, Umidade: {umidade}, Chuva: {chuva}, Fogo: {fogo}")

        # Aguarda um momento, mas sem bloquear o loop
        time.sleep(0.1)  # Pequeno atraso para permitir o processamento do loop

except KeyboardInterrupt:
    print("Programa interrompido pelo usuário.")

finally:
    client.disconnect()  # Desconecta do broker MQTT ao finalizar
