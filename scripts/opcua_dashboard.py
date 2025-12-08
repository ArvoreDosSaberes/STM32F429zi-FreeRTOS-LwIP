#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Dashboard OPC-UA para monitoramento de temperatura e controle de ventilador.

Este script conecta-se a um servidor OPC-UA (STM32) e exibe:
- Gauge de temperatura atual do MCU
- Campos para configuração de temperatura máxima e mínima
- Controle automático do ventilador baseado nos limites

O ventilador é automaticamente:
- Ligado quando a temperatura ultrapassa o limite máximo
- Desligado quando a temperatura cai abaixo do limite mínimo

Uso:
    python opcua_dashboard.py --server opc.tcp://192.168.0.228:4840

Dependências:
    - asyncua (cliente OPC-UA assíncrono)
    - flask (servidor web)
    - plotly (gráficos interativos)

Autor: Carlos Delfino <consultoria@carlosdelfino.eti.br>
"""

import os
import sys
import signal
import asyncio
import argparse
import logging
import json
from datetime import datetime
from threading import Thread, Event
from dataclasses import dataclass, field
from typing import Optional, Callable

from flask import Flask, render_template_string, jsonify, request
from asyncua import Client, ua

# Configuração de logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('OPCUA-Dashboard')

# =============================================================================
# Constantes e Configuração
# =============================================================================

# NodeIds do servidor OPC-UA (devem corresponder aos definidos no firmware)
OPCUA_NS_INDEX = 1
NODEID_TEMPERATURE = f"ns={OPCUA_NS_INDEX};i=1001"
NODEID_FAN_STATE = f"ns={OPCUA_NS_INDEX};i=1002"
NODEID_FAN_COMMAND = f"ns={OPCUA_NS_INDEX};i=1003"
NODEID_TEMP_MAX = f"ns={OPCUA_NS_INDEX};i=1004"
NODEID_TEMP_MIN = f"ns={OPCUA_NS_INDEX};i=1005"

# Valores padrão de temperatura
DEFAULT_TEMP_MAX = 45.0
DEFAULT_TEMP_MIN = 20.0
DEFAULT_SERVER_URL = "opc.tcp://192.168.0.228:4840"

# Intervalo de atualização em segundos
UPDATE_INTERVAL = 1.0

# =============================================================================
# Estruturas de Dados
# =============================================================================

@dataclass
class DashboardState:
    """Estado atual do dashboard."""
    temperature: float = 0.0
    fan_state: bool = False
    temp_max: float = DEFAULT_TEMP_MAX
    temp_min: float = DEFAULT_TEMP_MIN
    connected: bool = False
    last_update: Optional[datetime] = None
    auto_control: bool = True
    error_message: str = ""
    
    def to_dict(self) -> dict:
        """Converte para dicionário JSON-serializável."""
        return {
            'temperature': round(self.temperature, 1),
            'fan_state': self.fan_state,
            'temp_max': self.temp_max,
            'temp_min': self.temp_min,
            'connected': self.connected,
            'last_update': self.last_update.isoformat() if self.last_update else None,
            'auto_control': self.auto_control,
            'error_message': self.error_message
        }

# =============================================================================
# Cliente OPC-UA
# =============================================================================

class OpcUaClient:
    """
    Cliente OPC-UA para comunicação com o servidor STM32.
    
    Gerencia a conexão, leitura de valores e envio de comandos.
    Implementa reconexão automática em caso de falha.
    """
    
    def __init__(self, server_url: str, state: DashboardState, standard_mode: bool = False):
        """
        Inicializa o cliente OPC-UA.
        
        Args:
            server_url: URL do servidor OPC-UA (ex: opc.tcp://192.168.0.228:4840)
            state: Objeto de estado compartilhado com o dashboard
            standard_mode: Se True, usa client.connect() padrão da biblioteca
        """
        self.server_url = server_url
        self.state = state
        self.standard_mode = standard_mode
        self.client: Optional[Client] = None
        self.running = False
        self._stop_event = Event()
        self._loop: Optional[asyncio.AbstractEventLoop] = None
        self._thread: Optional[Thread] = None
        
        # Callbacks para eventos
        self.on_temperature_change: Optional[Callable[[float], None]] = None
        self.on_fan_state_change: Optional[Callable[[bool], None]] = None
    
    def start(self):
        """Inicia o cliente em uma thread separada."""
        if self.running:
            return
        
        self._stop_event.clear()
        self._thread = Thread(target=self._run_async_loop, daemon=True)
        self._thread.start()
        self.running = True
        logger.info(f"Cliente OPC-UA iniciado para {self.server_url}")
    
    def stop(self):
        """Para o cliente e fecha a conexão."""
        self._stop_event.set()
        self.running = False
        
        if self._thread and self._thread.is_alive():
            self._thread.join(timeout=5.0)
        
        logger.info("Cliente OPC-UA parado")
    
    def _run_async_loop(self):
        """Executa o loop assíncrono em uma thread separada."""
        self._loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self._loop)
        
        try:
            self._loop.run_until_complete(self._main_loop())
        except Exception as e:
            logger.error(f"Erro no loop assíncrono: {e}")
        finally:
            self._loop.close()
    
    async def _main_loop(self):
        """Loop principal de comunicação com o servidor."""
        while not self._stop_event.is_set():
            try:
                await self._connect_and_run()
            except Exception as e:
                logger.error(f"Erro na conexão: {e}")
                self.state.connected = False
                self.state.error_message = str(e)
                
                # Aguardar antes de reconectar
                await asyncio.sleep(5.0)
    
    async def _connect_and_run(self):
        """Conecta ao servidor e executa o loop de leitura."""
        self.client = Client(url=self.server_url)
        
        # Configurações de conexão
        self.client.session_timeout = 30000  # 30 segundos
        
        logger.info(f"Conectando a {self.server_url}...")
        
        # Modo standard: usa async with client (modo típico da biblioteca)
        if self.standard_mode:
            logger.info("Usando modo de conexão padrão (standard)")
            async with self.client:
                await self._run_read_loop()
            return
        
        # Modo com fallback para servidores embarcados
        connected = False
        
        # Tentar conexão normal primeiro
        try:
            await self.client.connect()
            connected = True
        except Exception as e:
            logger.warning(f"Conexão padrão falhou: {e}")
            logger.info("Tentando conexão direta sem verificação de endpoints...")
            
            # Extrair host e porta da URL
            import re
            match = re.match(r'opc\.tcp://([^:]+):(\d+)', self.server_url)
            if match:
                host, port = match.group(1), int(match.group(2))
                try:
                    await self.client.uaclient.connect_socket(host, port)
                    await self.client.uaclient.send_hello(self.server_url)
                    params = ua.OpenSecureChannelParameters()
                    params.RequestType = ua.SecurityTokenRequestType.Issue
                    params.SecurityMode = ua.MessageSecurityMode.None_
                    params.RequestedLifetime = 3600000
                    params.ClientNonce = b'\x00' * 32
                    await self.client.uaclient.open_secure_channel(params)
                    connected = True
                except Exception as e2:
                    logger.error(f"Conexão direta também falhou: {e2}")
                    raise
            else:
                raise ValueError(f"URL inválida: {self.server_url}")
        
        if not connected:
            return
        
        logger.info("Conectado ao servidor OPC-UA")
        self.state.connected = True
        self.state.error_message = ""
        
        try:
            # Obter referências aos nós
            temp_node = self.client.get_node(NODEID_TEMPERATURE)
            fan_state_node = self.client.get_node(NODEID_FAN_STATE)
            fan_cmd_node = self.client.get_node(NODEID_FAN_COMMAND)
            temp_max_node = self.client.get_node(NODEID_TEMP_MAX)
            temp_min_node = self.client.get_node(NODEID_TEMP_MIN)
            
            # Ler valores iniciais dos limites
            try:
                self.state.temp_max = await temp_max_node.read_value()
                self.state.temp_min = await temp_min_node.read_value()
            except Exception as e:
                logger.warning(f"Erro ao ler limites iniciais: {e}")
            
            # Loop de leitura
            while not self._stop_event.is_set() and self.state.connected:
                try:
                    # Ler temperatura
                    temp = await temp_node.read_value()
                    old_temp = self.state.temperature
                    self.state.temperature = float(temp)
                    
                    if self.on_temperature_change and abs(temp - old_temp) > 0.1:
                        self.on_temperature_change(temp)
                    
                    # Ler estado do ventilador
                    fan = await fan_state_node.read_value()
                    old_fan = self.state.fan_state
                    self.state.fan_state = bool(fan)
                    
                    if self.on_fan_state_change and fan != old_fan:
                        self.on_fan_state_change(fan)
                    
                    self.state.last_update = datetime.now()
                    
                    # Controle automático do ventilador
                    if self.state.auto_control:
                        await self._auto_control_fan(fan_cmd_node)
                    
                except ua.UaError as e:
                    logger.error(f"Erro UA: {e}")
                    self.state.error_message = str(e)
                    break
                
                await asyncio.sleep(UPDATE_INTERVAL)
        finally:
            self.state.connected = False
            try:
                await self.client.disconnect()
            except Exception:
                pass
    
    async def _run_read_loop(self):
        """Executa o loop de leitura de valores do servidor OPC-UA."""
        logger.info("Conectado ao servidor OPC-UA")
        self.state.connected = True
        self.state.error_message = ""
        
        # Obter referências aos nós
        temp_node = self.client.get_node(NODEID_TEMPERATURE)
        fan_state_node = self.client.get_node(NODEID_FAN_STATE)
        fan_cmd_node = self.client.get_node(NODEID_FAN_COMMAND)
        temp_max_node = self.client.get_node(NODEID_TEMP_MAX)
        temp_min_node = self.client.get_node(NODEID_TEMP_MIN)
        
        # Ler valores iniciais dos limites
        try:
            self.state.temp_max = await temp_max_node.read_value()
            self.state.temp_min = await temp_min_node.read_value()
        except Exception as e:
            logger.warning(f"Erro ao ler limites iniciais: {e}")
        
        # Loop de leitura
        while not self._stop_event.is_set() and self.state.connected:
            try:
                # Ler temperatura
                temp = await temp_node.read_value()
                old_temp = self.state.temperature
                self.state.temperature = float(temp)
                
                if self.on_temperature_change and abs(temp - old_temp) > 0.1:
                    self.on_temperature_change(temp)
                
                # Ler estado do ventilador
                fan = await fan_state_node.read_value()
                old_fan = self.state.fan_state
                self.state.fan_state = bool(fan)
                
                if self.on_fan_state_change and fan != old_fan:
                    self.on_fan_state_change(fan)
                
                self.state.last_update = datetime.now()
                
                # Controle automático do ventilador
                if self.state.auto_control:
                    await self._auto_control_fan(fan_cmd_node)
                
            except ua.UaError as e:
                logger.error(f"Erro UA: {e}")
                self.state.error_message = str(e)
                break
            
            await asyncio.sleep(UPDATE_INTERVAL)
        
        self.state.connected = False
    
    async def _auto_control_fan(self, fan_cmd_node):
        """
        Controle automático do ventilador baseado nos limites de temperatura.
        
        - Liga o ventilador se temperatura > temp_max
        - Desliga o ventilador se temperatura < temp_min
        """
        try:
            if self.state.temperature > self.state.temp_max and not self.state.fan_state:
                logger.info(f"Temperatura {self.state.temperature}°C > {self.state.temp_max}°C - Ligando ventilador")
                await fan_cmd_node.write_value(ua.DataValue(ua.Variant(True, ua.VariantType.Boolean)))
                self.state.fan_state = True
                
            elif self.state.temperature < self.state.temp_min and self.state.fan_state:
                logger.info(f"Temperatura {self.state.temperature}°C < {self.state.temp_min}°C - Desligando ventilador")
                await fan_cmd_node.write_value(ua.DataValue(ua.Variant(False, ua.VariantType.Boolean)))
                self.state.fan_state = False
                
        except Exception as e:
            logger.error(f"Erro no controle automático: {e}")
    
    async def set_fan_state(self, state: bool):
        """
        Define o estado do ventilador.
        
        Args:
            state: True para ligar, False para desligar
        """
        if not self.client or not self.state.connected:
            raise RuntimeError("Não conectado ao servidor")
        
        node = self.client.get_node(NODEID_FAN_COMMAND)
        await node.write_value(ua.DataValue(ua.Variant(state, ua.VariantType.Boolean)))
        logger.info(f"Ventilador {'ligado' if state else 'desligado'} manualmente")
    
    async def set_temp_limits(self, temp_max: float, temp_min: float):
        """
        Define os limites de temperatura.
        
        Args:
            temp_max: Limite máximo de temperatura
            temp_min: Limite mínimo de temperatura
        """
        if not self.client or not self.state.connected:
            raise RuntimeError("Não conectado ao servidor")
        
        if temp_min >= temp_max:
            raise ValueError("Temperatura mínima deve ser menor que a máxima")
        
        max_node = self.client.get_node(NODEID_TEMP_MAX)
        min_node = self.client.get_node(NODEID_TEMP_MIN)
        
        await max_node.write_value(ua.DataValue(ua.Variant(float(temp_max), ua.VariantType.Float)))
        await min_node.write_value(ua.DataValue(ua.Variant(float(temp_min), ua.VariantType.Float)))
        
        self.state.temp_max = temp_max
        self.state.temp_min = temp_min
        
        logger.info(f"Limites atualizados: {temp_min}°C - {temp_max}°C")
    
    def run_coroutine(self, coro):
        """Executa uma corrotina no loop do cliente."""
        if self._loop and self._loop.is_running():
            future = asyncio.run_coroutine_threadsafe(coro, self._loop)
            return future.result(timeout=5.0)
        raise RuntimeError("Loop não está rodando")

# =============================================================================
# Servidor Web Flask
# =============================================================================

# Template HTML do Dashboard
DASHBOARD_HTML = """
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dashboard OPC-UA - STM32 Temperature Monitor</title>
    <script src="https://cdn.plot.ly/plotly-2.27.0.min.js"></script>
    <script src="https://cdn.tailwindcss.com"></script>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css">
    <style>
        .gauge-container { min-height: 300px; }
        .status-dot { 
            width: 12px; 
            height: 12px; 
            border-radius: 50%; 
            display: inline-block;
            animation: pulse 2s infinite;
        }
        .status-connected { background-color: #22c55e; }
        .status-disconnected { background-color: #ef4444; }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        .fan-on { animation: spin 1s linear infinite; }
        @keyframes spin {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
        }
    </style>
</head>
<body class="bg-gray-900 text-white min-h-screen">
    <div class="container mx-auto px-4 py-8">
        <!-- Header -->
        <header class="mb-8">
            <div class="flex items-center justify-between">
                <div>
                    <h1 class="text-3xl font-bold text-cyan-400">
                        <i class="fas fa-microchip mr-2"></i>STM32 OPC-UA Dashboard
                    </h1>
                    <p class="text-gray-400 mt-1">Monitoramento de Temperatura e Controle de Ventilador</p>
                </div>
                <div class="flex items-center gap-4">
                    <span class="status-dot" id="status-dot"></span>
                    <span id="connection-status" class="text-sm">Desconectado</span>
                </div>
            </div>
        </header>

        <div class="grid grid-cols-1 lg:grid-cols-2 gap-8">
            <!-- Gauge de Temperatura -->
            <div class="bg-gray-800 rounded-xl p-6 shadow-lg">
                <h2 class="text-xl font-semibold mb-4 text-cyan-300">
                    <i class="fas fa-temperature-half mr-2"></i>Temperatura do MCU
                </h2>
                <div id="temperature-gauge" class="gauge-container"></div>
                <div class="text-center mt-4">
                    <span class="text-4xl font-bold" id="temp-value">--</span>
                    <span class="text-2xl text-gray-400">°C</span>
                </div>
            </div>

            <!-- Controles -->
            <div class="space-y-6">
                <!-- Estado do Ventilador -->
                <div class="bg-gray-800 rounded-xl p-6 shadow-lg">
                    <h2 class="text-xl font-semibold mb-4 text-cyan-300">
                        <i class="fas fa-fan mr-2" id="fan-icon"></i>Ventilador
                    </h2>
                    <div class="flex items-center justify-between">
                        <div>
                            <span class="text-2xl font-bold" id="fan-status">--</span>
                            <p class="text-gray-400 text-sm mt-1" id="fan-reason"></p>
                        </div>
                        <div class="flex gap-2">
                            <button onclick="setFan(true)" 
                                    class="px-4 py-2 bg-green-600 hover:bg-green-700 rounded-lg transition">
                                <i class="fas fa-power-off mr-1"></i>Ligar
                            </button>
                            <button onclick="setFan(false)" 
                                    class="px-4 py-2 bg-red-600 hover:bg-red-700 rounded-lg transition">
                                <i class="fas fa-power-off mr-1"></i>Desligar
                            </button>
                        </div>
                    </div>
                </div>

                <!-- Limites de Temperatura -->
                <div class="bg-gray-800 rounded-xl p-6 shadow-lg">
                    <h2 class="text-xl font-semibold mb-4 text-cyan-300">
                        <i class="fas fa-sliders mr-2"></i>Limites de Temperatura
                    </h2>
                    <div class="grid grid-cols-2 gap-4 mb-4">
                        <div>
                            <label class="block text-sm text-gray-400 mb-1">Temperatura Máxima (°C)</label>
                            <input type="number" id="temp-max" step="0.5" 
                                   class="w-full px-3 py-2 bg-gray-700 rounded-lg border border-gray-600 focus:border-cyan-500 focus:outline-none">
                        </div>
                        <div>
                            <label class="block text-sm text-gray-400 mb-1">Temperatura Mínima (°C)</label>
                            <input type="number" id="temp-min" step="0.5" 
                                   class="w-full px-3 py-2 bg-gray-700 rounded-lg border border-gray-600 focus:border-cyan-500 focus:outline-none">
                        </div>
                    </div>
                    <div class="flex items-center justify-between">
                        <label class="flex items-center cursor-pointer">
                            <input type="checkbox" id="auto-control" checked 
                                   class="w-4 h-4 mr-2 accent-cyan-500">
                            <span>Controle Automático</span>
                        </label>
                        <button onclick="setLimits()" 
                                class="px-4 py-2 bg-cyan-600 hover:bg-cyan-700 rounded-lg transition">
                            <i class="fas fa-save mr-1"></i>Aplicar
                        </button>
                    </div>
                    <p class="text-gray-400 text-sm mt-4">
                        <i class="fas fa-info-circle mr-1"></i>
                        Ventilador liga automaticamente acima do máximo e desliga abaixo do mínimo.
                    </p>
                </div>

                <!-- Informações -->
                <div class="bg-gray-800 rounded-xl p-6 shadow-lg">
                    <h2 class="text-xl font-semibold mb-4 text-cyan-300">
                        <i class="fas fa-info-circle mr-2"></i>Informações
                    </h2>
                    <div class="space-y-2 text-sm">
                        <div class="flex justify-between">
                            <span class="text-gray-400">Servidor OPC-UA:</span>
                            <span id="server-url" class="font-mono">--</span>
                        </div>
                        <div class="flex justify-between">
                            <span class="text-gray-400">Última Atualização:</span>
                            <span id="last-update">--</span>
                        </div>
                        <div class="flex justify-between">
                            <span class="text-gray-400">Status:</span>
                            <span id="error-message" class="text-red-400">--</span>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <!-- Footer -->
        <footer class="mt-8 text-center text-gray-500 text-sm">
            <p>Dashboard OPC-UA para STM32F429ZI</p>
            <p>Desenvolvido por Carlos Delfino - <a href="https://mcu.tec.br" class="text-cyan-400 hover:underline">mcu.tec.br</a></p>
        </footer>
    </div>

    <script>
        // Configuração do Gauge
        const gaugeData = [{
            type: "indicator",
            mode: "gauge+number",
            value: 25,
            gauge: {
                axis: { range: [0, 80], tickwidth: 1, tickcolor: "#64748b" },
                bar: { color: "#06b6d4" },
                bgcolor: "#1e293b",
                borderwidth: 2,
                bordercolor: "#334155",
                steps: [
                    { range: [0, 20], color: "#0ea5e9" },
                    { range: [20, 35], color: "#22c55e" },
                    { range: [35, 50], color: "#eab308" },
                    { range: [50, 65], color: "#f97316" },
                    { range: [65, 80], color: "#ef4444" }
                ],
                threshold: {
                    line: { color: "#ef4444", width: 4 },
                    thickness: 0.75,
                    value: 45
                }
            }
        }];

        const gaugeLayout = {
            paper_bgcolor: "transparent",
            font: { color: "#e2e8f0", size: 14 },
            margin: { t: 25, r: 25, l: 25, b: 25 }
        };

        Plotly.newPlot('temperature-gauge', gaugeData, gaugeLayout, { responsive: true });

        // Atualização periódica
        function updateDashboard() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    // Temperatura
                    const temp = data.temperature;
                    document.getElementById('temp-value').textContent = temp.toFixed(1);
                    Plotly.update('temperature-gauge', { value: [temp] });

                    // Limites no gauge
                    gaugeData[0].gauge.threshold.value = data.temp_max;
                    Plotly.update('temperature-gauge', {}, { 
                        'gauge.threshold.value': data.temp_max 
                    });

                    // Ventilador
                    const fanIcon = document.getElementById('fan-icon');
                    const fanStatus = document.getElementById('fan-status');
                    
                    if (data.fan_state) {
                        fanIcon.classList.add('fan-on');
                        fanStatus.textContent = 'LIGADO';
                        fanStatus.classList.remove('text-gray-400');
                        fanStatus.classList.add('text-green-400');
                    } else {
                        fanIcon.classList.remove('fan-on');
                        fanStatus.textContent = 'DESLIGADO';
                        fanStatus.classList.remove('text-green-400');
                        fanStatus.classList.add('text-gray-400');
                    }

                    // Limites
                    document.getElementById('temp-max').value = data.temp_max;
                    document.getElementById('temp-min').value = data.temp_min;
                    document.getElementById('auto-control').checked = data.auto_control;

                    // Conexão
                    const statusDot = document.getElementById('status-dot');
                    const connStatus = document.getElementById('connection-status');
                    
                    if (data.connected) {
                        statusDot.classList.add('status-connected');
                        statusDot.classList.remove('status-disconnected');
                        connStatus.textContent = 'Conectado';
                        connStatus.classList.add('text-green-400');
                    } else {
                        statusDot.classList.add('status-disconnected');
                        statusDot.classList.remove('status-connected');
                        connStatus.textContent = 'Desconectado';
                        connStatus.classList.remove('text-green-400');
                    }

                    // Informações
                    document.getElementById('last-update').textContent = 
                        data.last_update ? new Date(data.last_update).toLocaleTimeString() : '--';
                    document.getElementById('error-message').textContent = 
                        data.error_message || 'OK';
                })
                .catch(err => console.error('Erro ao atualizar:', err));
        }

        // Controle do ventilador
        function setFan(state) {
            fetch('/api/fan', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ state: state })
            })
            .then(response => response.json())
            .then(data => {
                if (data.error) {
                    alert('Erro: ' + data.error);
                }
            })
            .catch(err => alert('Erro ao controlar ventilador: ' + err));
        }

        // Definir limites
        function setLimits() {
            const tempMax = parseFloat(document.getElementById('temp-max').value);
            const tempMin = parseFloat(document.getElementById('temp-min').value);
            const autoControl = document.getElementById('auto-control').checked;

            if (tempMin >= tempMax) {
                alert('Temperatura mínima deve ser menor que a máxima!');
                return;
            }

            fetch('/api/limits', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ 
                    temp_max: tempMax, 
                    temp_min: tempMin,
                    auto_control: autoControl
                })
            })
            .then(response => response.json())
            .then(data => {
                if (data.error) {
                    alert('Erro: ' + data.error);
                } else {
                    alert('Limites atualizados com sucesso!');
                }
            })
            .catch(err => alert('Erro ao definir limites: ' + err));
        }

        // Inicialização
        document.addEventListener('DOMContentLoaded', function() {
            fetch('/api/config')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('server-url').textContent = data.server_url;
                });
            
            updateDashboard();
            setInterval(updateDashboard, 1000);
        });
    </script>
