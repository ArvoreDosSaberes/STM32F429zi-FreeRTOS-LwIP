#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simulador de Temperatura para Testes do Sistema MQTT.

Este script simula o comportamento do STM32F429ZI, publicando valores
de temperatura fictícios para testar o controlador de ventilador sem
necessidade do hardware real.

Uso:
    python mqtt_temperature_simulator.py [opções]

Exemplos:
    # Simular com padrão senoidal (oscila entre min e max)
    python mqtt_temperature_simulator.py --mode sine

    # Simular aquecimento gradual
    python mqtt_temperature_simulator.py --mode heat --start 25 --end 50

    # Simular temperatura fixa
    python mqtt_temperature_simulator.py --mode fixed --temp 42.5

    # Simular valores aleatórios
    python mqtt_temperature_simulator.py --mode random --min 30 --max 45

Autor: Cascade AI
Data: 2025-12-07
Versão: 1.0.0
"""

import argparse
import json
import logging
import math
import os
import random
import signal
import sys
import time
from dataclasses import dataclass
from datetime import datetime
from enum import Enum
from typing import Generator

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("Erro: Biblioteca paho-mqtt não encontrada.")
    print("Instale com: pip install paho-mqtt")
    sys.exit(1)


# =============================================================================
# Configuração de Logging
# =============================================================================

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)


# =============================================================================
# Enumerações e Estruturas
# =============================================================================

class SimulationMode(Enum):
    """Modos de simulação de temperatura."""
    FIXED = "fixed"       # Temperatura fixa
    SINE = "sine"         # Padrão senoidal
    HEAT = "heat"         # Aquecimento gradual
    COOL = "cool"         # Resfriamento gradual
    RANDOM = "random"     # Valores aleatórios
    CYCLE = "cycle"       # Ciclo aquecimento/resfriamento


@dataclass
class MqttConfig:
    """Configuração de conexão MQTT."""
    broker: str = "mqtt.rapport.tec.br"
    port: int = 1883
    username: str = ""
    password: str = ""
    base_topic: str = "racks"
    rack_number: str = "STM32F429"
    client_id: str = "temperature_simulator"


@dataclass
class SimulationConfig:
    """Configuração de simulação."""
    mode: SimulationMode = SimulationMode.SINE
    interval_s: float = 5.0   # Intervalo entre publicações
    temp_min: float = 25.0    # Temperatura mínima
    temp_max: float = 50.0    # Temperatura máxima
    temp_fixed: float = 35.0  # Temperatura fixa (modo fixed)
    period_s: float = 60.0    # Período do ciclo (modos sine/cycle)


# =============================================================================
# Geradores de Temperatura
# =============================================================================

def generate_fixed(config: SimulationConfig) -> Generator[float, None, None]:
    """Gera temperatura fixa."""
    while True:
        yield config.temp_fixed


def generate_sine(config: SimulationConfig) -> Generator[float, None, None]:
    """Gera temperatura com padrão senoidal."""
    start_time = time.time()
    amplitude = (config.temp_max - config.temp_min) / 2
    offset = config.temp_min + amplitude
    
    while True:
        elapsed = time.time() - start_time
        phase = (2 * math.pi * elapsed) / config.period_s
        temp = offset + amplitude * math.sin(phase)
        yield temp


def generate_heat(config: SimulationConfig) -> Generator[float, None, None]:
    """Gera temperatura com aquecimento gradual."""
    temp = config.temp_min
    step = (config.temp_max - config.temp_min) / (config.period_s / config.interval_s)
    
    while True:
        yield temp
        temp = min(temp + step, config.temp_max)


def generate_cool(config: SimulationConfig) -> Generator[float, None, None]:
    """Gera temperatura com resfriamento gradual."""
    temp = config.temp_max
    step = (config.temp_max - config.temp_min) / (config.period_s / config.interval_s)
    
    while True:
        yield temp
        temp = max(temp - step, config.temp_min)


def generate_random(config: SimulationConfig) -> Generator[float, None, None]:
    """Gera temperatura aleatória."""
    while True:
        yield random.uniform(config.temp_min, config.temp_max)


def generate_cycle(config: SimulationConfig) -> Generator[float, None, None]:
    """Gera ciclo de aquecimento e resfriamento."""
    temp = config.temp_min
    step = (config.temp_max - config.temp_min) / (config.period_s / config.interval_s / 2)
    heating = True
    
    while True:
        yield temp
        
        if heating:
            temp += step
            if temp >= config.temp_max:
                temp = config.temp_max
                heating = False
        else:
            temp -= step
            if temp <= config.temp_min:
                temp = config.temp_min
                heating = True


def get_temperature_generator(config: SimulationConfig) -> Generator[float, None, None]:
    """
    Retorna o gerador de temperatura apropriado para o modo.

    Args:
        config: Configuração de simulação.

    Returns:
        Gerador de valores de temperatura.
    """
    generators = {
        SimulationMode.FIXED: generate_fixed,
        SimulationMode.SINE: generate_sine,
        SimulationMode.HEAT: generate_heat,
        SimulationMode.COOL: generate_cool,
        SimulationMode.RANDOM: generate_random,
        SimulationMode.CYCLE: generate_cycle,
    }
    
    return generators[config.mode](config)


# =============================================================================
# Simulador Principal
# =============================================================================

class TemperatureSimulator:
    """
    Simulador de temperatura para testes MQTT.
    
    Publica valores de temperatura simulados no mesmo formato
    que o STM32F429ZI faria.
    """

    def __init__(self, mqtt_config: MqttConfig, sim_config: SimulationConfig):
        """
        Inicializa o simulador.

        Args:
            mqtt_config: Configuração de conexão MQTT.
            sim_config: Configuração de simulação.
        """
        self.mqtt_config = mqtt_config
        self.sim_config = sim_config
        self.running = True
        self.connected = False
        self.messages_sent = 0
        
        # Tópico de temperatura
        self.topic_temperature = f"{mqtt_config.base_topic}/{mqtt_config.rack_number}/temperature"
        self.topic_fan_status = f"{mqtt_config.base_topic}/{mqtt_config.rack_number}/fan/status"
        
        # Cliente MQTT
        self.client = mqtt.Client(
            client_id=mqtt_config.client_id,
            protocol=mqtt.MQTTv311,
            clean_session=True
        )
        
        # Callbacks
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message
        
        # Autenticação
        if mqtt_config.username:
            self.client.username_pw_set(
                mqtt_config.username,
                mqtt_config.password
            )
        
        # Gerador de temperatura
        self.temp_generator = get_temperature_generator(sim_config)
        
        # Tratamento de sinais
        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)

    def _signal_handler(self, signum, frame):
        """Trata sinais de interrupção."""
        logger.info("Sinal de interrupção recebido. Encerrando...")
        self.running = False

    def _on_connect(self, client, userdata, flags, rc):
        """Callback de conexão."""
        if rc == 0:
            self.connected = True
            logger.info(f"Conectado ao broker {self.mqtt_config.broker}")
            
            # Subscrever ao status do ventilador para ver respostas
            client.subscribe(self.topic_fan_status, 1)
            logger.info(f"Subscrito a: {self.topic_fan_status}")
        else:
            logger.error(f"Falha na conexão: rc={rc}")

    def _on_disconnect(self, client, userdata, rc):
        """Callback de desconexão."""
        self.connected = False
        logger.info("Desconectado do broker")

    def _on_message(self, client, userdata, msg):
        """Callback de mensagem recebida."""
        if msg.topic == self.topic_fan_status:
            try:
                payload = msg.payload.decode('utf-8')
                logger.info(f"Status do ventilador recebido: {payload}")
            except Exception as e:
                logger.error(f"Erro ao processar mensagem: {e}")

    def connect(self) -> bool:
        """Conecta ao broker MQTT."""
        try:
            logger.info(f"Conectando a {self.mqtt_config.broker}:{self.mqtt_config.port}...")
            self.client.connect(
                self.mqtt_config.broker,
                self.mqtt_config.port,
                60
            )
            return True
        except Exception as e:
            logger.error(f"Erro ao conectar: {e}")
            return False

    def publish_temperature(self, temperature: float):
        """
        Publica temperatura no formato JSON.

        Args:
            temperature: Valor da temperatura em Celsius.
        """
        if not self.connected:
            return
        
        payload = json.dumps({
            "temperature": round(temperature, 1),
            "unit": "C",
            "simulated": True,
            "timestamp": datetime.now().isoformat()
        })
        
        result = self.client.publish(
            self.topic_temperature,
            payload,
            qos=1,
            retain=False
        )
        
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            self.messages_sent += 1
            logger.info(f"Temperatura publicada: {temperature:.1f}°C -> {self.topic_temperature}")
        else:
            logger.error(f"Falha ao publicar temperatura: rc={result.rc}")

    def run(self):
        """Loop principal do simulador."""
        if not self.connect():
            return
        
        self.client.loop_start()
        
        logger.info("=" * 60)
        logger.info("Simulador de Temperatura MQTT Iniciado")
        logger.info("=" * 60)
        logger.info(f"Modo: {self.sim_config.mode.value}")
        logger.info(f"Intervalo: {self.sim_config.interval_s}s")
        logger.info(f"Faixa: {self.sim_config.temp_min}°C - {self.sim_config.temp_max}°C")
        logger.info(f"Tópico: {self.topic_temperature}")
        logger.info("=" * 60)
        logger.info("Pressione Ctrl+C para encerrar")
        logger.info("")
        
        try:
            while self.running:
                if self.connected:
                    temperature = next(self.temp_generator)
                    self.publish_temperature(temperature)
                
                time.sleep(self.sim_config.interval_s)
                
        except KeyboardInterrupt:
            pass
        finally:
            self.client.loop_stop()
            self.client.disconnect()
            
            logger.info("")
            logger.info("=" * 60)
            logger.info(f"Simulação encerrada. Mensagens enviadas: {self.messages_sent}")
            logger.info("=" * 60)


# =============================================================================
# Funções Auxiliares
# =============================================================================

def load_config_from_env() -> MqttConfig:
    """Carrega configuração de variáveis de ambiente."""
    return MqttConfig(
        broker=os.getenv('MQTT_BROKER', 'mqtt.rapport.tec.br'),
        port=int(os.getenv('MQTT_PORT', '1883')),
        username=os.getenv('MQTT_USERNAME', 'rack'),
        password=os.getenv('MQTT_PASSWORD', 'rack4567senha'),
        base_topic=os.getenv('MQTT_BASE_TOPIC', 'racks'),
        rack_number=os.getenv('MQTT_RACK_NUMBER', 'STM32F429'),
        client_id=f'temp_sim_{os.getpid()}',
    )


def parse_arguments() -> argparse.Namespace:
    """Processa argumentos de linha de comando."""
    parser = argparse.ArgumentParser(
        description='Simulador de Temperatura para Testes MQTT',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    # Modo de simulação
    parser.add_argument(
        '--mode', '-m',
        choices=['fixed', 'sine', 'heat', 'cool', 'random', 'cycle'],
        default='sine',
        help='Modo de simulação (padrão: sine)'
    )
    
    # Configuração de temperatura
    parser.add_argument(
        '--temp', '-t',
        type=float,
        default=35.0,
        help='Temperatura fixa (modo fixed)'
    )
    parser.add_argument(
        '--min',
        type=float,
        default=25.0,
        help='Temperatura mínima (padrão: 25.0)'
    )
    parser.add_argument(
        '--max',
        type=float,
        default=50.0,
        help='Temperatura máxima (padrão: 50.0)'
    )
    
    # Configuração de tempo
    parser.add_argument(
        '--interval', '-i',
        type=float,
        default=5.0,
        help='Intervalo entre publicações em segundos (padrão: 5.0)'
    )
    parser.add_argument(
        '--period',
        type=float,
        default=60.0,
        help='Período do ciclo em segundos (padrão: 60.0)'
    )
    
    # Configuração MQTT
    parser.add_argument('--broker', '-b', help='Endereço do broker')
    parser.add_argument('--port', '-p', type=int, help='Porta do broker')
    parser.add_argument('--username', '-u', help='Usuário MQTT')
    parser.add_argument('--password', '-P', help='Senha MQTT')
    parser.add_argument('--rack', help='Identificador do rack')
    
    # Opções gerais
    parser.add_argument('--verbose', '-v', action='store_true', help='Modo verboso')
    
    return parser.parse_args()


def main():
    """Função principal."""
    args = parse_arguments()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # Carregar configuração MQTT
    mqtt_config = load_config_from_env()
    
    if args.broker:
        mqtt_config.broker = args.broker
    if args.port:
        mqtt_config.port = args.port
    if args.username:
        mqtt_config.username = args.username
    if args.password:
        mqtt_config.password = args.password
    if args.rack:
        mqtt_config.rack_number = args.rack
    
    # Configuração de simulação
    sim_config = SimulationConfig(
        mode=SimulationMode(args.mode),
        interval_s=args.interval,
        temp_min=args.min,
        temp_max=args.max,
        temp_fixed=args.temp,
        period_s=args.period,
    )
    
    # Criar e executar simulador
    simulator = TemperatureSimulator(mqtt_config, sim_config)
    simulator.run()


if __name__ == '__main__':
    main()