</body>
</html>
"""

def create_app(state: DashboardState, client: OpcUaClient, server_url: str) -> Flask:
    """
    Cria a aplicação Flask do dashboard.
    
    Args:
        state: Estado compartilhado do dashboard
        client: Cliente OPC-UA
        server_url: URL do servidor OPC-UA
    
    Returns:
        Aplicação Flask configurada
    """
    app = Flask(__name__)
    app.config['server_url'] = server_url
    
    @app.route('/')
    def index():
        """Página principal do dashboard."""
        return render_template_string(DASHBOARD_HTML)
    
    @app.route('/api/status')
    def api_status():
        """Retorna o status atual do sistema."""
        return jsonify(state.to_dict())
    
    @app.route('/api/config')
    def api_config():
        """Retorna a configuração do dashboard."""
        return jsonify({
            'server_url': server_url
        })
    
    @app.route('/api/fan', methods=['POST'])
    def api_fan():
        """Controla o ventilador."""
        try:
            data = request.get_json()
            fan_state = data.get('state', False)
            
            client.run_coroutine(client.set_fan_state(fan_state))
            
            return jsonify({'success': True, 'fan_state': fan_state})
        except Exception as e:
            logger.error(f"Erro ao controlar ventilador: {e}")
            return jsonify({'error': str(e)}), 500
    
    @app.route('/api/limits', methods=['POST'])
    def api_limits():
        """Define os limites de temperatura."""
        try:
            data = request.get_json()
            temp_max = float(data.get('temp_max', DEFAULT_TEMP_MAX))
            temp_min = float(data.get('temp_min', DEFAULT_TEMP_MIN))
            auto_control = data.get('auto_control', True)
            
            if temp_min >= temp_max:
                return jsonify({'error': 'Temperatura mínima deve ser menor que máxima'}), 400
            
            client.run_coroutine(client.set_temp_limits(temp_max, temp_min))
            state.auto_control = auto_control
            
            return jsonify({
                'success': True, 
                'temp_max': temp_max, 
                'temp_min': temp_min,
                'auto_control': auto_control
            })
        except Exception as e:
            logger.error(f"Erro ao definir limites: {e}")
            return jsonify({'error': str(e)}), 500
    
    return app

# =============================================================================
# Ponto de Entrada Principal
# =============================================================================

def main():
    """Função principal do dashboard."""
    parser = argparse.ArgumentParser(
        description='Dashboard OPC-UA para monitoramento de temperatura STM32',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exemplos:
  python opcua_dashboard.py --server opc.tcp://192.168.0.228:4840
  python opcua_dashboard.py --server opc.tcp://192.168.0.228:4840 --port 8080
  python opcua_dashboard.py --temp-max 50 --temp-min 25

NodeIds OPC-UA do servidor:
  - Temperatura:    ns=1;i=1001 (Float, leitura)
  - Estado Fan:     ns=1;i=1002 (Boolean, leitura)
  - Comando Fan:    ns=1;i=1003 (Boolean, escrita)
  - Temp Máxima:    ns=1;i=1004 (Float, leitura/escrita)
  - Temp Mínima:    ns=1;i=1005 (Float, leitura/escrita)
        """
    )
    
    parser.add_argument(
        '--server', '-s',
        default=os.environ.get('OPCUA_SERVER', DEFAULT_SERVER_URL),
        help=f'URL do servidor OPC-UA (padrão: {DEFAULT_SERVER_URL})'
    )
    
    parser.add_argument(
        '--port', '-p',
        type=int,
        default=int(os.environ.get('DASHBOARD_PORT', 5000)),
        help='Porta do servidor web (padrão: 5000)'
    )
    
    parser.add_argument(
        '--temp-max',
        type=float,
        default=float(os.environ.get('TEMP_MAX', DEFAULT_TEMP_MAX)),
        help=f'Temperatura máxima inicial (padrão: {DEFAULT_TEMP_MAX}°C)'
    )
    
    parser.add_argument(
        '--temp-min',
        type=float,
        default=float(os.environ.get('TEMP_MIN', DEFAULT_TEMP_MIN)),
        help=f'Temperatura mínima inicial (padrão: {DEFAULT_TEMP_MIN}°C)'
    )
    
    parser.add_argument(
        '--debug',
        action='store_true',
        help='Habilita modo debug'
    )
    
    parser.add_argument(
        '--standard',
        action='store_true',
        help='Usa modo de conexão padrão da biblioteca asyncua (client.connect())'
    )
    
    args = parser.parse_args()
    
    # Configurar logging
    if args.debug:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # Validar argumentos
    if args.temp_min >= args.temp_max:
        logger.error("Temperatura mínima deve ser menor que a máxima")
        sys.exit(1)
    
    # Criar estado compartilhado
    state = DashboardState(
        temp_max=args.temp_max,
        temp_min=args.temp_min
    )
    
    # Criar cliente OPC-UA
    client = OpcUaClient(args.server, state, standard_mode=args.standard)
    
    if args.standard:
        logger.info("Usando modo de conexão padrão (standard)")
    
    # Criar aplicação Flask
    app = create_app(state, client, args.server)
    
    # Handler para Ctrl+C (saída graciosa)
    def signal_handler(sig, frame):
        logger.info("Recebido sinal de interrupção, encerrando...")
        client.stop()
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Iniciar cliente OPC-UA
    client.start()
    
    # Iniciar servidor web
    logger.info(f"Dashboard disponível em http://localhost:{args.port}")
    logger.info(f"Conectando a {args.server}")
    logger.info(f"Limites: {args.temp_min}°C - {args.temp_max}°C")
    logger.info("Pressione Ctrl+C para sair")
    
    try:
        app.run(
            host='0.0.0.0',
            port=args.port,
            debug=args.debug,
            use_reloader=False,  # Desabilitar reloader para evitar duplicar threads
            threaded=True
        )
    except KeyboardInterrupt:
        pass
    finally:
        client.stop()

if __name__ == '__main__':
    main()
